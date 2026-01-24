# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.
        
# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"

# Multiboot header.  Data to direct multiboot loader.
# 多引导头，用于GNU Grub等多引导加载器
# 符合Multiboot规范，允许使用GRUB等引导器启动xv6
.p2align 2  # 强制4字节对齐（Multiboot规范要求）
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002  # Multiboot魔数
  #define flags 0           # 标志位
  .long magic               # 必需：魔数，标识这是一个Multiboot兼容的内核
  .long flags               # 必需：标志位，指定引导加载器的行为
  .long (-magic-flags)      # 必需：校验和，所有三个字段的和必须为0

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
# 按照约定，_start符号指定ELF入口点
# 由于我们还没有设置虚拟内存，我们的入口点是'entry'的物理地址
.globl _start
_start = V2P_WO(entry)  # 转换为物理地址（V2P_WO = Virtual to Physical, Write-Only）

# Entering xv6 on boot processor, with paging off.
# 在引导处理器上进入xv6，此时分页机制尚未开启
.globl entry
entry:
  # ==================== 阶段1：设置内存分页 ====================
  
  # 1.1 启用4MB大页支持
  # 开启CR4寄存器的PSE位，允许使用4MB大页，而不是默认的4KB小页
  # 这可以简化早期内存管理
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax    # 设置Page Size Extension位
  movl    %eax, %cr4
  
  # 1.2 设置页目录
  # 将页目录的物理地址加载到CR3寄存器
  # entrypgdir是内核的初始页目录，定义在main.c中
  # 它将低地址内存（0-4MB）映射到自身，建立恒等映射
  movl    $(V2P_WO(entrypgdir)), %eax  # 获取页目录的物理地址
  movl    %eax, %cr3                    # 设置页目录基址寄存器
  
  # 1.3 启用分页机制
  # 开启CR0寄存器的PG位，激活分页
  # 同时设置WP位（写保护），防止内核代码被意外修改
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax      # PG=分页使能，WP=写保护
  movl    %eax, %cr0                  # 从此CPU开始使用虚拟地址
  
  # ==================== 阶段2：设置内核栈 ====================
  
  # 设置堆栈指针，为C函数调用做准备
  # stack是内核栈的基址，KSTACKSIZE是栈大小（通常是4096字节）
  # 栈指针指向栈顶（栈向下增长）
  movl $(stack + KSTACKSIZE), %esp    # 设置栈指针到栈顶
  
  # ==================== 阶段3：跳转到C代码 ====================
  
  # 跳转到main()函数，并切换到高地址执行
  # 间接调用是必要的，因为汇编器会为直接跳转生成PC相对指令
  # 但在启用分页后，我们需要绝对地址跳转
  mov $main, %eax      # 将main函数的地址加载到eax
  jmp *%eax            # 间接跳转到main()，开始执行内核C代码

# ==================== 内核栈定义 ====================
# 为内核分配栈空间
# .comm声明一个未初始化的通用内存区域
# stack是符号名，KSTACKSIZE是大小
.comm stack, KSTACKSIZE  # 分配KSTACKSIZE字节的内核栈空间
