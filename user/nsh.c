#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
 
// shell的执行过程
// 1.获取终端输入 getcmd
// 2.解析输入 parsecmd
// 3.创建一个子进程，子进程进行程序替换 runcmd; 若有管道指令，则执行pipecmd
// 4.父进程等待子进程运行完毕
 
#define MAXARGS 10
#define MAXWORD 30
#define MAXLINE 100
 
char whitespace[] = " \t\r\n\v";
char args[MAXARGS][MAXWORD];

int getcmd(char *buf, int nbuf)
{
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0)
        return -1;
    return 0;
}
 
void parsecmd(char *cmd, char* argv[],int* argc)
{
    for(int i=0;i<MAXARGS;i++)
        argv[i]=&args[i][0];
    int i = 0; 
    int j = 0;
    for (; cmd[j] != '\n' && cmd[j] != '\0'; j++)
    {
        while (strchr(whitespace,cmd[j])) j++;
        argv[i++]=cmd+j;
        while (strchr(whitespace,cmd[j])==0) j++;
        cmd[j]='\0';
    }
    argv[i]=0;
    *argc=i;
}

void pipecmd(char*argv[],int argc);

void runcmd(char*argv[],int argc)
{
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"|"))
            pipecmd(argv,argc);
    }
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],">")){
            close(1);
            open(argv[i+1],O_CREATE|O_WRONLY);
            argv[i]=0;
        }
        if(!strcmp(argv[i],"<")){
            close(0);
            open(argv[i+1],O_RDONLY);
            argv[i]=0;
        }
    }
    exec(argv[0], argv);
}
 
void pipecmd(char*argv[],int argc){
    int i=0;
    for(;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            argv[i]=0;
            break;
        }
    }
    int fd[2];
    pipe(fd);
    if(fork()==0){
        close(1);
        dup(fd[1]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv,i);
    }else{
        close(0);
        dup(fd[0]);
        close(fd[0]);
        close(fd[1]);
        runcmd(argv+i+1,argc-i-1);
    }
}
int main()
{
    char buf[MAXLINE];
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
 
        if (fork() == 0)
        {
            char* argv[MAXARGS];
            int argc=-1;
            parsecmd(buf, argv,&argc);
            runcmd(argv,argc);
        }
        wait(0);
    }
 
    exit(0);
}