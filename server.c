#include <unistd.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <strings.h>

#include <ctype.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <sys/wait.h>

#include <sys/sendfile.h>

#include <sys/socket.h>

#include <sys/types.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <sys/epoll.h>





#define MAX 1024

#define HOME_PATH "index.html"





static int epoll_fd = 0;





//epoll添加文件描述符

void addepol(int fd)

{

  struct epoll_event event;

  event.events = EPOLLIN;

  event.data.fd = fd;

  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);

}





//获取首行

int get_line(int sock, char* line, int size)

{

  int i = 0;

  char c = 'a';

  while(i < size && c != '\n')

  {

    int s = recv(sock, &c, 1 , 0);

    if( s <= 0 )break;

    if(c == '\r')

    {

      recv(sock, &c, 1, MSG_PEEK);

      if(c != '\n')

      {

        c = '\n';

      }

      else

      {

        recv(sock, &c, 1, 0);

      }

    }

    line[i++] = c;

  }

  line[i] = '\0';

  return i;

}





//清空首行

void clear_header(int sock)

{

  char buf[MAX];

  do{

    get_line(sock, buf, sizeof buf);

  }while(strcmp(buf, "\n") != 0);

}





//输出静态网页

void printf_www(int sock, char* path, int size, int* errCode)

{

  clear_header(sock);

  char line[MAX];

  int fd = open(path, O_RDONLY);

  if( fd < 0)

  {

    *errCode = 0; 

    return ;

  }

  sprintf(line, "HTTP/1.1 200 OK\r\n");

  send(sock, line, strlen(line), 0);

  sprintf(line, "Content-Length: %d\r\n",size);

  send(sock, line, strlen(line), 0);

  sprintf(line, "\r\n");

  send(sock, line, strlen(line), 0);

  sendfile(sock, fd, NULL, size);

  close(fd);

}





//建立套接字连接

void ProcessCreate(int sock)

{

  struct sockaddr_in addr;

  socklen_t len = sizeof addr;

  int conn_sock =  accept(sock, (struct sockaddr*)&addr, &len);

  if(conn_sock < 0)

  {

    perror("accept");

    return ;

  }

  addepol(conn_sock);

  printf("连接成功：[id]: %d  [ip]:%s\n",conn_sock,inet_ntoa(addr.sin_addr)); 

}







//404NOT-Find

void NotFind(int sock)

{

	

  clear_header(sock);

  char line[MAX];

  char body[MAX];

  sprintf(body,"<!DOCTYPE HTML><head>");

  sprintf(body,"%s<title>404</title>",body);

  sprintf(body,"%s<meta charset='utf-8' />",body);

  sprintf(body,"%s</head><body><center>",body);

  sprintf(body,"%s<h1> 404！</h1>",body);

  sprintf(body,"%s<h1> 您所访问的页面不存在。</h1>",body);

  sprintf(body,"%s</center></body></html>",body);

  int size = (int)strlen(body);

  sprintf(line, "HTTP/1.1 200 OK\r\n");

  send(sock, line, strlen(line), 0);

  sprintf(line, "Content-Length: %d\r\n", size);

  send(sock, line, strlen(line), 0);

  sprintf(line, "\r\n");

  send(sock, line, strlen(line), 0);

  send(sock, body, strlen(body), 0);

}





//cgi程序

int exe_cgi(int sock, char* path, char* method, char* query_string)

{

  char line[MAX];

  int content_length = -1;

  char method_env[MAX/32] = {0};

  char query_string_env[MAX] = {0};

  char content_length_env[MAX] = {0};

  if(strcasecmp(method , "GET") == 0)

  {

    clear_header(sock);

  }

  else 

  {

    do

    {

      get_line(sock, line, sizeof line);

      if(strncmp(line, "Content-Length: ",16) == 0)

      {

        sscanf(line, "Content-Length: %d", &content_length);

      }

    }while(strcmp(line, "\n") != 0);

    if(content_length == -1)

      return 404;

  }

  sprintf(line, "HTTP/1.1 200 OK\r\n");

  send(sock, line, strlen(line), 0);

  sprintf(line, "Content-type: text/html\r\n");

  send(sock, line, strlen(line), 0);

  sprintf(line, "\r\n");

  send(sock, line, strlen(line), 0);

  int input[2];

  int output[2];

  pipe(input);

  pipe(output);

  pid_t pid = fork();

  if(pid < 0)

  {

    return 404;

  }

  if(pid == 0)

  {

    close(input[1]);

    close(output[0]);

    dup2(input[0], 0);

    dup2(output[1], 1);

    sprintf(method_env, "METHOD=%s", method);

    putenv(method_env);

    if(strcasecmp(method, "POST") == 0)

    {

      sprintf(content_length_env, "CONTENT_LENGTH=%d", content_length);

      putenv(content_length_env);

    }

    else 

    {

      sprintf(query_string_env, "QUERY_STRING=%s", query_string);

      putenv(query_string_env);

    }

    execl(path, path , NULL);

  }

  else 

  {

    close(input[0]);

    close(output[1]);

    char c;

    if(strcasecmp(method, "POST") == 0)

    {

      int i = 0;

      for(; i<content_length; ++i)

      {

        read(sock, &c, 1);

        write(input[1], &c, 1);

      }

    }

    while( read(output[0], &c, 1) > 0)

    {

      send(sock, &c, 1, 0);

    }

    waitpid(input[1] , 0, 0);

    close(input[1]);

    close(output[0]);

  }

  return 200;

}





