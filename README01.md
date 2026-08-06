# 第二步：函数封装 — 从"面条代码"到模块化

> 对应代码：`01-main.c`
> 前置知识：[README00.md](README00.md) — 主循环、fgets、strcmp/strncmp
> 难度：★★☆☆☆
> 新概念：函数封装、指针数组、NULL 哨兵、指针运算、宏定义、防御性编程

---

## 从 00 到 01 的进化

| 方面 | 00-main.c | 01-main.c | 改进 |
|------|-----------|-----------|------|
| 代码行数 | 47 | 71 | 看起来更多了，但更清晰 |
| 命令判断 | 全部写在 main 里 | `is_builtins()` 函数 | 可复用 |
| 空格处理 | 硬编码 `input+5` | `skip_space()` 函数 | 通用、灵活 |
| 命令存储 | 二维数组 `[][100]` | 指针数组 `*[]` | 省内存、更优雅 |
| 缓冲区 | `char input[100]` | `char input[MAX_INPUT]` | 可配置 |
| 输入检查 | 无 `fgets` 返回值检查 | `if(fgets(...) == NULL)` | 更健壮 |

---

## 难点 1：指针数组 — 用指针代替二维数组

### 00 版本的问题代码

```c
// 00-main.c：每行必须 100 字节
char builtins[][100] = {"echo", "type", "exit"};
int len = sizeof(builtins) / sizeof(builtins[0]);  // 需要手动算长度
```

### 01 版本的改进代码

```c
// 01-main.c：每个字符串只占自身所需的字节
char *builtins[] = {"echo", "type", "exit", NULL};
//                  ↑      ↑      ↑       ↑
//                 指针    指针    指针    哨兵

for (i = 0; builtins[i] != NULL; i++) {
    if (strcmp(cmd, builtins[i]) == 0) {
        return 1;
    }
}
```

### 内存对比

**二维数组 `char builtins[3][100]`：**

```
300 字节总大小
┌─────────────────────────────────┐
│ e c h o \0 [95字节空白]         │  ← "echo"
│ t y p e \0 [95字节空白]         │  ← "type"
│ e x i t \0 [95字节空白]         │  ← "exit"
└─────────────────────────────────┘
```

**指针数组 `char *builtins[]`：**

```
约 24 字节(指针数组) + 14 字节(字符串)
┌──────────────────────┐
│ 0x1000 ──────────┐   │  ← 指针1(8字节)
│ 0x2000 ──────┐   │   │  ← 指针2(8字节)
│ 0x3000 ──┐   │   │   │  ← 指针3(8字节)
│ NULL     │   │   │   │  ← 哨兵(8字节)
└──────────┼───┼───┼───┘
           │   │   └──→ "exit\0"    (5字节)
           │   └──────→ "type\0"    (5字节)
           └──────────→ "echo\0"    (5字节)
```

**节省了约 260 字节！** 当命令列表变长时，差距更明显。

### 为什么遍历不需要长度？

```c
for (i = 0; builtins[i] != NULL; i++)  // NULL 就是"终点"标志
```

这就像走一条路，不需要事先知道有多长——看到"终点"标志就知道走完了。`NULL` 就是这个标志，术语叫**哨兵值（Sentinel）**。

> **注意：** 必须手动在数组末尾加 `NULL`。如果忘了：
> ```c
> char *builtins[] = {"echo", "type", "exit"}; // ❌ 没有 NULL！
> for (i = 0; builtins[i] != NULL; i++) // 越界访问，程序崩溃！
> ```

---

## 难点 2：skip_space() — 指针走路的艺术

```c
char* skip_space(const char* str) {
    while (*str != '\0' && *str == ' ') {
        str++;    // 指针向后移动一个字符
    }
    return (char*)str;
}
```

### 逐行解析

```c
while (*str != '\0' && *str == ' ')
//      ↑             ↑
//  还没到末尾    当前字符是空格
```

- `*str`：解引用指针，取出当前指向的那个字符
- `*str != '\0'`：字符串还没结束（`\0` 是 C 字符串的结束标志）
- `*str == ' '`：当前字符是空格

```c
str++;   // 指针+1，指向下一个字符的地址
```

### 图解执行过程

