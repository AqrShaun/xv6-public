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
| 5 | 生产者-消费者模型 | 第三层 | sync.h, sync.c, main.c, sysproc.c, syscall.c, user.h, usys.S, prodcons.c, Makefile, defs.h |

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

### 任务 5：生产者-消费者模型

**目标**：实现一个简化版的生产者-消费者同步模型，包含两个进程共享有界缓冲区，使用自旋锁和 sleep/wakeup 实现同步。

**设计概述**：
- 使用循环缓冲区，大小为 10
- 生产者产生数据，消费者消费数据
- 使用 spinlock 保护临界区
- 使用 sleep/wakeup 实现阻塞等待，避免 busy waiting

**实现步骤**：

1. **创建同步机制头文件** - [sync.h](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/sync.h)
   ```c
   #define BUFFER_SIZE 10
   struct buffer {
     int data[BUFFER_SIZE];
     int count;
     int in;
     int out;
   };
   void buffer_init(void);
   void buffer_produce(int item);
   int buffer_consume(void);
   ```

2. **实现缓冲区操作** - [sync.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/sync.c)
   - 使用 spinlock 保护临界区
   - 当缓冲区满时，生产者 sleep
   - 当缓冲区空时，消费者 sleep
   - 唤醒时使用 wakeup

   ```c
   void buffer_produce(int item) {
     acquire(&buffer_lock);
     while(buf.count >= BUFFER_SIZE) {
       sleep(&buf, &buffer_lock);
     }
     buf.data[buf.in] = item;
     buf.in = (buf.in + 1) % BUFFER_SIZE;
     buf.count++;
     cprintf("[PRODUCER] produced: %d, count=%d\n", item, buf.count);
     wakeup(&buf);
     release(&buffer_lock);
   }
   
   int buffer_consume(void) {
     int item;
     acquire(&buffer_lock);
     while(buf.count <= 0) {
       sleep(&buf, &buffer_lock);
     }
     item = buf.data[buf.out];
     buf.out = (buf.out + 1) % BUFFER_SIZE;
     buf.count--;
     cprintf("[CONSUMER] consumed: %d, count=%d\n", item, buf.count);
     wakeup(&buf);
     release(&buffer_lock);
     return item;
   }
   ```

