#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_INPUT 4096

//对内置函数进行判断，这里使用指针数组
//原来使用的是二维数组，但是二维数组不能更简洁做循环判断，需要两个参数，而指针数组只需要一个参数
int is_builtins(const char* cmd){
  //define arry save builtins types
  char *builtins[] = {"echo", "type", "exit", "pwd", "cd", NULL};
  //这里结尾必须是NULL，否则下面的for循环会越界的
  //char *builtins[] = {"echo", "type", "exit"};
  int i = 0;

  for( i = 0; builtins[i] != NULL; i++){
    if(strcmp(cmd, builtins[i]) == 0){
      return 1;
    }
  }
  return 0;
}

//从PATH环境变量中查找可执行文件
//如果找到，返回完整路径(静态缓冲区),否则返回NULL
char* find_in_path(const char* cmd){
  static char full_path[MAX_INPUT];
  char* path_env = getenv("PATH");

  if( path_env == NULL || cmd == NULL || cmd[0] == '\0' ){
    return NULL;
  }
  
  char* path_copy = strdup(path_env);
  if(path_copy == NULL){
    return NULL;
  }
  
  char* dir = strtok(path_copy, ":");
  
  while(dir != NULL){
    size_t dir_len = strlen(dir);
    if(dir_len == 0){
      dir = strtok(NULL, ":");
      continue;
    }
    if(dir[dir_len - 1] != '/'){  
      snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
    }else{ 
      snprintf(full_path, sizeof(full_path), "%s%s", dir, cmd);
    }
 
    
    if(access(full_path, X_OK) == 0){
      free(path_copy);
      return full_path;
    }

    dir = strtok(NULL, ":");
  }
  free(path_copy);
  return NULL;
}

//辅助函数，用与跳过字符串开头的空格，返回有效命令的起始指针
char* skip_space(const char* str){
  while(*str != '\0' && *str == ' '){
    str++;
  }
  return (char*)str;
}

//提取命令以及参数列表
void extract_word(const char* src, char* dest, const char** rest){
  int i = 0;
  while(src[i] != '\0' && src[i] != ' '){
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';

  if(rest != NULL){
    * rest = skip_space(src + i);
  }
}

void exec_command(const char* cmd_name, char* args_str){
  char* exec_path = find_in_path(cmd_name);
  if(exec_path == NULL){
    printf("%s: command not found\n", cmd_name);
    return;
  }
  
  char* argv[MAX_INPUT];
  int argc = 0;
  argv[argc++] = (char*)cmd_name;
  char* args_copy = NULL;

  if(args_str != NULL && *args_str != '\0'){
    args_copy = strdup(args_str);
    char* token = strtok(args_copy, " ");
    while(token != NULL && argc < MAX_INPUT - 1){
      argv[argc++] = token;
      token = strtok(NULL, " ");
    }
  }
  
  argv[argc] = NULL;

  pid_t pid = fork();
  if(pid < 0){
    perror("fork");
    return;
  }
 
  if( pid == 0){
    execvp(exec_path, argv);
    perror("execvp");    
    return;
  }else{
    int status;
    waitpid(pid, &status, 0);
  }

  free(args_copy);

}

//=================cd命令执行=============
void handle_cd(const char* path){
  const char* home = getenv("HOME");
  char* home_path = skip_space(path);
  if(path == NULL && *path == '\0'){
    if(home != NULL){
      if(chdir(home) != 0){
        printf("cd: %s: No such file or directory\n", home);
      }
    }
    
    return;
  }

  if(strcmp(home_path, "~") == 0){
    chdir(home);
    return;
  }    
  if(chdir(path) != 0){
    printf("cd: %s: No such file or directory\n", path);
  }
}


int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  //char* path_env = getenv("PATH");
  //printf("%s", path_env);
  while(1) {
  // TODO: Uncomment the code below to pass the first stage
   printf("$ ");

  //wite your input
  char input[MAX_INPUT];
  //读取标准输入的内容并赋值给input
  if(fgets(input, sizeof(input), stdin) == NULL){
    break;
  }
 
  //去掉末尾换行符
  size_t len = strlen(input);
  if( len > 0 && input[len - 1] == '\n'){
    input[len - 1] = '\0';
  }

  char* cmd = skip_space(input);
  if(*cmd == '\0'){
    continue;
  }
  
  char cmd_name[MAX_INPUT];
  const char* rest;

  extract_word(cmd, cmd_name, &rest);
  //int i = 0;
  //while(cmd[i] != '\0' && cmd[i] != ' '){
  //  cmd_name[i] = cmd[i];
  //  i++;
  //}
  //cmd_name[i] = '\0';
  //char* args = skip_space(cmd + i);
  char* args = skip_space(rest);

  if(strcmp(cmd_name, "exit") == 0){
    break;
  }else if(strcmp(cmd_name, "echo") == 0){
    printf("%s\n", rest);
  }else if(strcmp(cmd_name, "pwd") == 0){
    printf("%s\n", getcwd(NULL, 0));
  }else if(strcmp(cmd_name, "type") == 0){
    if(*args == '\0'){
      continue;
    }
    
    char type_arg[MAX_INPUT];
    const char* type_rest;
    extract_word(args, type_arg, &type_rest);
    //int j = 0;
    //while(args[j] != '\0' && args[j] != ' '){
    //  type_arg[j] = args[j];
    //  j++;
    //}
    //type_arg[j] = '\0';

    if(is_builtins(type_arg)){
      printf("%s is a shell builtin\n",type_arg);
    }else{
      char* exec_path = find_in_path(type_arg);
      if(exec_path != NULL){
        printf("%s is %s\n", type_arg, exec_path);
      }else{
        printf("%s: not found\n",type_arg);
      }
    }

  }else if(strcmp(cmd_name, "cd") == 0){
    char cd_arg[MAX_INPUT];
    const char* cd_rest;
    extract_word(args, cd_arg, &cd_rest);
    handle_cd(cd_arg);
    
  }else{ 
    //printf("%s: command not found\n", cmd_name);
    exec_command(cmd_name,args);
  }
  }
  return 0;
}
