#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define MAXSIZE 1024
#define PORT 30000
typedef struct
{
    char loginid[20];  /*用户名*/
    char password[20]; /*密码*/
} users_t;

typedef struct
{
    int r_sockfd;
    //  int sender;
    char content[500];
} hdr_t;

void _chat(int);
int _login(int,users_t);
void mysystem(int);
void _register(int);

void mysystem(int fd)
{
    users_t user;
	char ch,in_ch,buf[MAXSIZE];
	int flag,ret;
    do
    {
        printf("*-----------------------------*\n");
	    printf("welcome to use Qchat!Please login or register!\n");
	    printf("1.login\n");
	    printf("2.register\n");
        printf("3.quit\n");
        printf("4.write off\n");
        printf("*-----------------------------*\n");
        scanf("\n%c",&ch);
        switch(ch)
        {
            case '1':
            {
                send(fd,"1",sizeof("1"),0);                 //login()

                /*获取用户输入*/
                printf("用户名：");
                memset(user.loginid, 0, sizeof(user.loginid));
                scanf("%s", user.loginid);

                printf("用户密码：");
                memset(user.password, 0, sizeof(user.password));
                scanf("%s", user.password);

                flag=_login(fd,user);
                printf("%d\n",flag);
                if(flag==1)
                {
                    do
                    {
                        printf("*-----------------------------*\n");
                        printf("Welcome,%s!now what do you want to do?\n",user.loginid);
                        printf("1.show users_online\n");
                        printf("2.chat\n");
                        printf("3.goback\n");
                        printf("*-----------------------------*\n");
                        scanf("\n%c",&in_ch);
                        switch(in_ch)
                        {
                            case '1':
                            {
                                send(fd,"3",sizeof("3"),0);              //online()
                                while((ret=read(fd,buf,sizeof(buf)))>0)
                                fputs(buf,stdout);
                                break;
                            }
                            case '2':
                            {
                                send(fd,"4",sizeof("4"),0);                 //chat()
                                _chat(fd);
                                break;
                            }
                            case '3':break;
                            default:
                            {
                                printf("Input wrong\n");
                                break;
                            }
                        }
                    }while(in_ch!='3');
					break;
                }
                else continue;
            }
            case '2':
            {
                send(fd,"2",sizeof("2"),0);                     //register
                _register(fd);
                break;
            }
            case '3':
            {
                send(fd,"6",sizeof("6"),0);
                break;
            }
            case '4':
            {
                send(fd,"5",sizeof("5"),0);                     //write_off
                printf("Account cancelled.\n");
                ch='3';
                break;
            }
            default:
            {
                printf("Input wrong\n");
                break;
            }
        }
    }while(ch!='3');
}

void main(int argc, char *argv[])
{
    int fd;
    pid_t pid;
    char buf[1024], buf_read[100];
    struct sockaddr_in servaddr;
    int portnumber, nbytes;

    /*Create a socket*/
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /*perror("Create sockets error!");*/
        fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
        exit(1);
    }

    /*Connect a service*/
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    //	servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    if (inet_aton("127.0.0.1", &servaddr.sin_addr) < 0)
    {
        perror("inet_aton error");
        exit(1);
    }
    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0)
    {
        perror("connect error");
        exit(1);
    }

    
    mysystem(fd); //change to "mysystem()" later

    /*结束通信*/
    close(fd);
    exit(0);
}

void _chat(int fd)
{
    pid_t pid;
    hdr_t hd;
    if ((pid = fork()) == -1)
    {
        perror("客户端fork失败\n");
        exit(1);
    }

    //子进程收数据
    else if (pid == 0)
    {
        char recvbuf[1024];
        bzero(recvbuf, sizeof(recvbuf));
        printf("chatting...");
        while(1)
        {
            puts("");
            int ret = read(fd, recvbuf, sizeof(recvbuf));
            if (ret == -1)
            {
                perror("客户端读取失败\n");
                exit(1);
            }
            else if (ret == 0)
            {
                printf("服务器关闭\n");
                break; //跳出循环不再接受消息
            }
            else
            {
                fputs(recvbuf, stdout);
            }
        }
        printf("客户端子进程退出\n");
        //close(fd);
        // kill(getppid(), SIGUSR1);        //向父进程发送信号
   //     exit(0);
    }

    //父进程发数据
    else
    {
        //  signal(SIGUSR1, handler);
        char sendbuf[MAXSIZE];
        bzero(sendbuf, sizeof(sendbuf));

        do
        {
            scanf("%d %s", &hd.r_sockfd, hd.content);

            memcpy(sendbuf, &hd, sizeof(hd));
            send(fd, sendbuf, sizeof(sendbuf), 0);
        } while (hd.r_sockfd);

        printf("客户端父进程退出\n");
    }
    kill(pid,SIGKILL);
   // close(fd);
}

void _register(int sockfd)
{
    char buf[MAXSIZE];

    //get new User`s ID
    printf("Input User_ID,please:");
    memset(buf, 0, MAXSIZE);
    scanf("%s", buf);
    if (!send(sockfd, buf, sizeof(buf), 0))
    {
        perror("send User_ID error\n");
    }

    //get new User`s password
    printf("Input User_Password,please:");
    memset(buf, 0, MAXSIZE);
    scanf("%s", buf);
    if (!send(sockfd, buf, sizeof(buf), 0))
        perror("send User_Password error\n");

    //get responsble from server
    memset(buf, 0, MAXSIZE);
    recv(sockfd, buf, sizeof(buf), 0);
    printf("%s\n", buf);
}

int _login(int sockfd,users_t user)
{
    int ret;
    /*声明用户登陆信息*/
    char buf[MAXSIZE];

    /*发送用户登陆信息到服务器*/
    memset(buf, 0, MAXSIZE);
    memcpy(buf, &user, sizeof(user)); //Is it OK?
    send(sockfd, buf, sizeof(buf), 0);

    //get responsble from server
    memset(buf, 0, MAXSIZE);
    recv(sockfd, buf, sizeof(buf), 0);
    printf("%s", buf);

    if(strncmp("login successfully!",buf,19)==0)
    return 1;
    else return 0;
}