//连接服务器

void ProcessConnect(int sock)

{

  char first_line[MAX] = {0};

  char method[MAX/32] = {0};

  char url[MAX] = {0};

  char path[MAX] = {0};

  char* query_string = NULL;

  int errCode = 200;

  int cgi = 0;

  int i = 0;

  int j = 0;

  if(get_line(sock, first_line, sizeof(first_line)) ==  0 )

  {

    NotFind(sock);

    close(sock);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);

	return ;

  }

  while(i < sizeof(method) -1 && j < sizeof(first_line) && !isspace(first_line[j]))

  {

      method[i++] = first_line[j++];

  }

  method[i] = '\0';

  while(j < sizeof(first_line) && isspace(first_line[j]))

  {

    j++;

  }

  i = 0;

  while(i < sizeof(url) -1 && j < sizeof(first_line) && !isspace(first_line[j]))

  {

      url[i++] = first_line[j++];

  }

  if(strcasecmp(method, "POST") == 0)

  {

    cgi = 1;

  }

  else if(strcasecmp(method, "GET") == 0 )

  {

    query_string = url;

    while(*query_string)

    {

      if(*query_string == '?')

      {

        *query_string = '\0';

        query_string++;

        cgi = 1;

        break;

      }

      query_string++; 

    }

  }

  else 

  {

    NotFind(sock);

    close(sock);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);

	return ;

  }

  sprintf(path, "www%s", url);

  if( path[strlen(path)-1] == '/'  )

  {

    strcat(path, HOME_PATH);

  }

  struct stat st;

  if(stat(path, &st) < 0)

  {

    NotFind(sock);

    close(sock);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);

	return ;

  }

  if(S_ISDIR(st.st_mode))

  {

    strcat(path, HOME_PATH);

  }

  if( (st.st_mode & S_IXUSR) ||  (st.st_mode & S_IXGRP ) ||  (st.st_mode & S_IXOTH) )

  {

    cgi = 1;

  }

  if(cgi)

  {

    errCode = exe_cgi(sock, path, method, query_string);

  }

  else 

  {

    printf_www(sock, path, st.st_size, &errCode);

  }

    close(sock);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);

}





// 套接字初始化

int SocketInit(char* ip, int port)

{

  int sock = socket(AF_INET,SOCK_STREAM, 0);

  if( sock < 0 )

  {

    perror("socket");

    exit(1);

  }

  int opt = 1;

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

  struct sockaddr_in addr;

  addr.sin_family = AF_INET;

  addr.sin_port = htons(port);

  addr.sin_addr.s_addr = inet_addr(ip);

  if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0 )

  {

    perror("bind");

    exit(1);

  }

  if(listen(sock, 5) < 0)

  {

    perror("listen");

    exit(1);

  }

  epoll_fd = epoll_create(10);

  return sock;

}





int main(int argc, char* argv[])

{

  argv[1]="127.0.0.1";

  argv[2]="8080";

  int lis_sock = SocketInit(argv[1],atoi(argv[2]) );

  printf("正在连接......\n\n");

  addepol(lis_sock);

  struct epoll_event event[10];

  while(1)

  {

    int size = epoll_wait(epoll_fd, event, sizeof(event)/sizeof(event[0]), 500);

    if(size < 0)

    {

      perror("epoll_wait");

      exit(1);

    }

    if(size == 0)

    {   

      continue; 

    }

    int i = 0;

    for(; i<size; ++i)

    {

      if(event[i].data.fd == lis_sock)

        ProcessCreate(lis_sock);

      else

        ProcessConnect(event[i].data.fd); 

    }

  }

  return 0;

}