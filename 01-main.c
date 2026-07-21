#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 4096



//对内置函数进行判断，这里使用指针数组
//原来使用的是二维数组，但是二维数组不能更简洁做循环判断，需要两个参数，而指针数组只需要一个参数
int is_builtins(const char* cmd){
  //define arry save builtins types
  char *builtins[] = {"echo", "type", "exit", NULL};
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


//辅助函数，用与跳过字符串开头的空格，返回有效命令的起始指针
char* skip_space(const char* str){
  while(*str != '\0' && *str == ' '){
    str++;
  }
  return (char*)str;
}

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while(1) {
  // TODO: Uncomment the code below to pass the first stage
   printf("$ ");

  //wite your input
  char input[MAX_INPUT];
  //读取标准输入的内容并赋值给input
  if(fgets(input, sizeof(input), stdin) == NULL){
    break;
  }
 
  //remove the trailing newline
  input[strlen(input) - 1] = '\0';

  if(strcmp(input, "exit") == 0){
    break;
  }else if(strncmp(input, "echo", 4) == 0){
    printf("%s\n", input+5);
  }else if(strncmp(input, "type", 4) == 0){
    char* arg = skip_space(input+5);
    if(is_builtins(arg) == 1){
      printf("%s is a shell builtin\n",input+5);
    }else{
      printf("%s: not fount\n",input+5);
    }

  }else{ 
    printf("%s: command not found\n", input);
  }

  }
  return 0;
}
