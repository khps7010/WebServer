#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h> //perror
#include <sys/wait.h>
#include <signal.h>

#define root "/home/khps7010/program/HTTP" //html路徑
#define BUFSIZE 1024

int status;

/*檔案格式*/
struct data{
  char *ext;
  char *filetype;
}extensions[] = {{"gif", "image/gif" },
         {"jpg", "image/jpeg"},
         {"jpeg","image/jpeg"},
         {"png", "image/png" },
         {"zip", "image/zip" },
         {"gz",  "image/gz"  },
         {"tar", "image/tar" },
         {"htm", "text/html" },
         {"html","text/html" },
         {"exe","text/plain" },
         {0,0}};

/*處理程式*/
static void handle(int fd){
  int file_fd;  //檔案標籤
  int len;  //接收到的訊息長度
  char buffer[BUFSIZE], path[BUFSIZE];  //接收buf,路徑buf
  char *ftype, *ptr;  //檔案類型指標, 字串切割用指標
  struct stat st; //stat系統調用

  /*Set select*/
  int ret;  //select return
  struct timeval tv;  //waiting time
  fd_set rfd; //read set
  FD_ZERO(&rfd);  //清空
  FD_SET(fd,&rfd);  //將fd加入

  while(1){
    /* Set timeout */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if((ret = select(fd+1,&rfd,NULL,NULL,&tv)) == -1){
      perror("select()");
      break;
    }

    else if(ret == 0){
      perror("timeout");
      break;
    }
    
    else{
      memset(&buffer, 0, sizeof(buffer)); //清為0
      memset(&path, 0, sizeof(path)); //清為0
   
      //接收request
      if((len = read(fd, buffer, BUFSIZE)) < 0){
        perror("request read");
      }

      printf("%s,%d\n", buffer, (int)strlen(buffer));

      //字串搜尋HTTP
      if(strstr(buffer, " HTTP/") == NULL){
        printf("NOT HTTP\n");
        break;
      }

      else{
        ptr = strtok(buffer, " ");  //在空白處做字串切割

        /*處理GET指令*/
        if(strcmp(ptr, "GET") == 0){  //字串比較
          ptr = strtok(NULL, " ");  //空白處切割,找檔名

          /*比對檔名,檔案型態*/
          int i;
          for(i = 0; extensions[i].ext != 0; i++){
            len = strlen(extensions[i].ext);
            if(!strncmp(&ptr[strlen(ptr)-len], extensions[i].ext, len)) {
              ftype = extensions[i].filetype;     
              break;
            }
          }

          /*建立完整路徑*/
          strcpy(path,root);  //字串複製
          strcat(path,ptr); //字串相加

          /*讀取檔案並發送*/
          file_fd = open(path,O_RDONLY,0);
          if(file_fd == -1){
            sprintf(buffer, "HTTP/1.1 404 No Found\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n", ftype);
            write(fd, buffer, strlen(buffer));
          }
          else{
            stat(path, &st);
            sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: keep-alive\r\n\r\n", ftype, (int)st.st_size);
            write(fd, buffer, strlen(buffer));
            while ((len = read(file_fd, buffer, BUFSIZE)) > 0) {
              write(fd, buffer, len);
            }
          }
        }
      }
    }
  }
}
 
int main(int argc, char *argv[])
{
  int sd, client_fd;
  int one = 1; 
  int port;
  struct sockaddr_in svr_addr, cli_addr;
  socklen_t sin_len = sizeof(cli_addr);

  if(argc>1)
    port=atoi(argv[1]);
  else{
    fprintf (stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }
  
  /*open socket*/
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0){
    perror("open socket");
    exit(1);
  }

  /*Setting socket Opt*/
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
 
  /*Initial Svr_Addr*/
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(port);
 
  /*Binding ScoketAddr & SocketFD*/
  if (bind(sd, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    close(sd);
    perror("bind");
    exit(1);
  }

  /*Setting Listening*/
  if (listen(sd, 20) == -1){
    perror("listen");
    exit(1);
  }

  while (1) {
    client_fd = accept(sd, (struct sockaddr *) &cli_addr, &sin_len);

    /*If accept unsuccessful*/
    if (client_fd == -1) {
      perror("Can't accept");
      continue;
    }

    /*Accept successful*/
    printf("%d,Client from %s:%d\n", client_fd, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    switch (fork()){
      case -1:
        perror("fork Error");

      case 0:
        close(sd);
        handle(client_fd);
        close(client_fd);
        exit(status);

      default:
        close(client_fd);
    }
  }
  close(sd);
  return 0;
}
