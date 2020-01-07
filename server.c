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

void epoll_add(int fd)

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

void clear_header(int sock)

{

  char buf[MAX];

  do{

    get_line(sock, buf, sizeof buf);

  }while(strcmp(buf, "\n") != 0);

}





//输出静态网页

void echo_www(int sock, char* path, int size, int* errCode)

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

  epoll_add(conn_sock);

  printf("连接成功：[id]: %d  [ip]:%s\n",conn_sock,inet_ntoa(addr.sin_addr)); 

}

//404NOT-Find

void echo_404(int sock)

{



  clear_header(sock);

  char line[MAX];

  char body[MAX];

  sprintf(body,"<!DOCTYPE HTML><head>");

  sprintf(body,"%s<title>404</title>",body);

sprintf(body,"%s<meta charset='utf-8' />",body);

sprintf(body,"%s<style>",body);

sprintf(body,"%s body { background-image: url('body.jpg'); } ",body);

sprintf(body,"%s</style> </head><body><center>",body);

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







//连接服务器

void ProcessConnect(int sock)

{

  char first_line[MAX] = {0};

  char method[MAX/32] = {0};

  char url[MAX] = {0};

  char path[MAX] = {0};

  int errCode = 200;

  int  i = 0;

  int j = 0;

  get_line(sock, first_line, sizeof(first_line));

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

  sprintf(path, "www%s", url);

  if( path[strlen(path)-1] == '/'  )

  {

    strcat(path, HOME_PATH);

  }

  struct stat st;

  if(stat(path, &st) < 0)

  {

    echo_404(sock);

    close(sock);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, NULL);

    return ;

  }

  if(S_ISDIR(st.st_mode))

  {

    strcat(path, HOME_PATH);

  }

  if( errCode != 200 )

  {

    echo_404(sock);

  }

  else

  {

	echo_www(sock, path, st.st_size, &errCode);

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

  epoll_add(lis_sock);

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
