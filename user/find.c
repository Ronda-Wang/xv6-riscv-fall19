#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//给路径，逆向查询最后一个/ 后面的文件跟我们想要的是不是同一个
int wanted_file(char *path, char *name)
{
  char *p;
  for(p=path+strlen(path); p >= path && *p != '/'; p--) ;
  p++;
  if(strcmp(p, name) == 0) return 1; else return 0;
}

void find(char* path, char* name){
  char buf[64], *p;
  int fd;
  struct dirent de;
  struct stat st;
  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  //本路径对应的文件的类型：文件/文件夹
  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  switch(st.type){
    //是文件的话直接查询是否为所找的
  case T_FILE:
    if (wanted_file(path, name))
        printf("%s\n", path);
    break;
  //是文件夹
  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    //de类似于本路径目录下的所有目录/文件
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      //补充路径
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, name);
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[])
{
  if(argc < 3){
    exit();
  }
  find(argv[1], argv[2]);
  exit();
}