#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// echo：用于字符串的输出

// grep：常用于搜索文件中是否含有某些特定模式的字符串，该命令以行为单位读取文本并使用正则表达式进行匹配，匹配成功后打印出该行文本。

// cat：常用来显示文件内容，或者将几个文件连接起来显示，或者从标准输入读取内容并显示，它常与重定向符号配合使用
// cat filename：一次显示整个文件
// cat > filename 从键盘创建一个文件，只能创建新文件，不能编辑已有文件

// wc：统计指定文件中的字节数、字数、行数，并将统计结果显示输出
 
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

//获取用户输入
int getcmd(char *buf, int nbuf)
{
    //测试用例testsh重定向了shell的标准输出，所以实现的shell应该使用fprintf(2，…)
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    //获得缓冲区内容
    gets(buf, nbuf);
    if (buf[0] == 0)
        return -1;
    return 0;
}

//解析输入
void parsecmd(char *cmd, char* argv[],int* argc)
{
    for(int i=0;i<MAXARGS;i++)
        argv[i]=&args[i][0];
    int i = 0; 
    int j = 0;
    for (; cmd[j] != '\n' && cmd[j] != '\0'; j++)
    {
        //去掉空格
        while (strchr(whitespace,cmd[j])) j++;
        //空格后面的字符串收进argv
        argv[i++]=cmd+j;
        //跨过字符串
        while (strchr(whitespace,cmd[j])==0) j++;
        //字符串后面的空格转成'\0'
        cmd[j]='\0';
    }
    argv[i]=0;
    *argc=i;
}

void pipecmd(char*argv[],int argc);

void runcmd(char*argv[],int argc)
{
    //如果是管道命令
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"|"))
            pipecmd(argv,argc);
    }
    for(int i=1;i<argc;i++){
        //输出重定向 命令>文件  将命令执行的标准输出结果重定向输出到指定的文件中
        if(!strcmp(argv[i],">")){
            close(1);//关闭文件描述符1，从此agrv[i+1]指向的文件的文件描述符变为1
            open(argv[i+1],O_CREATE|O_WRONLY);//若文件存在，打开文件；不存在则创建
            argv[i]=0;
        }
        //输入重定向 命令 < 文件  将指定文件作为命令的输入设备
        if(!strcmp(argv[i],"<")){
            //新分配的文件描述符总是当前进程未使用的最小的那个。
            close(0);//关闭文件描述符0，从此agrv[i+1]指向的文件的文件描述符变为0
            open(argv[i+1],O_RDONLY);//只读打开argv[i+1]指向的文件，文件描述符为0
            argv[i]=0;//argv[i]记为空，前面的命令基于文件描述符0代表的文件执行
        }
    }
    exec(argv[0], argv);//执行命令
}
//管道就是 用"|"符号来连接两个命令，以前面命令的标准输出作为后面命令的标准输入 
void pipecmd(char*argv[],int argc){
    int i=0;
    for(;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            argv[i]=0;//分隔开两个管道命令
            break;
        }
    }
    int fd[2];
    pipe(fd);
    if(fork()==0){
        close(1);
        dup(fd[1]);//令子进程写入描述符为1
        close(fd[0]);//关闭子进程原有的读写描述符
        close(fd[1]);
        runcmd(argv,i);//子进程执行左侧指令
    }else{
        close(0);
        dup(fd[0]);//令父进程读出文件描述符为0
        close(fd[0]);//关闭父进程原有的读写描述符
        close(fd[1]);
        runcmd(argv+i+1,argc-i-1);//父进程执行右侧命令
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
            //解析指令
            parsecmd(buf, argv,&argc);
            //运行指令
            runcmd(argv,argc);
        }
        wait(0);
    }
 
    exit(0);
}