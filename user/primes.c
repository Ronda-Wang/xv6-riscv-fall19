#include "kernel/types.h"
#include "user/user.h"
//用0或1代替原本的管道端口号
void closegd(int k, int gd[]) 
{
  close(k);
  //复制句柄
  dup(gd[k]);
  close(gd[0]);
  close(gd[1]);
}

void go() 
{
  int gd[2];
  int p,n;
  //从管道读出数据
  if (read(0, &p, sizeof(p)))
  {
    //第一个数据直接输出
    printf("prime %d\n", p);
    //生成新管道
    pipe(gd);
    //
    if (fork()) 
    {
      closegd(0, gd);
      go();
    } 
    else 
    {
      //令写端端口号为1
      closegd(1, gd);
      //不能被管道第一个读出的数整除的数都扔掉
      while (read(0, &n, sizeof(n))) 
      {
        if (n % p != 0) 
        {
            write(1, &n, sizeof(n));
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {

  int gd[2],i;
  pipe(gd);

  if (fork()) 
  {
    //读端默认为0
    closegd(0, gd);
    go();
  } 
  else 
  {
    //写端默认为1
    closegd(1, gd);
    for (i = 2; i < 36; i++) 
    {
        write(1, &i, sizeof(i));
    }
  }
  exit();
}