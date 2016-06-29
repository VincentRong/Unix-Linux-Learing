#define TRUE 1
#include<unistd.h>
#include "run.h"
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>

char s[MAX_STR];

//redirect input to infile:
void redire_in(char *in, int head, int tail)
{   
    int len = tail - head + 1;
    char infile[len+1];
    strncpy(infile, in+head, len);
    infile[len] = '\0';
    
    char *words[MAX_PARA];
    int n;
    int *count = &n;
    split(infile, count, words);
    char *target = words[*count-1];

    int fd;
    fd = open(target, O_RDONLY);
    if(fd<0){
        printf("%s--open error\n", target);
        exit(1);
    }
    dup2(fd, STDIN_FILENO);
    close(fd);
}

//redirect output to outfile
void redire_out(char *in, int head, int tail)
{   
    int len = tail - head + 1;
    char outfile[len+1];
    strncpy(outfile, in+head, len);
    outfile[len] = '\0';

    char *words[MAX_PARA];
    int n;
    int *count = &n;
    split(outfile, count, words);
    char *target = words[*count-1];

    int fd;
    fd = open(target, O_WRONLY);
    if(fd<0){
        printf("%s--open error\n", target);
        exit(1);
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);
}

//execute the command
int execute(char toexec_command[])
{
    int pid;
    int fd[2];
    if (pipe(fd) < 0)
        printf("pipe error\n");

    char *words[MAX_PARA];
    int n;
    int *count = &n;
    split(toexec_command, count, words); //split the commands
    if((pid = fork()) < 0)
    {
        printf("%s wrong!\n", words[0]);
    }
    if(pid == -1) exit(1);
    if(pid == 0){   //child
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);  //redirect the write port to STDOUT
        //close(fd[1]);   //needed??????????
        
        printf("Ready to execute %s !\n", toexec_command);
        
        words[*count] = '\0';
        execvp(words[0], words);
        printf("%s--Could not execute!\n",words[0]);
        exit(1);
    }
    close(fd[1]);
    dup2(fd[0], STDIN_FILENO);
    //close(fd[0]); // needed???
    /*
        else{
        while(waitpid(pid,&status,0) != pid);
        //back up the STDIN_FILENO
        dup2(stdin_backup, STDIN_FILENO);
        dup2(stdout_backup, STDIN_FILENO);
        //printf("child process end.\n");
    }
    */
    return pid; 
}

//execute the last comman without pipe
int execute_last(char toexec_command[], int stdout_backup)
{
    int pid;
    char *words[MAX_PARA];
    int n;
    int j;
    int *count = &n;
    split(toexec_command, count, words); //split the commands
    if((pid = fork()) < 0)
    {
        printf("%s wrong!\n", words[0]);
    }
    if(pid == -1) exit(1);
    if(pid == 0){   //child
        dup2(stdout_backup, STDOUT_FILENO);
        
        words[*count] = '\0';
        execvp(words[0], words);
        printf("%s--Could not execute in exec-last!\n",words[0]);

        //DEBUG strange
        //for(j = 0; j < 1; j++)
        //{
            //printf("%d para is %s \n", j, words[j]);
        //}
        exit(1);
    }
    //close(fd[0]); // needed???
    /*
        else{
        while(waitpid(pid,&status,0) != pid);
        //back up the STDIN_FILENO
        dup2(stdin_backup, STDIN_FILENO);
        dup2(stdout_backup, STDIN_FILENO);
        //printf("child process end.\n");
    }
    */
    return pid; 
}


void run()
{
    int i;
	char *parameters;
    char *in;
    int segnum;
    int *seg = &segnum;
    	
    int current_child;
    int status;
    char *commands[MAX_SEG+1];
    int channel_pos[MAX_SEG];
    
    int stdin_backup = dup(STDIN_FILENO);
	int stdout_backup = dup(STDOUT_FILENO);

    printf("\nCOMMAND>");
	in = fgets(s,MAX_STR,stdin);
    in[strlen(in)-1] = '\0';  // '\n' -> '\0'
   
    findchannels(in, seg, channel_pos);
    int cur_pos;
    int next_pos;

    //first command:
    char toexec_command[MAX_STR];
    cur_pos = 0;
    if(*seg == 0) next_pos = strlen(in);
    else next_pos = channel_pos[0];
    cur_pos = -1;
    int toexec_len = next_pos - cur_pos - 1;
    strncpy(toexec_command, in+cur_pos+1 , toexec_len); //to standarize
    toexec_command[toexec_len] = '\0';
    
    for(i = 0; i < *seg; i++)
    {
        if(i == *seg-1){
            next_pos = strlen(in); //point to the '\0'
        }
        else{
            next_pos = channel_pos[i+1];
        }
        cur_pos = channel_pos[i];

        //test what the channel is
        if(in[cur_pos] == '<')
        {
            redire_in(in, cur_pos+1, next_pos-1);
        }
        else if(in[cur_pos] == '>')
        {
            redire_out(in, cur_pos+1, next_pos-1);
        }
        else if(in[cur_pos] == '|'){
            //needed to wait the former process???? Yes!!
            current_child = execute(toexec_command);
            int cur_child_status;
            while(waitpid(current_child, &cur_child_status, 0) != current_child);
            int newcommand_len = next_pos - cur_pos - 1;
            strncpy(toexec_command, in+cur_pos+1, newcommand_len);
            toexec_command[newcommand_len] = '\0';
        }
    }

    int next_child = execute_last(toexec_command, stdout_backup);
    
    while(waitpid(next_child, &status,0) != next_child);
    //back up the STDIN_FILENO
    dup2(stdin_backup, STDIN_FILENO);
    dup2(stdout_backup, STDIN_FILENO);

}

int main()
{
    while(1){
        run();
    }
    return 0;
}
