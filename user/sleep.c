#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int x = atoi(argv[1]);

  if (argc != 2)
    write(2, "Error", strlen("Error")); //标准错误输出文件为2

  sleep(x);
  write(1,"nothing happens for a little while",strlen("nothing happens for a little while\n"));
  exit();
}