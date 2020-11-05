#include "kernel/types.h"
#include "user/user.h"

void closegd(int k, int gd[]) 
{
  close(k);
  dup(gd[k]);
  close(gd[0]);
  close(gd[1]);
}

void go() 
{
  int gd[2];
  int p,n;
  if (read(0, &p, sizeof(p)))
  {
    printf("prime %d\n", p);
    pipe(gd);
    if (fork()) 
    {
      closegd(0, gd);
      go();
    } 
    else 
    {
      closegd(1, gd);
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
    closegd(0, gd);
    go();
  } 
  else 
  {
    closegd(1, gd);
    for (i = 2; i < 36; i++) 
    {
        write(1, &i, sizeof(i));
    }
  }
  exit();
}