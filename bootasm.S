#include "asm.h"
#include "memlayout.h"
#include "mmu.h"

# Start the first CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.code16                       # 生成16位实模式代码
.globl start
start:
  cli                         # BIOS 开启了中断；现在禁用中断

  # 将数据段寄存器 DS、ES 和 SS 清零
  xorw    %ax,%ax             # 将 %ax 置零
  movw    %ax,%ds             # -> 数据段
  movw    %ax,%es             # -> 附加段
  movw    %ax,%ss             # -> 堆栈段

  # 物理地址线 A20 被绑定到零，这是为了兼容早期只支持 1MB 内存的 PC。
  # 现在我们需要取消这个限制，启用 A20 地址线。
seta20.1:
  inb     $0x64,%al               # 从键盘控制器端口 0x64 读取状态
  testb   $0x2,%al                # 检查位 1（输入缓冲区是否满）
  jnz     seta20.1                # 如果忙，继续等待

  movb    $0xd1,%al               # 0xd1 -> 端口 0x64（发送写命令）
  outb    %al,$0x64               # 告诉控制器我们要写入输出端口

seta20.2:
  inb     $0x64,%al               # 再次等待控制器就绪
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> 端口 0x60
  outb    %al,$0x60               # 0xdf = 11011111，启用 A20 位

  # 从实模式切换到保护模式。
  # 使用一个引导 GDT，使得虚拟地址直接映射到物理地址，
  # 这样在切换过程中有效内存映射不会改变。
  lgdt    gdtdesc                 # 加载全局描述符表
  movl    %cr0, %eax
  orl     $CR0_PE, %eax           # 设置保护模式使能位
  movl    %eax, %cr0              # 现在 CPU 运行在保护模式下

//PAGEBREAK!
  # 通过长跳转重新加载 %cs 和 %eip，完成到32位保护模式的转换。
  # 段描述符设置为无转换，所以映射仍然是恒等映射。
  ljmp    $(SEG_KCODE<<3), $start32  # 跳转到32位代码段

.code32  # 告诉汇编器现在生成32位代码
start32:
  # 设置保护模式下的数据段寄存器
  movw    $(SEG_KDATA<<3), %ax    # 我们的数据段选择子
  movw    %ax, %ds                # -> DS: 数据段
  movw    %ax, %es                # -> ES: 附加段
  movw    %ax, %ss                # -> SS: 堆栈段
  movw    $0, %ax                 # 清零未准备使用的段寄存器
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS

  # 设置堆栈指针并调用 C 函数
  movl    $start, %esp            # 设置堆栈指针（指向 0x7c00）
  call    bootmain                # 跳转到 C 语言的 bootmain 函数

  # 如果 bootmain 返回（它不应该返回），触发 Bochs 断点（如果在 Bochs 下运行），然后循环。
  movw    $0x8a00, %ax            # 0x8a00 -> 端口 0x8a00
  movw    %ax, %dx
  outw    %ax, %dx
  movw    $0x8ae0, %ax            # 0x8ae0 -> 端口 0x8a00
  outw    %ax, %dx
spin:
  jmp     spin                    # 无限循环

# 引导 GDT（全局描述符表）
.p2align 2                                # 强制4字节对齐
gdt:
  SEG_NULLASM                             # 空段描述符（必须为0）
  SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # 代码段：可执行、可读、基址0、限制4GB
  SEG_ASM(STA_W, 0x0, 0xffffffff)         # 数据段：可写、基址0、限制4GB

gdtdesc:
  .word   (gdtdesc - gdt - 1)             # GDT 大小减1
  .long   gdt                             # GDT 地址
