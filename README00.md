# 第一步：最简 Shell — 学会"读取-判断-输出"循环

> 对应代码：`00-main.c`
> 难度：★☆☆☆☆
> 新概念：Shell 主循环、fgets()、strcmp/strncmp、指针偏移

---

## 代码全景

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);

    while(1) {
        printf("$ ");
        char input[100];
        fgets(input, 100, stdin);

        input[strlen(input) - 1] = '\0';  // 去换行符

        if (strcmp(input, "exit") == 0) {
            break;
        } else if (strncmp(input, "echo ", 5) == 0) {
            printf("%s\n", input+5);
        } else if (strncmp(input, "type", 4) == 0) {
            // 用二维数组存储内置命令列表
            char builtins[][100] = {"echo", "type", "exit"};
            int len = sizeof(builtins) / sizeof(builtins[0]);
            int i, flage = 0;

            for (i = 0; i < len; i++) {
                if (strcmp(input+5, builtins[i]) == 0) {
                    printf("%s is a shell builtin\n", input+5);
                    flage = 1;
                    break;
                }
            }
            if (flage != 1) {
                printf("%s: not found\n", input+5);
            }
        } else {
            printf("%s: command not found\n", input);
        }
    }
    return 0;
}
```

这个版本只有 **47 行代码**，能处理 3 个内置命令：`echo`、`type`、`exit`。麻雀虽小，五脏俱全。

---

## 难点 1：Shell 的主循环 — while(1)

```c
while(1) {
    printf("$ ");       // 打印提示符
    fgets(input, ...);  // 等待用户输入
    // ... 处理命令 ...
    // 除非输入 exit，否则回到循环开头
}
```

### 这和你熟悉的程序有什么不同？

你之前写的程序可能是这样的：

```c
int main() {
    int a, b;
    scanf("%d %d", &a, &b);
    printf("%d\n", a + b);
    return 0;  // ← 一次计算就结束了
}
```

Shell 不一样：它需要**一直运行**，处理完一条命令后，继续等用户输入下一条。就像一个餐厅服务员——不会服务完一位顾客就下班，而是不断循环"接单→上菜→等下一单"。

### `while(1)` 表示什么？

```c
while(1) { ... }   // 条件永远是"真"，循环永不停止
```

只有当用户输入 `exit` 时，`break` 才会跳出循环，程序结束。

```
         ┌──────────┐
         │  打印 $   │
         └────┬─────┘
              ▼
         ┌──────────┐
         │ 等待输入  │◄─────────────────┐
         └────┬─────┘                  │
              ▼                        │
         ┌──────────┐                  │
         │ 解析命令  │                  │
         └────┬─────┘                  │
              ▼                        │
    ┌──── exit? ──── 是 ──→ break退出  │
    │         │                        │
    │        否                        │
    ▼                                  │
┌──────────┐                           │
│ 执行命令  │──────────────────────────┘
└──────────┘
```

---

## 难点 2：fgets() — 读一行用户输入

```c
char input[100];
fgets(input, 100, stdin);
```

### 拆解三个参数

| 参数 | 含义 | 类比 |
|------|------|------|
| `input` | 存到哪里 | 购物袋 |
| `100` | 最多读多少个字符 | 购物袋容量 |
| `stdin` | 从哪读（标准输入=键盘） | 从哪个超市买 |

### fgets 的两个特点

**特点 1：会保留末尾的换行符 `\n`**

```c
// 用户输入：echo hello 然后按回车
// input 的内容实际是：
// {'e','c','h','o',' ','h','e','l','l','o','\n','\0'}
//                                        ↑
//                                  换行符也被存进来了！
```

**特点 2：安全，不会溢出**

```c
fgets(input, 100, stdin);
// 即使用户输入了 200 个字符，也只读 99 个（第 100 个留给 \0）
// 这比 gets() 安全得多（gets 已被 C11 标准移除）
```

### 为什么要去掉换行符？

```c
input[strlen(input) - 1] = '\0';   // 把最后一个字符（\n）替换成 \0
```

如果不这样做，`strcmp(input, "exit")` 永远不相等——因为 `input` 末尾多了个 `\n`。

> **注意：** 这种写法假设输入一定以 `\n` 结尾。如果输入刚好 99 字符（没读完换行符），或用户按 Ctrl+D 结束，可能会出错。后续版本会改进这一点。

---

## 难点 3：strcmp vs strncmp — 比较字符串

### strcmp：精确匹配

```c
strcmp("exit", "exit")       // → 0（相等）
strcmp("exit\n", "exit")     // → 非0（不相等，因为有 \n）
strcmp("exitabc", "exit")    // → 非0（不相等）
```

`strcmp` 比较整个字符串，**一个字符都不能多，一个字符都不能少。**

这就是为什么 `exit` 命令用 `strcmp`：

```c
if (strcmp(input, "exit") == 0) {  // input 必须是恰好 "exit"
    break;
}
```

### strncmp：只比较前 N 个字符

```c
strncmp("echo hello", "echo ", 5)  // → 0（前5个字符匹配）
strncmp("echo",      "echo ", 5)  // → 非0（"echo"只有4个字符，第5个不同）
```

`strncmp(str1, str2, n)` 只比较前 `n` 个字符。这对 Shell 很有用——我们只想判断命令是不是以 `"echo "` 开头的。

```c
strncmp(input, "echo ", 5)
// 比较 input 的前 5 个字符 和 "echo " 的前 5 个字符
//  "echo hello"  → 匹配！
//  "echo123"     → 匹配前5个，注意这里的陷阱...
```

---

## 难点 4：指针偏移 `input+5` — 跳过命令名

这是初学者最容易困惑的写法。

```c
// 输入: "echo hello world"
// input 指向的字符串:
//  ↓
// ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
// │ e │ c │ h │ o │   │ h │ e │ l │ l │ o │...│\0 │
// └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
//  位0  位1  位2  位3  位4  位5  位6  位7  位8  位9
//                       ↑
//                    input+5 指向这里

