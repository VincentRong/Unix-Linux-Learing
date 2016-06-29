#include<stdio.h>
#include<stdlib.h>
int main()
{
    char *s = "/home/vincentr/myserver/myscript.sh";
    int pid = fork();
    if(pid == 0){
        execl(s, s, (char *)0);    
    }
    return 0;
}
