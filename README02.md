# 第三步：PATH 环境变量 — 让 Shell 认识外部命令

> 对应代码：`02-main.c`
> 前置知识：[README01.md](README01.md) — 指针数组、skip_space、宏定义
> 难度：★★★☆☆
> 新概念：环境变量 getenv、strtok 分割、static 缓冲区、strdup、access、snprintf

---

## 从 01 到 02 的进化

| 方面 | 01-main.c | 02-main.c | 新增什么 |
|------|-----------|-----------|---------|
| 头文件 | `stdio, stdlib, string` | 新增 `unistd.h, sys/stat.h` | POSIX 系统调用 |
| PATH 查找 | 没有 | `find_in_path()` 函数 | 环境变量解析 |
| type 命令 | 只识别内置命令 | 能显示外部命令路径 | `type ls` → `/bin/ls` |
| 文本解析 | 仍用 `input+5` | 用 while 循环手动解析 | 通用的命令/参数分离 |
| 换行处理 | `input[strlen-1]='\0'` | 先判断再替换 | 更安全 |

**最重要的一点：** 这一版虽然还没让外部命令真正执行，但 Shell 已经能"认识"外部命令的存在了——你能看到 `ls` 在 `/bin` 目录下。

---

## 难点 1：getenv — 读取环境变量

```c
char* path_env = getenv("PATH");
```

### 什么是环境变量？

环境变量是操作系统给每个进程的"配置字典"。打开终端，输入：

```bash
$ echo $PATH
/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
```

PATH 告诉 Shell："当用户输入一个命令时，去这些目录里找可执行文件。"

### getenv 做了什么？

```c
char* path = getenv("PATH");
// path 现在指向字符串："/usr/local/bin:/usr/bin:/bin"
// ↑ 注意：这是只读内存！不能修改它！
```

**生活类比：** 环境变量就像你手机里的"默认应用设置"——点击一个链接，系统会根据设置自动选择用哪个浏览器打开。PATH 就是告诉系统"去哪找可执行文件"的配置。

---

## 难点 2：strtok — 按 `:` 分割 PATH 字符串

这是本步**最核心的技术难点**。

```c
char* path_copy = strdup(path_env);  // 1. 先复制一份
char* dir = strtok(path_copy, ":");  // 2. 取出第一个目录

while (dir != NULL) {
    // ... 拼接路径、检查文件 ...
    dir = strtok(NULL, ":");          // 3. 继续取下一个
}
```

### strtok 的完整工作流程

假设 `PATH=/usr/bin:/bin:/usr/local/bin`：

```
第一步: strtok(path_copy, ":")
┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
│ / │ u │ s │ r │ / │ b │ i │ n │\0 │ / │ b │ i │ n │ / │ u │ s │ r │ ...  │\0 │
└───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
                                  ↑
                     冒号被替换为 \0（字符串结束符）
 返回指针 → "/usr/bin"（到第一个 \0 为止）


第二步: strtok(NULL, ":")
                                从这里继续→  ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
                                             │ / │ b │ i │ n │\0  │ / │ u │ s │ r │ ...│
                                             └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
                                                                   ↑
                                                         再替换冒号为 \0
 返回指针 → "/bin"


第三步: strtok(NULL, ":")
                                                                              从这里→ ...
 返回指针 → "/usr/local/bin"

第四步: strtok(NULL, ":")
 没有更多内容了 → 返回 NULL
```

### 三个关键细节

**细节 1：必须用 strdup 复制**

```c
char* path_copy = strdup(path_env);  // ← 不能直接用 getenv 的返回值！
```

`getenv` 返回的字符串存在只读区域，`strtok` 会修改它（把 `:` 改成 `\0`）。**直接传会导致段错误（Segmentation Fault）崩溃。**

`strdup` = "string duplicate"，做了两步：
1. `malloc(strlen(str) + 1)` 分配新内存
2. `strcpy` 复制内容

**细节 2：第二次起传 NULL**

```c
dir = strtok(path_copy, ":");   // 首次：给原字符串
dir = strtok(NULL, ":");        // 后续：给 NULL
```

`strtok` 内部用了一个**静态变量**记住上次处理到哪。传 `NULL` = "继续从上次停下的地方往后走"。

**细节 3：使用完必须 free**

```c
free(path_copy);  // strdup 分配的内存必须手动释放！
```

---

## 难点 3：snprintf — 安全地拼接路径

```c
snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
//         ↑           ↑               ↑        ↑    ↑
//      目标缓冲区   缓冲区大小        格式串   目录 命令名
```

### 为什么用 snprintf 而不是 sprintf？

```c
// ❌ sprintf — 危险！不检查缓冲区大小
sprintf(full_path, "%s/%s", dir, cmd);
// 如果 dir 很长，可能溢出 full_path → 缓冲区溢出漏洞

// ✅ snprintf — 安全！最多写 sizeof(full_path)-1 个字符
snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
// 超出则截断，永远以 \0 结尾
```

### 拼接结果举例

