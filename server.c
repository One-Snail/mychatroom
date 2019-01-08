#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>                     //gethostbyname();
#include <pthread.h>
#include<mysql/mysql.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>


#define MAXSIZE 1024
#define SERVER_PORT 30000
#define MAXSENDSIZE 4096
typedef struct
{
	char loginid[20];    /*用户名*/
	char password[20];       /*密码*/
}users_t;

typedef struct
{
    int r_sockfd;
  //  int sender;
    char content[500];
}hdr_t;

typedef struct mes 
{
	char command[8];
	char content[32];
}Message;

int echo_file(int sockfd);
int get_file(int sockfd, char *filename);
struct sockaddr_in serv, cli; 

MYSQL mysql;
void _chat(int);
users_t _login(int);
void _online(int);
void _register(int);
void sql_manage(void);
void _quit(int);
void write_off(char *);
void gif_handle_client(int *);

void main()
{
	int on=1;
	int sockfd, client_sockfd, length;
	struct sockaddr_in servAddr, cliAddr;
	pthread_t pthd;

pid_t pid;
	char buff[MAXSIZE];
	Message msg;
	
	
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Cannot create socket");
		exit(0);
	}
	
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(SERVER_PORT);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   	
	//设置地址重复利用，服务器在time_wait状态还没有消失的时候就允许服务器重启
    struct timeval tv_out;
    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;
    if( setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &tv_out, sizeof(tv_out)) == -1)
    {
        perror("setsockopt失败");
        exit(1);
    }


	if((bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr))) == -1)
	{
		perror("Cannot bind address");
		exit(0);
	}  
	
	printf("running gchat........\n");
	sql_manage();                       //init MySQL.
		
	length = sizeof(struct sockaddr_in);

	while(1)
	{
		listen(sockfd, 5);
		if((client_sockfd = accept(sockfd, (struct sockaddr *) &cliAddr, &length)) < 0)
		{
			perror("Cannot accept clients request");
			continue;
		}
		else
		{
			if(pthread_create( &pthd, NULL, (void *)gif_handle_client,(int *)&client_sockfd) != 0)
				perror("Thread creation problem");
			pthread_detach(pthd);
		}
	}
	mysql_close(&mysql);
	mysql_library_end();
}


void gif_handle_client(int* client_sockfd)
{
	int flag,ret,sockfd;
	users_t user;
	char ch,buf[10];
	sockfd=*client_sockfd;
	bzero(buf,sizeof(buf));
	while(1)
	{
		ret=read(sockfd, buf, sizeof(buf));
		flag=atoi(buf);

		switch(flag)
		{
			case 1:
			{
				user=_login(sockfd);
				printf("socket:%d\n",sockfd);
				sleep(4);
				break;
			}
			case 2:
			{
				_register(sockfd);
				break;
			}
			case 3:
			{
				_online(sockfd);
				break;
			}
			case 4:
			{
				_chat(sockfd);
				break;
			}
			case 5:
			{
				write_off(user.loginid);
				_quit(sockfd);
			//	printf("Client has Cancelled\n");
				break;
			}
			case 6:
			{
				_quit(sockfd);
			//	printf("Client has quit\n");
				break;
			}
		}
	}
}


void _chat(int client_sockfd)
{
	hdr_t r_hd;
	int recver_sock,ret;
	char recvbuf[MAXSIZE];
    bzero(recvbuf, sizeof(recvbuf));
//	printf("sockfd:%d\n",client_sockfd);

	while(1)
            {
                
                int ret = read(client_sockfd, recvbuf, sizeof(recvbuf));
                printf("收到客户端消息： ");
                if(ret == -1)   
                {
                    perror("服务器读取失败\n");
                    exit(1);
                }
                else if(ret == 0)
                {
                    printf("客户端关闭\n");
                    break;             //跳出循环不再接受消息
                }
                else       //正常读取
				{
					memcpy(&r_hd , recvbuf , sizeof(r_hd));
					if(strncmp(r_hd.content,"quit",4)==0)
					{
						printf("chat quit\n");
						break;
					}
					printf("reciver`s sockfd:%d\ncontent:%s\n",r_hd.r_sockfd,r_hd.content);
					memcpy(recvbuf,r_hd.content,sizeof(r_hd.content));
					write(r_hd.r_sockfd, recvbuf, sizeof(recvbuf));
					memset(&r_hd , 0 , sizeof(r_hd));
				}
            }
}

