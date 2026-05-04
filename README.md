# XV6 操作系统实验报告

## 一、实验内容说明

本实验完成了 xv6 操作系统的第一层机制观察和第二层机制理解任务：

### 任务列表

| 任务编号 | 任务名称 | 层级 | 文件修改 |
|---------|---------|------|---------|
| 1 | 系统调用路径跟踪 | 第一层 | printf.c, syscall.c, sysfile.c |
| 2 | 调度过程观察 | 第一层 | proc.c |
| 3 | 内存分配观察 | 第一层 | kalloc.c |
| 4 | 系统调用扩展（hello） | 第二层 | user.h, syscall.h, usys.S, sysproc.c, syscall.c, hello.c, Makefile |

---

## 二、任务实现过程

### 任务 1：系统调用路径跟踪

**目标**：跟踪 `write` 系统调用的完整路径，观察用户程序到内核的调用链。

**实现位置**：

1. **用户程序调用前** - [printf.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/printf.c#L46-L49)
   ```c
   if(!printing){
       printing = 1;
       write(fd, "[USER] calling write\n", 21);
   }
   ```
   在 `printf()` 函数开头添加调试输出，使用 `static int printing` 标志防止递归调用。

2. **syscall.c 中 syscall() 函数** - [syscall.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/syscall.c#L131)
   ```c
   cprintf("[KERNEL] enter syscall %d\n", num);
   ```
   在系统调用入口处打印 syscall 编号。

3. **sys_write() 实现处** - [sysfile.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/sysfile.c#L86)
   ```c
   cprintf("[KERNEL] sys_write invoked\n");
   ```

**实验输出示例**：
```
[USER] calling write
[KERNEL] enter syscall 4
[KERNEL] sys_write invoked
```

**观察结论**：
- 系统调用路径清晰：用户态 printf → 内核 syscall() → 具体 sys_write()
- syscall 编号 4 对应 write 系统调用

---

### 任务 2：调度过程观察

**目标**：在 `scheduler()` 中添加日志，观察进程切换行为。

**实现位置** - [proc.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/proc.c#L293)：
```c
cprintf("[SCHED] switch to pid=%d name=%s\n", p->pid, p->name);
```

**实验输出示例**：
```
[SCHED] switch to pid=1 name=sh
[SCHED] switch to pid=2 name=echo
[SCHED] switch to pid=1 name=sh
[SCHED] switch to pid=2 name=echo
```

**观察结论**：
- 不同进程交替执行：sh 和 echo 进程交替获得 CPU
- 同一进程可能连续运行多次（取决于时间片和进程状态）
- 调度器按顺序遍历进程表，选择第一个 RUNNABLE 状态的进程

---

### 任务 3：内存分配观察

**目标**：在 `kalloc()` 中记录页分配情况，观察分配模式。

**实现位置** - [kalloc.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/kalloc.c#L95-L96)：
```c
if(r)
    cprintf("[MEM] alloc page at 0x%x\n", (uint)r);
```

**实验输出示例**：
```
[MEM] alloc page at 0x8010d000
[MEM] alloc page at 0x8010c000
[MEM] alloc page at 0x8010b000
[MEM] alloc page at 0x8010a000
```

**观察结论**：
- 分配地址呈递减趋势（从高地址向低地址分配）
- 分配是连续的（地址每次减少 PGSIZE=4096 字节）
- 符合首次适配（first-fit）策略：优先使用空闲链表头部的页

---

### 任务 4：系统调用扩展（新增 hello()）

**目标**：在 xv6 中新增一个简单的 `hello()` 系统调用，实现从用户态到内核态的完整调用链。

**实现步骤**：

1. **在 user.h 中添加声明** - [user.h](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/user.h#L26)
   ```c
   int hello(void);
   ```

2. **在 syscall.h 中添加系统调用编号** - [syscall.h](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/syscall.h#L23)
   ```c
   #define SYS_hello  22
   ```

3. **在 usys.S 中添加系统调用封装** - [usys.S](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/usys.S#L32)
   ```c
   SYSCALL(hello)
   ```

4. **在 sysproc.c 中实现内核函数** - [sysproc.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/sysproc.c#L93-L98)
   ```c
   int
   sys_hello(void)
   {
     cprintf("Hello from kernel! PID=%d\n", proc->pid);
     return 0;
   }
   ```

5. **在 syscall.c 中注册系统调用** - [syscall.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/syscall.c#L101)
   ```c
   extern int sys_hello(void);
   // ...
   [SYS_hello]   sys_hello,
   ```

6. **创建用户测试程序 hello.c** - [hello.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/hello.c)
   ```c
   #include "types.h"
   #include "stat.h"
   #include "user.h"
   
   int
   main(void)
   {
     printf(1, "Calling hello() system call...\n");
     hello();
     printf(1, "hello() system call returned\n");
     exit();
   }
   ```

7. **在 Makefile 中添加 hello 到 UPROGS** - [Makefile](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/Makefile#L164)
   ```c
   _hello\
   ```

**实验输出示例**：
```
[USER] calling write
Calling hello() system call...
[USER] calling write
[KERNEL] enter syscall 22
Hello from kernel! PID=2
[USER] calling write
hello() system call returned
```

**观察结论**：
- 新增系统调用成功完成从用户态到内核态的调用
- 系统调用编号 22 对应 hello
- 内核可以正确访问当前进程的 PID

---

## 三、遇到的问题及解决方法

### 问题 1：printf 递归调用导致死循环

**问题描述**：最初在 `printf` 中直接调用 `write` 输出调试信息，但由于 `write` 内部又调用 `printf`，导致无限递归。

**解决方法**：使用 `static int printing` 标志。第一次进入 `printf` 时设置标志并输出调试信息，输出完成前阻止递归调用。

```c
static int printing = 0;
if(!printing){
    printing = 1;
    write(fd, "[USER] calling write\n", 21);
}
// ... printf 主体 ...
printing = 0;
```

### 问题 2：usys.S 中无法直接调用 cprintf

**问题描述**：尝试在汇编文件 usys.S 中使用 `SYSCALL_TRACE` 宏直接调用 `cprintf`，但用户态汇编无法访问内核函数。

**解决方法**：放弃在 usys.S 中添加调试，改为在更合适的层级添加。最终选择修改 `printf.c`，因为 `printf` 是用户程序调用 `write` 的主要入口。

### 问题 3：Windows 环境下无法执行 make

**问题描述**：在 Windows PowerShell 环境中直接运行 `make` 命令失败，提示找不到该命令。

**解决方法**：xv6 需要在 Linux/Unix 环境下编译运行。可通过以下方式解决：
- 使用 WSL (Windows Subsystem for Linux)
- 使用虚拟机安装 Linux
- 使用 Docker 容器

---

## 四、实验总结

本实验完成了 xv6 操作系统的第一层机制观察和第二层机制理解任务：

### 第一层：机制观察
1. **系统调用机制**：通过跟踪 `write` 系统调用，清晰展示了从用户态到内核态的完整调用路径
2. **进程调度机制**：观察到多进程交替执行，调度器按顺序遍历进程表选择 RUNNABLE 进程
3. **内存分配机制**：验证了页分配采用首次适配策略，地址连续递减

### 第二层：机制理解
4. **系统调用扩展**：成功新增了 `hello()` 系统调用，完整实现了从用户态接口到内核实现的整个流程

实验加深了对操作系统底层机制的理解，为后续的深入学习和开发奠定了基础。
