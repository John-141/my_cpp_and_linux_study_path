# 第四步：fork/exec — Shell 真正的灵魂

> 对应代码：`step03/03-main.c`
> 前置知识：[README02.md](../README02.md) — PATH 解析、环境变量、strtok
> 难度：★★★★★（本系列最难的一步）
> 新概念：fork()、execvp()、waitpid()、argv 数组、进程管理

---

## 从 02 到 03 的进化

| 方面 | 02-main.c | 03-main.c | 新增什么 |
|------|-----------|-----------|---------|
| 头文件 | - | `sys/wait.h` | 进程等待 |
| 外部命令执行 | 只显示 "not found" | `exec_command()` 函数 | **真正能运行 ls、cat 了！** |
| 命令参数 | 只做 type 的参数解析 | `strtok` 分割所有参数 | 支持 `ls -l -a` |
| 进程管理 | 无 | fork + execvp + waitpid | 本步核心 |

**这是最关键的一步！** 从"只认识命令"升级为"真正能执行命令"。

---

## 核心概念预览

把所有概念串成一句话：

> Shell 用 **fork()** 克隆自己，让克隆体用 **execvp()** 变成另一个程序，自己用 **waitpid()** 等克隆体跑完。

下面逐个拆解。

---

## 难点 1：为什么不能直接 exec？

### 先看 execvp 做了什么

```c
execvp("/bin/ls", argv);
```

这**不是**一个普通的函数调用。它不是"调用 ls 函数然后返回"，而是：

```
┌─────────────────────┐        execvp()        ┌─────────────────────┐
│     当前进程          │   ═══════════════>    │     同一个进程        │
│                     │                       │                     │
│  PID = 100          │                       │  PID = 100 (不变)    │
│  代码 = Shell       │                       │  代码 = ls          │
│  数据 = Shell 的    │                       │  数据 = ls 的       │
│  堆栈 = Shell 的    │                       │  堆栈 = ls 的       │
└─────────────────────┘                       └─────────────────────┘

     Shell 自身不复存在！                       完全变成了 ls 程序
```

**就像把你的一本书的所有内容擦掉，印上另一本书的内容。** 书还是那本书（同一个进程 ID），但内容全换了。

### 如果直接在 Shell 里调用 execvp

```c
int main() {
    printf("$ ");
    fgets(input, ...);
    
    execvp("/bin/ls", argv);   // ← Shell 被 ls 替换了！
    
    printf("$ ");              // ← 这行永远不会执行
    // Shell 已经不存在了，没有进程在等用户的下一条命令
}
```

**结果：** 执行完 `ls` 后没有进程在循环等待输入——Shell 死了。

---

## 难点 2：fork() — 一次调用，两次返回

### fork 在做什么？

```
调用 fork() 之前：
┌─────────────────┐
│   父进程          │
│   PID = 100      │
│   代码 = Shell   │
└─────────────────┘

调用 fork() 之后：
┌─────────────────┐      ┌─────────────────┐
│   父进程          │      │   子进程          │
│   PID = 100      │      │   PID = 200      │
│   代码 = Shell   │      │   代码 = Shell   │  ← 一模一样！
│   数据 = Shell   │      │   数据 = Shell   │  ← 完全复制！
└─────────────────┘      └─────────────────┘

fork() 返回 200          fork() 返回 0
(子进程的 PID)            (我是子进程的标记)
```

**子进程是父进程的"影分身"：** 创建时一模一样，但之后各走各的路。

### "一次调用，两次返回"的直观理解

```c
printf("fork 之前\n");           // 执行 1 次

pid_t pid = fork();              // ← 只调用 1 次函数

// ══════ 从这里开始，有两份代码在同时运行 ══════

printf("fork 之后，pid = %d\n", pid);  // 执行 2 次！
```

**运行结果：**

```
fork 之前                         ← 只打印 1 次
fork 之后，pid = 200              ← 父进程打印的
fork 之后，pid = 0                ← 子进程打印的
```

### 为什么初学者觉得反直觉？

你以前写的所有函数，调用一次就返回一次：

```c
int result = add(3, 5);  // 调用 → 计算 → 返回 → 继续
```

但 `fork()` 违反了这个常规——它返回了两次，在两个不同的进程中！

**生活类比：** 就像在游戏里使用"分身术"。你施法（调用 fork）一次，但出现了两个你：本体（父进程）和分身（子进程）。本体手里的令牌写着分身的编号（PID > 0），分身手里的令牌写着 0（告诉我我是分身）。