void _register(int client_sockfd)
{
	int t;
	users_t new_user;
	char E_buf[]={"register error"},S_buf[]={"register successfully"};
	char sqlcmd[MAXSIZE],user_ID[MAXSIZE],user_password[MAXSIZE];

	memset(user_ID, 0 , MAXSIZE);
	recv(client_sockfd , user_ID , sizeof(user_ID) , 0);
	printf("User_ID:%s\n",user_ID);

	memset(user_password, 0 , MAXSIZE);
	recv(client_sockfd , user_password , sizeof(user_password) , 0);
	printf("User_password:%s\n",user_password);

	memcpy(new_user.loginid,user_ID,20);
	memcpy(new_user.password,user_password,20);

	sprintf(sqlcmd,"insert into user_info values('%s','%s')",new_user.loginid,new_user.password);
	if(mysql_query(&mysql,sqlcmd))
	{
		fprintf(stderr,"query error:%s\n",mysql_error(&mysql));
		send(client_sockfd ,E_buf, sizeof(E_buf),0);
	}
	else send(client_sockfd ,S_buf, sizeof(S_buf),0);    //send
}

users_t _login(int client_sockfd)
{
	int t;
	users_t user;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char buf[MAXSIZE],sqlcmd[MAXSIZE];
	char E_buf[]={"No such a user\n"};
	char S_buf[]={"login successfully!\n"};
	bzero(buf,sizeof(buf));

	recv(client_sockfd , buf , sizeof(user) , 0);              //最好加个判断错误的语句
	memset(&user , 0 , sizeof(user));
	memcpy(&user , buf , sizeof(buf));

	sprintf(sqlcmd,"select * from user_info where user_id='%s' and password='%s'",user.loginid,user.password);
	printf("loginid=%s\npassword=%s\n",user.loginid,user.password);
	t=mysql_real_query(&mysql,sqlcmd,(unsigned int)strlen(sqlcmd));
	if(t)
	{
		fprintf(stderr,"query error:%s\n",mysql_error(&mysql));
	}
	else 
	{
		res=mysql_store_result(&mysql);
		row=mysql_fetch_row(res);

		if(row)
		{
			printf("login successfully\n");
			send(client_sockfd ,"login successfully!\n", sizeof(S_buf),0);

			//add information to user_online
			sprintf(sqlcmd,"insert into user_online values(%d,'%s')",client_sockfd,user.loginid);
			mysql_query(&mysql,sqlcmd);

		}
		else
		{
			
			printf("login error\n");
			send(client_sockfd ,"No such a user\n", sizeof(E_buf),0);
		}
		mysql_free_result(res);

	}
	return user;
}

void _online(int fd)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlcmd[200],recvbuf[MAXSIZE],En[]={"\n"};
	int t;
	bzero(recvbuf,sizeof(recvbuf));
	sprintf(sqlcmd,"%s","select * from user_online");
	t=mysql_real_query(&mysql,sqlcmd,(unsigned int)strlen(sqlcmd));
	if(t)
	{
		fprintf(stderr,"query error:%s\n",mysql_error(&mysql));
	}
	else 
	{
		res=mysql_store_result(&mysql);
		row=mysql_fetch_row(res);
		while(row=mysql_fetch_row(res))
		{
			for(t=0;t<mysql_num_fields(res);t++)
			{
				printf("%s ",row[t]);
				sprintf(recvbuf,"%s ",row[t]);
				send(fd,recvbuf,strlen(recvbuf),0);
			}
			send(fd,"\n",strlen("\n"),0);
			printf("\n");
		}
		mysql_free_result(res);
	}
}

void _quit(int client_sockfd)
{
	char sqlcmd[MAXSIZE];

	sprintf(sqlcmd,"delete from user_online where sockfd=%d",client_sockfd);
	mysql_query(&mysql,sqlcmd);
	close(client_sockfd);
}

void write_off(char  _id[])
{
	char sqlcmd[MAXSIZE];

	sprintf(sqlcmd,"delete from user_info where user_id='%s'",_id);
	mysql_query(&mysql,sqlcmd);
}

void sql_manage()
{
	static char *server_args[]={"this_program",
	"--datadir=.",
	"--key_buffer_size=32M"};
	static char *server_groups[]={"embeded","server","this_program_SERVER",(char *)NULL};

	if(mysql_library_init(sizeof(server_args)/sizeof(char *),server_args,server_groups))
	exit(1);
	mysql_init(&mysql);
	mysql_options(&mysql,MYSQL_READ_DEFAULT_GROUP,"your_prog_name");
	if(!mysql_real_connect(&mysql,"localhost","root","wujiabao195820","wujiabao",0,NULL,0))
	fprintf(stderr,"can not connect MYSQL:%s\n",mysql_error(&mysql));
	else puts("MySQL connect successfully");
}