printf("%s\n", input+5);
// 从 input+5 的位置开始打印
// 输出: "hello world"
// 相当于: printf("%s\n", &input[5]);
```

**图解：**

```
input      → e c h o   h e l l o   w o r l d \0
             ^         ^
             |         |
          input      input+5
          (命令名)    (参数部分)
```

**为什么是 `+5`？**
- `"echo "` 正好 5 个字符：e, c, h, o, 空格。
- `"type "` 也是 5 个字符（由于带空格）。

这种写法叫**硬编码的魔法数**，在初级代码中方便，但不够健壮。后续版本会用更通用的方式替代它。

---

## 难点 5：二维数组存储命令列表

```c
char builtins[][100] = {"echo", "type", "exit"};
int len = sizeof(builtins) / sizeof(builtins[0]);
```

### `sizeof` 算数组长度

```
sizeof(builtins)         = 3 行 × 100 字节 = 300 字节
sizeof(builtins[0])      = 1 行 = 100 字节
len = 300 / 100 = 3      ← 有 3 个内置命令
```

### 内存布局

```
  builtins[0]: ┌───┬───┬───┬───┬───┬───┬───┬─...──┬───┐
               │ e │ c │ h │ o │\0 │ ? │ ? │  ?  │ ? │  (100字节)
               └───┴───┴───┴───┴───┴───┴───┴─...──┴───┘
  builtins[1]: ┌───┬───┬───┬───┬───┬───┬───┬─...──┬───┐
               │ t │ y │ p │ e │\0 │ ? │ ? │  ?  │ ? │  (100字节)
               └───┴───┴───┴───┴───┴───┴───┴─...──┴───┘
  builtins[2]: ┌───┬───┬───┬───┬───┬───┬───┬─...──┬───┐
               │ e │ x │ i │ t │\0 │ ? │ ? │  ?  │ ? │  (100字节)
               └───┴───┴───┴───┴───┴───┴───┴─...──┴───┘
```

每行 100 字节，`"echo"` 只用了 5 字节，剩下的 95 字节都浪费了。**后续版本会用指针数组来优化这个问题。**

---

## 难点 6：setbuf(stdout, NULL) — 立即刷新输出

```c
setbuf(stdout, NULL);
```

### 问题：没有这行代码会怎样？

标准输出（`stdout`）默认有缓冲区。`printf` 的内容不是立刻显示在屏幕上的，而是先存在缓冲区里，等缓冲区满了或遇到 `\n` 才输出。

```
没有 setbuf:    有 setbuf:
$               $
(光标在这等)    用户输入→
                    ↓
                立即显示
```

没有它时，可能出现提示符 `$` 不显示，用户不知道可以输入的情况。

**通俗解释：** 就像发微信消息——没有 `setbuf` 是"攒够 10 条一起发"，有 `setbuf` 是"每条立即发送"。

---

## 当前代码的问题（为下一步铺垫）

| 问题 | 说明 | 后续如何解决 |
|------|------|-------------|
| `input+5` 硬编码 | echo/type 各5字符，但 pwd 是3字符就出 bug | 用 `skip_space()` + 通用解析 |
| 二维数组浪费 | 100字节/行，大量浪费 | 改用指针数组 |
| 代码重复 | main 中 type 的逻辑很臃肿 | 抽取 `is_builtins()` 函数 |
| 只能处理 3 个命令 | 无法扩展 | 用更通用的架构 |
| 参数带多余空格 | `echo hello world` 可能被 strncmp 误判 | 用更健壮的解析 |

---

## 小结

这一版实现了 Shell 的**骨架**：

```
  读取输入 → 去除换行 → 比较判断 → 执行/输出 → 回到开头
```

虽然简陋，但它已经具备了一个 Shell 的基本特征：**无限循环等待、解析、执行**。接下来的每一步，都是在给这个骨架"长肉"。