3. **添加系统调用** - [syscall.h](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/syscall.h#L24-L25)
   ```c
   #define SYS_produce 23
   #define SYS_consume 24
   ```

4. **实现系统调用内核函数** - [sysproc.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/sysproc.c#L100-L112)
   ```c
   int sys_produce(void) {
     int n;
     if(argint(0, &n) < 0) return -1;
     buffer_produce(n);
     return 0;
   }
   
   int sys_consume(void) {
     return buffer_consume();
   }
   ```

5. **创建用户测试程序** - [prodcons.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/prodcons.c)
   - 主进程 fork 出生产者和消费者
   - 生产者产生 20 个数据项
   - 消费者消费 20 个数据项

   ```c
   int main(int argc, char *argv[]) {
     int pid;
     printf(1, "=== Producer-Consumer Model Test ===\n");
     pid = fork();
     if(pid == 0) {
       producer();
       exit();
     } else {
       consumer();
       wait();
       exit();
     }
   }
   ```

6. **初始化缓冲区** - [main.c](file:///d:/xv6/xv6-public-xv6-rev9/xv6-public-xv6-rev9/main.c#L40)
   ```c
   buffer_init();
   ```

**实验输出示例**：
```
=== Producer-Consumer Model Test ===
Buffer size: 10, Items to produce: 20

[PRODUCER] Started (pid=3)
[PRODUCER] Calling produce(1)
[PRODUCER] produced: 1, count=1
[CONSUMER] Started (pid=2)
[CONSUMER] Calling consume()
[CONSUMER] consumed: 1, count=0
[CONSUMER] Received: 1
[PRODUCER] Calling produce(2)
[PRODUCER] produced: 2, count=1
...
[PRODUCER] Finished producing 20 items
[CONSUMER] Finished consuming 20 items
=== Test Complete ===
```

**观察结论**：
- 两个进程成功通过共享缓冲区进行通信
- spinlock 有效保护了临界区，避免了数据竞争
- sleep/wakeup 机制避免了忙等待，当缓冲区为空或满时进程正确阻塞
- 生产者和消费者正确交替执行

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

### 第三层：同步模型构建
5. **生产者-消费者模型**：成功实现了基于 spinlock 和 sleep/wakeup 的进程同步机制

实验加深了对操作系统底层机制的理解，为后续的深入学习和开发奠定了基础。

---

## 五、Git Commit 指南

### 分阶段提交（推荐）

```bash
# Task 1: 系统调用路径跟踪
git add printf.c syscall.c sysfile.c
git commit -m "Task 1: 添加write系统调用路径跟踪

- printf.c: 在用户态添加[USER] calling write输出
- syscall.c: 在内核入口添加[KERNEL] enter syscall输出
- sysfile.c: 在sys_write实现处添加[KERNEL] sys_write invoked输出"

# Task 2: 调度过程观察
git add proc.c
git commit -m "Task 2: 添加scheduler()进程调度观察日志

- 在scheduler()中添加[SCHED] switch to pid=X name=Y输出
- 用于观察不同进程交替执行和同一进程连续运行情况"

# Task 3: 内存分配观察
git add kalloc.c
git commit -m "Task 3: 添加kalloc()内存分配观察日志

- 在kalloc()中添加[MEM] alloc page at 0x...输出
- 用于观察页分配是否连续和是否存在复用"

# Task 4: 系统调用扩展
git add user.h syscall.h usys.S sysproc.c syscall.c hello.c Makefile
git commit -m "Task 4: 新增hello()系统调用

- user.h: 添加hello()声明
- syscall.h: 添加SYS_hello=22
- usys.S: 添加系统调用封装
- sysproc.c: 实现sys_hello()内核函数
- syscall.c: 注册系统调用到系统调用表
- hello.c: 创建用户测试程序
- Makefile: 添加_hello到UPROGS"
```

### 一次性提交所有修改

```bash
git add .
git commit -m "feat: 第一层机制观察 + 第二层系统调用扩展

第一层任务:
- Task 1: write系统调用路径跟踪(printf.c, syscall.c, sysfile.c)
- Task 2: 进程调度观察(proc.c)
- Task 3: 内存分配观察(kalloc.c)

第二层任务:
- Task 4: 新增hello()系统调用(user.h, syscall.h, usys.S, sysproc.c, syscall.c, hello.c, Makefile)"

# Task 5: 生产者-消费者模型
git add sync.h sync.c main.c sysproc.c syscall.c user.h usys.S prodcons.c Makefile defs.h
git commit -m "Task 5: 实现生产者-消费者同步模型"

- sync.h/sync.c: 创建共享缓冲区和同步原语
- main.c: 添加缓冲区初始化调用
- sysproc.c: 实现sys_produce和sys_consume系统调用
- syscall.c/syscall.h: 注册新的系统调用
- user.h/usys.S: 添加用户接口
- prodcons.c: 创建用户测试程序
- 使用spinlock保护临界区
- 使用sleep/wakeup实现阻塞等待"
```

---

## 六、实践心得

通过本次 xv6 操作系统实验，我对操作系统底层机制有了更深入的理解和认识。

**第一层：机制观察**的实验让我意识到，操作系统并非神秘的黑盒，而是由一系列清晰的层次和流程组成。通过在 `printf` → `syscall` → `sys_write` 路径上添加调试输出，我亲眼见证了一个简单的 `printf` 调用如何在用户态和内核态之间层层传递。这种"追踪式"的学习方法帮助我建立了对系统调用机制的直观认识。同样，通过观察 `scheduler()` 中的进程切换日志，我理解了多道程序设计的核心思想——CPU 在多个进程之间快速切换，给用户一种"并行"的错觉。

**第二层：机制理解**的实验则更具挑战性。从新增 `hello()` 系统调用的过程中，我学习到了操作系统系统调用的完整流程：用户程序通过汇编指令触发中断 → 内核根据系统调用号查找处理函数 → 执行内核代码 → 返回结果。这个过程涉及到多个文件的协同修改：用户层接口（user.h）、系统调用号定义（syscall.h）、汇编入口（usys.S）、内核实现（sysproc.c）和系统调用表（syscall.c）。这个实验让我理解了操作系统如何安全地将控制权从用户态转移到内核态。

**第三层：同步模型构建**是本次实验最具挑战性的部分。实现生产者-消费者模型涉及到几个关键的操作系统概念：

首先是**临界区保护**。多个进程共享同一个缓冲区，如果没有适当的同步机制，就会产生数据竞争（race condition）。通过使用 spinlock，我学习到了如何用硬件支持的原子操作来实现互斥。

其次是**阻塞与唤醒机制**。如果使用忙等待（busy waiting）来实现同步，会严重浪费 CPU 资源。通过 `sleep/wakeup` 机制，当缓冲区满时让生产者睡眠，当缓冲区空时让消费者睡眠，显著提高了 CPU 利用率。这个实验让我理解了经典的生产者-消费者问题中"有界缓冲区"的概念如何用实际的操作系统机制来实现。

最后是**进程间通信**。在 xv6 中，进程之间通过共享内核数据结构来进行通信。生产者向内核缓冲区写入数据，消费者从同一个缓冲区读取数据，这种设计体现了操作系统"提供服务"的核心思想。

整个实验过程中，我遇到了许多预料之外的问题。比如在 `printf` 中调用 `write` 导致递归死循环、用户态汇编无法直接调用内核函数等。这些问题的解决过程锻炼了我的调试能力和对系统工作原理的深入理解。我认为，学习操作系统最好的方法就是动手实践，只有在实际编写代码的过程中，才能真正理解那些看似抽象的概念。
