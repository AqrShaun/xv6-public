// Boot loader.
//
// Part of the boot block, along with bootasm.S, which calls bootmain().
// bootasm.S has put the processor into protected 32-bit mode.
// bootmain() loads an ELF kernel image from the disk starting at
// sector 1 and then jumps to the kernel entry routine.

#include "types.h"
#include "elf.h"
#include "x86.h"
#include "memlayout.h"

#define SECTSIZE  512

void readseg(uchar*, uint, uint);

void
bootmain(void)
{
  struct elfhdr *elf;
  struct proghdr *ph, *eph;
  void (*entry)(void);
  uchar* pa;

  // 将ELF头部加载到内存地址0x10000处（64KB）
  // 这个地址在实模式下是可访问的（低于1MB）
  elf = (struct elfhdr*)0x10000;  // 临时空间

  // 从磁盘读取第一页（4KB）数据到elf指针指向的位置
  // 这包含了ELF头部和可能的程序头表
  readseg((uchar*)elf, 4096, 0);

  // 检查是否是有效的ELF可执行文件
  // ELF_MAGIC是魔数0x7F 'E' 'L' 'F'
  if(elf->magic != ELF_MAGIC)
    return;  // 如果不是ELF文件，让bootasm.S处理错误（进入无限循环）

  // 加载每个程序段（忽略ph标志）
  // phoff: 程序头表在文件中的偏移量
  ph = (struct proghdr*)((uchar*)elf + elf->phoff);
  eph = ph + elf->phnum;  // 程序头表的结束位置
  
  // 遍历所有程序段头
  for(; ph < eph; ph++){
    // paddr: 该段应加载到的物理地址
    pa = (uchar*)ph->paddr;
    
    // 从磁盘读取该段到内存
    // filesz: 段在文件中的大小
    // off: 段在文件中的偏移量
    readseg(pa, ph->filesz, ph->off);
    
    // 如果内存大小大于文件大小（即存在.bss段）
    // 将剩余部分清零（初始化未初始化的数据）
    if(ph->memsz > ph->filesz)
      stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
  }

  // 从ELF头部获取入口点地址并跳转
  // 这是内核的入口函数，不会返回！
  entry = (void(*)(void))(elf->entry);
  entry();  // 跳转到内核代码
}

// 等待磁盘就绪
void
waitdisk(void)
{
  // 等待磁盘准备好
  // 0x1F7是IDE状态寄存器
  // 0xC0 = 11000000b，检查位7（忙）和位6（就绪）
  // 0x40 = 01000000b，等待直到就绪位为1且忙位为0
  while((inb(0x1F7) & 0xC0) != 0x40)
    ;
}

// 读取单个扇区到dst，offset是扇区号
void
readsect(void *dst, uint offset)
{
  // 等待磁盘就绪
  waitdisk();
  
  // 向IDE控制器发送读扇区命令
  outb(0x1F2, 1);   // 扇区数 = 1（端口0x1F2）
  outb(0x1F3, offset);         // LBA低8位（端口0x1F3）
  outb(0x1F4, offset >> 8);    // LBA中8位（端口0x1F4）
  outb(0x1F5, offset >> 16);   // LBA高8位（端口0x1F5）
  // 端口0x1F6：位0-3 = LBA最高4位，位4=0主盘，位6=1LBA模式，位5=1，位7=1
  outb(0x1F6, (offset >> 24) | 0xE0);
  outb(0x1F7, 0x20);  // 命令0x20 - 读取扇区（端口0x1F7）

  // 读取数据（等待磁盘准备好后）
  waitdisk();
  
  // 从数据端口0x1F0读取512字节（SECTSIZE/4 = 128个双字）
  insl(0x1F0, dst, SECTSIZE/4);
}

// 从内核的'offset'处读取'count'字节到物理地址'pa'
// 可能会读取比要求更多的数据（按扇区对齐）
void
readseg(uchar* pa, uint count, uint offset)
{
  uchar* epa;

  // 计算结束地址
  epa = pa + count;

  // 向下舍入到扇区边界
  // 因为磁盘读写必须以扇区为单位（512字节）
  pa -= offset % SECTSIZE;

  // 将字节偏移转换为扇区号
  // 内核从扇区1开始（扇区0是引导扇区）
  offset = (offset / SECTSIZE) + 1;

  // 如果这太慢，我们可以一次读取多个扇区
  // 我们可能会写入比要求更多的内存，但这没关系
  // 因为我们按递增顺序加载
  for(; pa < epa; pa += SECTSIZE, offset++)
    readsect(pa, offset);  // 每次读取一个扇区
}