---

## 难点 3：基于 fork 返回值的分支判断

```c
pid_t pid = fork();

if (pid < 0) {
    // fork 失败了（极少见，内存不足时）
    perror("fork");
    
} else if (pid == 0) {
    // ▼▼▼ 子进程走这条路 ▼▼▼
    execvp(exec_path, argv);   // 把自己替换成 ls
    
} else {
    // ▼▼▼ 父进程走这条路 ▼▼▼
    waitpid(pid, &status, 0); // 等子进程结束
}
```

### 执行流图解

```
                  main()
                    │
                    │ fork()
                    │
        ┌───────────┴───────────┐
        │                       │
    fork() 返回 200          fork() 返回 0
    (子进程的PID)            (我是子进程)
        │                       │
    走 else 分支             走 if(pid==0) 分支
    即: 父进程代码            即: 子进程代码
        │                       │
    waitpid(200,...)         execvp("/bin/ls",...)
    阻塞等待...              ┌─────────────────┐
        │                   │ 进程变成 ls      │
        │  ⏳               │ ls 开始运行      │
        │  ⏳               │ ls 输出结果      │
        │  ⏳               │ ls 结束 → 退出   │
        │                   └─────────────────┘
    waitpid 返回                进程结束 ✕
        │
    继续 Shell 主循环
    printf("$ ");
```

---

## 难点 4：execvp 的参数 — argv 数组

### execvp 的签名

```c
execvp(const char *file, char *const argv[]);
//       ↑                    ↑
//    可执行文件路径         参数数组（必须以 NULL 结尾）
```

### argv 数组的构建过程

以 `ls -l /home` 为例：

```c
char* argv[MAX_INPUT];   // 存放参数指针的数组
int argc = 0;

argv[argc++] = (char*)cmd_name;   // argv[0] = "ls"
```

参数部分 `"-l /home"` 需要拆开：

```c
char* args_copy = strdup(args_str);  // 复制：strtok 会破坏原串
char* token = strtok(args_copy, " "); // token = "-l"

while (token != NULL && argc < MAX_INPUT - 1) {
    argv[argc++] = token;             // argv[1] = "-l"
    token = strtok(NULL, " ");        // token = "/home"
}                                      // argv[2] = "/home"

argv[argc] = NULL;                    // 哨兵结束
```

### 最终 argv 数组的样子

```
argv:
┌───────────┐
│ argv[0] ──┼──→ "ls"
├───────────┤
│ argv[1] ──┼──→ "-l"
├───────────┤
│ argv[2] ──┼──→ "/home"
├───────────┤
│ argv[3] ──┼──→ NULL    ← 必须！execvp 用它判断参数结束
└───────────┘
```

**这和 `main(int argc, char* argv[])` 中的 argv 是一回事。** 你写的 C 程序接收的命令行参数，就是 Shell 构建的。

> **关键：** execvp 的第二个参数的第一个元素 `argv[0]`，在传统上是程序名，但可以任意设置——Shell 通常直接填命令名。

---

## 难点 5：waitpid — 父进程的"等待"

```c
int status;
waitpid(pid, &status, 0);
```

### 三个参数

| 参数 | 含义 | 这里传了什么 |
|------|------|-------------|
| `pid` | 等哪个子进程 | 子进程的 PID（fork 的返回值） |
| `&status` | 子进程的退出状态存哪 | status 变量的地址 |
| `0` | 选项（0 = 默认阻塞等待） | 0，即一直等到子进程结束 |

### 为什么必须等？

如果没有 `waitpid`：

```bash
$ ls
$ Desktop Documents Downloads  ← ls 的输出
                                ← 提示符和输出混在一起！
```

有了 `waitpid` 后才是正常的：

```bash
$ ls
Desktop Documents Downloads
$                              ← 等 ls 结束才打印提示符
```

**本质：** `waitpid` 让父进程"卡住"（阻塞），直到子进程结束。这就是**前台进程**的实现原理。

### 如果不等会怎样？（僵尸进程）

如果父进程不等子进程结束，子进程退出后会变成**僵尸进程（Zombie）**——进程已死，但进程表项还在，占用系统资源。`waitpid` 负责"收尸"，清理子进程的资源。

---

## 难点 6：子进程中的 exit(1) 和 perror

