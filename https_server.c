#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "connect_tls.h"

#define MAXBUFLEN 300
int length;
int overhead;
void sendReturn(char* request, TLS_Session* session){
  char* file = strtok(request, "\n");
  file = strtok(file, " ");
  file = strtok(NULL, "/");
  file = strtok(file, " ");
  FILE* fp = fopen(file, "rb");
  char* tmp;
  if(fp != NULL){
    /*fseek( fp , 0L, SEEK_END );
    length = ftell(fp);
    rewind(fp);*/
    int overhead = strlen("HTTP/1.0 200 OK\r\n\r\n");
    tmp = (char*)malloc(length);
    strcpy(tmp, "HTTP/1.0 200 OK\r\n\r\n");
    sendTLS(session, tmp, strlen(tmp));
    char buf[MAXBUFLEN];
    int bytes_read;
    while ((bytes_read = fread(buf, 1, MAXBUFLEN, fp)) != 0){
      sendTLS(session, buf, bytes_read);
      memset(&buf, 0, MAXBUFLEN);
	}
    fclose(fp);
  }
  else{
    tmp = "HTTP/1.0 404 Not Found\r\n\r\n";
    sendTLS(session, tmp, strlen(tmp));
  }	
}
int main(int argc, char *argv[]){
  int port = atoi(argv[1]);
  int ret = serverInitTLS();
  int theSocket = listenTCP(NULL, port);
  while(1){
    int acceptedSocket = acceptTCP(theSocket);
    char buf[10000];
    int numbytes;
    if(!fork()){
      TLS_Session* session = acceptTLS(acceptedSocket);
      if((numbytes = recvTLS(session, buf, 10000)) == -1){
        printf("ERROR\n");
        exit(1);
      }
      sendReturn(buf, session);
      /*
      char* returnString = getReturn(buf);
      printf("%d", length + overhead);
      int sent;
      int pointer = 0;
      while(1) {
        if(pointer > length + overhead) {
          if((sent = sendTLS(session, returnString + length + overhead - (pointer - MAXBUFLEN), MAXBUFLEN)) == -1) {
            perror("send");
          }
          break;
        }
        if ((sent = sendTLS(session, returnString + pointer, MAXBUFLEN)) == -1)
          printf("sendERROR\n");
        pointer += MAXBUFLEN;
        printf("sent:%d\n", sent);
        }*/
      close(acceptedSocket);
      shutdownTLS(session);
    }
    close(acceptedSocket);			
  }
  serverUninitTLS();
}