```c
// dir = "/usr/bin", cmd = "ls"
snprintf(full_path, ..., "%s/%s", dir, cmd);
// full_path = "/usr/bin/ls"

// dir = "/usr/bin/", cmd = "ls"
snprintf(full_path, ..., "%s%s", dir, cmd);  // 不用加 /
// full_path = "/usr/bin/ls"  (不产生双斜杠)
```

代码中有一个判断，避免路径中出现双斜杠：

```c
if (dir[dir_len - 1] != '/') {
    snprintf(full_path, ..., "%s/%s", dir, cmd);   // 加 /
} else {
    snprintf(full_path, ..., "%s%s", dir, cmd);     // 不加 /
}
```

---

## 难点 4：access + X_OK — 检查文件是否可执行

```c
if (access(full_path, X_OK) == 0) {
    return full_path;  // 文件存在且可执行
}
```

### access 函数的含义

| 返回值 | 含义 |
|--------|------|
| `0` | 文件存在且符合检查条件 |
| `-1` | 文件不存在或无权限 |

### X_OK 中 X 的含义

```
R_OK = 4  → Read   (可读)      chmod +r
W_OK = 2  → Write  (可写)      chmod +w
X_OK = 1  → eXecute(可执行)    chmod +x
```

**为什么检查 X_OK 而不是 F_OK（仅检查存在）？**

```bash
$ ls -l /bin/ls
-rwxr-xr-x  1 root  wheel  138,288  /bin/ls
#  ^
# x = 可执行位

$ ls -l /etc/passwd
-rw-r--r--  1 root  wheel  2,345  /etc/passwd
#   ^
# 没有 x = 不可执行
```

`/etc/passwd` 存在但在 Shell 里不能作为命令运行。`X_OK` 正确地区分了"普通文件"和"可执行文件"。

---

## 难点 5：手动文本解析 — 分离命令名和参数

02 版本还没有 `extract_word()` 函数，在 main 函数里用 while 循环手动完成：

```c
// 提取命令名
char cmd_name[MAX_INPUT];
int i = 0;
while (cmd[i] != '\0' && cmd[i] != ' ') {
    cmd_name[i] = cmd[i];
    i++;
}
cmd_name[i] = '\0';  // 别忘了加结束符！

// 获取参数字符串
char* args = skip_space(cmd + i);
```

### 图解解析过程

```
输入: "echo  hello world"

cmd 指向 → ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
           │ e │ c │ h │ o │   │   │ h │ e │ l │ l │ o │   │ w │...│
           └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
            i=0 i=1 i=2 i=3 i=4   ← i=4 时遇到空格，循环退出

cmd_name → ['e','c','h','o','\0', ...]

cmd + i = cmd + 4 → 指向第一个空格
skip_space(cmd+4) → 跳过两个空格，指向 'h'

args → 指向 "hello world"
```

### 这种写法的局限

`type` 命令也有几乎相同的 while 循环代码——这是**代码重复**。04 版本会用 `extract_word()` 函数来消除这个重复。

---

## 对比：01 → 02 type 命令的变化

### 01 版本（只能识别内置命令）

```c
} else if (strncmp(input, "type", 4) == 0) {
    char* arg = skip_space(input+5);
    if (is_builtins(arg)) {
        printf("%s is a shell builtin\n", arg);
    } else {
        printf("%s: not found\n", arg);  // ← 所有外部命令都显示 not found
    }
}
```

### 02 版本（能查找外部命令路径）

```c
} else if (strcmp(cmd_name, "type") == 0) {
    // ... 提取 type_arg ...
    if (is_builtins(type_arg)) {
        printf("%s is a shell builtin\n", type_arg);
    } else {
        char* exec_path = find_in_path(type_arg);    // ← 新增！
        if (exec_path != NULL) {
            printf("%s is %s\n", type_arg, exec_path);  // 显示路径
        } else {
            printf("%s: not found\n", type_arg);
        }
    }
}
```

**运行效果：**

```bash
$ type echo
echo is a shell builtin

$ type ls
ls is /bin/ls        ← 02 版本的新能力！

$ type python3
python3: not found   ← PATH 里没有
```

---

## 当前代码的局限（为 03 版本铺垫）

虽然 `type ls` 知道 `/bin/ls` 在哪，但直接输入 `ls` 还是不行：

```bash
$ ls
ls: command not found    ← 还没实现 fork/exec！
```

02 版本只是一个"能查找但不能执行"的 Shell。**下一版本（03）将补上最关键的能力——fork + execvp 真正运行外部命令。**

---

## 小结：这一版的核心进步

| 进步 | 具体技术 | 你可以做什么新的事情 |
|------|---------|---------------------|
| **环境变量读取** | `getenv("PATH")` | 知道系统在哪些目录查找命令 |
| **字符串分割** | `strtok` | 把 PATH 拆成一个个目录 |
| **安全字符串拼接** | `snprintf` | 拼出完整路径如 `/bin/ls` |
| **文件权限检查** | `access(..., X_OK)` | 区分可执行文件和普通文件 |
| **动态内存管理** | `strdup` + `free` | 分配临时字符串并正确释放 |

**记忆口诀：**
- `getenv` 拿 PATH → `strdup` 复制一份 → `strtok` 逐个切目录 → `snprintf` 拼路径 → `access` 检查存在 → `free` 释放内存