```
输入 "   hello" (开头有3个空格)

循环第1次:  *str = ' ' → 是空格 → str++
            ↓
            "   hello"
             ↑ 指针位置

循环第2次:  *str = ' ' → 是空格 → str++
            ↓
            "   hello"
              ↑ 指针位置

循环第3次:  *str = ' ' → 是空格 → str++
            ↓
            "   hello"
               ↑ 指针位置

循环第4次:  *str = 'h' → 不是空格 → 退出循环
            ↓
            "   hello"
                ↑ 返回这个指针

结果: 返回指向 'h' 的指针
```

### 为什么 return 时要 `(char*)str`？

函数参数声明为 `const char* str`（承诺不修改内容），但返回值需要的类型中 `const` 层级不同。强制转换 `(char*)` 是为了消除编译警告。这是 C 语言中处理 `const` 的一个小技巧。

---

## 难点 3：宏定义 `#define` — 给数字起个名字

```c
#define MAX_INPUT 4096
```

之前在 00 版本中：

```c
char input[100];      // 为什么是 100？够用吗？
fgets(input, 100, stdin);
```

如果有人输入超长的命令（比如管道拼接很多命令），100 字符可能不够。改成：

```c
char input[MAX_INPUT];                    // MAX_INPUT = 4096
fgets(input, sizeof(input), stdin);       // sizeof 自动计算
```

**好处：**
- 想改大小？只改一处（`#define` 那行），所有地方自动更新
- `sizeof(input)` 替代硬编码数字，避免了数组大小和 fgets 参数不一致的 bug

---

## 难点 4：防御性编程 — 检查 fgets 的返回值

### 00 版本（脆弱）

```c
fgets(input, 100, stdin);         // 不管读没读到，直接继续
input[strlen(input) - 1] = '\0';  // 如果没读到任何东西，这里崩溃
```

### 01 版本（健壮）

```c
if (fgets(input, sizeof(input), stdin) == NULL) {
    break;   // EOF（Ctrl+D）或读取出错 → 退出 Shell
}
```

**什么时候 `fgets` 返回 `NULL`？**
- 用户按了 Ctrl+D（Unix）或 Ctrl+Z（Windows），表示"输入结束"
- 发生了 I/O 错误

不检查返回值的话，`input` 的内容未定义，后续 `strlen(input)` 会出问题。

**生活类比：** 就像外卖员——00 版本不管盒子是空的还是满的，直接打开吃；01 版本先检查盒子是不是空的。

---

## 难点 5：代码中的 echo 仍然有 bug

```c
} else if (strncmp(input, "echo", 4) == 0) {
    printf("%s\n", input+5);
```

### 问题

```c
// 输入: "echo  hello"  (echo 后有两个空格)
// input+5 指向第二个空格
// 输出: " hello"  (开头有空格的 hello)

// 输入: "echooops"  (以 echo 开头但不是 echo 命令)
// strncmp(input, "echo", 4) → 0  (误判为 echo 命令！)
```

`echo` 命令这两行代码本质上和 00 版本一样，用了硬编码的 `input+5`。**命令名 "echo" 是 4 字符，但偏移却是 `+5`**（假设后面跟一个空格）。这种不一致会在后续版本中彻底解决。

---

## 代码结构对比

### 00 版本：全部塞进 main

```
main() {
    循环 {
        判断exit
        判断echo    ← 混在一起，难读难改
        判断type    ← 30多行都在 main 里
        其他
    }
}
```

### 01 版本：抽取函数

```
is_builtins()           ← 独立函数：判断是否内置命令
skip_space()            ← 独立函数：跳过空格
main() {
    循环 {
        读取输入
        调用 is_builtins()    ← 干净清爽
        调用 skip_space()
        判断 exit/echo/type
    }
}
```

---

## 小结：这一版的核心进步

| 进步 | 具体技术 |
|------|---------|
| **代码复用** | `is_builtins()` 函数，type 命令无需重复写判断逻辑 |
| **内存效率** | 指针数组替代二维数组 |
| **通用性** | `skip_space()` 替代硬编码 `input+5` |
| **可维护性** | `#define MAX_INPUT` + `sizeof` 替代魔法数 |
| **健壮性** | `fgets` 返回值检查 |

这一版依然是**只支持内置命令**的小 Shell，但代码质量大大提升。下一步将给它加上识别外部命令的能力。
