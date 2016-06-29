/*
 *  webserver.c
 */
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<errno.h>

#define MAX_BUFF_SIZE 100
#define EOL "\r\n"
#define EOL_SIZE 2

int portno;
char webroot[100];

typedef struct{
    char *ext;
    char *mediatype;
}ext_dict;

ext_dict extensions[] = {
    {"gif", "image/gif"},
    {"txt", "text/plain"},
    {"jpg", "image/jpg"},
    {"png", "image/png"},
    {"ico", "image/ico"},
    {"html", "text/html"},
    {"sh", "shell/sh"},  //可执行
    {0,0} //用作哨兵
};

int dict_size = (sizeof(extensions)) / (sizeof(extensions[0]));

/*
 * error output function
 */
void error(const char *msg)
{
    perror(msg);
}

/*
 * error output and exit! Dealing with fatal error.
 */
void error_exit(const char *msg)
{
    perror(msg);
    exit(1);
}

/*
 * send message to sockfd
 */
void send_msg(int sockfd, char *msg)
{
    int len = strlen(msg);
    if(send(sockfd, msg, len, 0) == -1){
        error("Error in sending!!\n");
    }
}

/*
 * receive message and store in the buffer, end if encounter the EOL
 */
int recv_msg(int sockfd, char *buf)
{
    char *p = buf;
    int n;
    int match_flag = 0;
    while((n = recv(sockfd, p, 1, 0)) > 0){   //receive and check one character at one time
        if(*p == EOL[match_flag])
        {
            match_flag++; //这个状态记录参考别的作者代码的，很不错
            if(match_flag == EOL_SIZE)   //连续匹配两个EOL字符，终结该字符串读取
            {
                *(p - EOL_SIZE + 1) = '\0';
                return(strlen(buf));  
            }
        }
        else{
            match_flag = 0;
        }
        p++;
    }
    return(0);  //receive nothing
}


/*
 * get config
 */
void get_conf()
{
    FILE *in = fopen("myhttpd.conf","rt");
    char buf[1000];
    fread(buf, sizeof(char),  1000, in);
    fclose(in);

    const char *sep = "\n=";
    char *p;
    p = strtok(buf, sep);
    while(p){   //read the corresponding items
        if(strcmp(p, "Port") == 0)
        {
            p = strtok(NULL, sep);
            portno = atoi(p);
        }
        else if(strcmp(p, "Directory") == 0)
        {
            p = strtok(NULL, sep);
            strcpy(webroot, p);
        }
        p = strtok(NULL, sep);
    }
}


/*
 * connect the fd and parse message from the fd
 */
int communicate(int sockfd)
{
    char request[300], resource[300], *ptr;
    int filefd, length;
    int recv_length = recv_msg(sockfd, request);
    if(recv_length == 0){
        error("Error:receive Nothing!!\n");
    }

    //处理http头格式
    ptr = strstr(request, " HTTP/"); //查找最开始出现的位置
    if(ptr == NULL){
        error("No HTTP!\n");
    }
    else{
        *ptr = 0;
        ptr = NULL;
        if(strncmp(request, "GET ", 4) == 0){
            ptr = request + 4;  //跳过"GET "字符串
        }
        if(ptr == NULL){
            error("Not a GET request!\n");
        }
        else{
            if(ptr[strlen(ptr) - 1] == '/'){
                strcat(ptr, "fuck.html");
            }
            strcpy(resource, webroot);
            strcat(resource, ptr); //组成完整目录路径
            char *s = strchr(ptr, '.');
            int i;
            for(i = 0; extensions[i].ext != NULL; i++){
                if(strcmp(extensions[i].ext, s+1) == 0){
                    //find the type!
                    struct stat fileinfo;
                    printf("resource is %s\n",resource);
                    if(stat(resource, &fileinfo) < 0)
                    {
                        error("Stat Failed!!\n");
                        break;
                    }
                    if((fileinfo.st_mode & S_IXUSR) || (fileinfo.st_mode & S_IXGRP))
                    { //可执行文件
                    
                        printf("Get into executable stage.\n");
                       
                        send_msg(sockfd, "HTTP/1.1 200 OK\r\n");
                        int exe_pid = fork();
                        if(exe_pid < 0)
                        {
                            error("Fork Failed!!\n");
                            break;
                        }
                        if(exe_pid == 0)
                        {
                            printf("dup2?\n");
                            dup2(sockfd, STDOUT_FILENO);
                            //close(sockfd);
                            //no parameters, MAYBE WRONG!!!?
                            execl(resource, resource, (char *)0);
                            exit(1);
                        }
                        else
                        {
                            sleep(2);
                            break;
                        }
                    }
                    else  //不可执行文件
                    {
                        printf("Get into the non-executabel stage\n");
                        filefd = open(resource, O_RDONLY, 0);
                        send_msg(sockfd, "HTTP/1.1 200 OK\r\n");
                        char contenttype[100];
                        strcpy(contenttype, "Content-Type: ");
                        strcat(contenttype, extensions[i].mediatype);
                        strcat(contenttype, "\r\n\r\n");
                        send_msg(sockfd, contenttype);
                        if(ptr == request + 4)  //满足上面那个GET的条件
                        {
                            struct stat stat_struct;
                            if(fstat(filefd, &stat_struct) == -1)
                                error("Error: get the stat failed!\n");
                            length = (int)stat_struct.st_size;    
                            size_t bytes_had_sent = 0;
                            ssize_t bytes_just_sent;
                            while(bytes_had_sent < length){
                                if((bytes_just_sent = sendfile(sockfd, filefd, 0, length - bytes_had_sent)) <= 0)
                                {
                                    if(errno = EINTR || errno == EAGAIN)  //errno没有出错时可能保持不变状态。
                                    {
                                        continue;
                                    }
                                    error_exit("Error: sendfile. Not the EINTR or EAGAIN");
                                }
                                bytes_had_sent += bytes_just_sent;
                            }//end while
                        } //end of if(ptr == request + 4)
                    }
                    close(filefd);
                    break;
                } //end if strcmp()

                //if not match with the listed file type:
                if(i == dict_size-2)
                {
                    printf("415 Unsupported Media Type\n");
                    send_msg(sockfd, "HTTP/1.11 415 Unsuported Media Type\r\n");
                    send_msg(sockfd, "<html><head><title>415 Unsupported Media Type</head></title>");
                    send_msg(sockfd, "<body><p>415 Unsupported Media Type!</t></body><html>");
                }
            }   // end for()
        }
    }
    printf("Shutdown sockfd\nEnd of a connection!---------\n");
    shutdown(sockfd, SHUT_RDWR);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, pid;
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        error("Error in opening socket");
    bzero((char *)&server_addr, sizeof(server_addr));
    get_conf(); //获取各种配置信息
    printf("webroot is %s\n", webroot);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr); //点分式转换成字符串形式
    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        error("Error when binding!\n");
    listen(sockfd, 10);
    client_len = sizeof(client_addr);
    
    /*
     * Main process of Server continue listening
     * Another one process deal with the connection
     */
    while(1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if(newsockfd < 0)
            error("Error when accepting!\n");
        pid = fork();
        if(pid < 0)
            error("Error when forking!\n");
        if(pid == 0)
        {
            close(sockfd);
            communicate(newsockfd);
            exit(1);
        }
        else
        {
            //sleep(2);   //?sleeping can help the running of multi-process more clear
            close(newsockfd);
        }
    } //end of while(1)
    close(sockfd); 
    return(0);
}