```c
if (pid == 0) {
    execvp(exec_path, argv);   // 如果成功，下面都不会执行
    perror("execvp");           // 执行到这里说明 execvp 失败了
    exit(1);                    // ← 为什么用 exit 而不是 return？
}
```

### perror 的作用

`perror("execvp")` 会在错误信息前加上你给的标签：

```
execvp: No such file or directory
//      ↑                       ↑
//   你给的标签            系统的错误描述
```

它根据全局变量 `errno` 自动翻译成人类可读的错误信息。

### 为什么用 exit(1) 而不是 return？

```c
// ❌ 如果子进程用 return
if (pid == 0) {
    execvp(...);    // 失败
    perror(...);
    return;         // 从 exec_command() 返回，回到 main()
}                   // main 继续执行 Shell 主循环！

// 结果：子进程变成了一个"幽灵 Shell"
// 有两个进程在等用户输入！混乱！
```

```c
// ✅ 正确做法：用 exit
if (pid == 0) {
    execvp(...);    // 失败
    perror(...);
    exit(1);        // 立即终止子进程，绝不回头！
}
```

**关键区别：**
- `return` → 从函数返回，继续执行调用者代码
- `exit()` → 终止整个进程，清理所有资源

---

## 完整执行流程举例

用户输入：`ls -l /tmp`

```
01. main() 读取 "ls -l /tmp"
        │
02.     解析: cmd_name = "ls", args = "-l /tmp"
        │
03.     strcmp("ls", "exit") → 否
        strcmp("ls", "echo") → 否
        strcmp("ls", "type") → 否
        → 走 else 分支
        │
04.     exec_command("ls", "-l /tmp")
        │
05.     find_in_path("ls")
        → getenv("PATH") = "/usr/bin:/bin:..."
        → strtok 逐目录查找
        → 在 /bin 找到 ls!
        → 返回 "/bin/ls"
        │
06.     构建 argv:
        argv[0]="ls", argv[1]="-l", argv[2]="/tmp", argv[3]=NULL
        │
07.     fork() ────────────────────┐
        │                          │
    父进程(PID=100)              子进程(PID=200)
    fork返回200                  fork返回0
        │                          │
08. waitpid(200,...)          09. execvp("/bin/ls", argv)
    阻塞等待...                    → 子进程变成 ls 程序
        │                          → ls 读取 /tmp 目录
        │ ⏳                        → ls 输出文件列表
        │ ⏳                        → ls 结束，进程退出
        │                          ✕
10. waitpid 返回
        │
11. 继续 while(1) → printf("$ ")
        │
    等待下一条命令...
```

---

## 代码中的一个内存管理细节

注意 `args_copy` 的生命周期问题：

```c
void exec_command(const char* cmd_name, char* args_str) {
    // ...
    if (args_str != NULL && *args_str != '\0') {
        char* args_copy = strdup(args_str);
        char* token = strtok(args_copy, " ");
        while (token != NULL && argc < MAX_INPUT - 1) {
            argv[argc++] = token;   // argv[1] 指向 args_copy 内的地址
            token = strtok(NULL, " ");
        }
    }
    // ... fork/exec ...
    // 注意：这里没有 free(args_copy)！
}
```

这是一个**有意的设计**：`argv[i]` 中的指针指向 `args_copy` 的内存。如果 fork 之前就 free 了，子进程的 `argv` 就变成了悬空指针。要么在 if-else 两个分支都 free（但子进程 execvp 成功后会换掉整个内存空间），要么就不 free（进程退出时 OS 回收）。

> 04 版本会修复这个，把 `free` 放在父进程分支中。

---

## 小结：Shell 四大核心机制至此全部到位

| 机制 | 出现于 | 作用 |
|------|--------|------|
| 主循环 + 输入解析 | 00 | 等待用户输入 |
| 内置命令判断 | 01 | 识别 echo/type/exit |
| PATH 可执行文件查找 | 02 | 定位外部命令 |
| **fork/exec/wait** | **03（本步）** | **真正运行外部命令** |

### 一句话总结整个流程

```
用户输入 → 解析命令 → 内置？直接干
                     → 外部？fork 分身 → execvp 变装 → waitpid 等结束
```

**恭喜！** 到这里你已经理解了 Unix Shell 最核心的进程管理机制。这是操作系统课程的经典内容，也是面试中的高频考点。下一步（04）将加入 `cd` 命令和一些代码优化。
