#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  while(1) {
  // TODO: Uncomment the code below to pass the first stage
   printf("$ ");

  //wite your input
  char input[100];
  fgets(input, 100, stdin);
  int i,flage=0;
  
  //define arry save builtins types
  char builtins[][100] = {"echo", "type", "exit"};
  int len = sizeof(builtins)/sizeof(builtins[0]);

  //remove the trailing newline
  input[strlen(input) - 1] = '\0';

  if(strcmp(input, "exit") == 0){
    break;
  }else if(strncmp(input, "echo ", 5) == 0){
    printf("%s\n", input+5);
  }else if(strncmp(input, "type", 4) == 0){
    for( i = 0; i < len; i++){
      if(strcmp(input+5, builtins[i]) == 0){
        printf("%s is a shell builtin\n", input+5);
        flage = 1; 
        break;
      }
    }
    if(flage != 1){
      printf("%s: not found\n", input+5);
      flage = 0;
    }
  }else{ 
    printf("%s: command not found\n", input);
  }

  }
  return 0;
}
