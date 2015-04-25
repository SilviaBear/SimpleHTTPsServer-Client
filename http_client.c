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

#define MAXDATASIZE 100000
#define MAXBUFLEN 100000
#define HTTP_REQ_STR "GET /%s HTTP/1.0\r\n"\
					"User-Agent:Wget/1.12(linux-gnu)\r\n"\
					"Accept:*/*\r\n"\
					"Host: %s\r\n"\
					"Connection: Keep-Alive\r\n"\
					"\r\n"
char request[2048];
void (*getSession)(char* port, char* hostName, char* fileName);
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void getHTTPSession(char* port, char* hostName, char* fileName){
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	int rv;
	char s[INET6_ADDRSTRLEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if((rv = getaddrinfo(hostName, port, &hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return;
	}
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	if ((numbytes = sendto(sockfd, request, strlen(request), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
    printf("%s\n", request);
	addr_len = sizeof their_addr;
	FILE* fp = fopen("output", "wb+");
	int time = 0;
	while((numbytes = recvfrom(sockfd, buf, MAXBUFLEN, 0,
		(struct sockaddr *)&their_addr, &addr_len)) > 0){
      printf("%s\n", buf);
		if(time == 0){
			char* data;
			if((data = strstr(buf, "\r\n\r\n")) != NULL){
				int length = strlen(buf) - strlen(data) + 4 ;
				fwrite(data + 4, numbytes - length, 1, fp);
				time = 1;
			}
		}
		else
			fwrite(buf, numbytes, 1, fp);
		memset(buf, 0, MAXDATASIZE);
	}
	fclose(fp);
}
void getHTTPsSession(char* port, char* hostName, char* fileName){
	char buf[MAXDATASIZE];
	int rev;
	clientInitTLS();
	TLS_Session* session = connectTLS(hostName, atoi(port));
	sendTLS(session, request, strlen(request));
	FILE* fp = fopen("output", "wb+");
	int time = 0;
	while((rev = recvTLS(session, buf, MAXBUFLEN-1)) > 0){
		
		if(time == 0){
			char* data;
			if((data = strstr(buf, "\r\n\r\n")) != NULL){
				int length = strlen(buf) - strlen(data) + 4 ;
				fwrite(data + 4, rev - length, 1, fp);
				time = 1;
			}
		}
		else
			fwrite(buf, rev, 1, fp);
		memset(buf, 0, MAXDATASIZE);
	}
	fclose(fp);
	clientUninitTLS();
}
char** getHostInfo(char* host){
	char** result = (char**)malloc(3*sizeof(char*));
	char* copy = (char*)malloc(strlen(host) + 1);
	strcpy(copy, host);
	char* hostName = strtok(copy, "://");
	char* port;
	if(hostName[strlen(hostName) - 1] == 's'){
		getSession = getHTTPsSession;
		port = "443";
	}
	else{
		getSession = getHTTPSession;
		port = "80";
	}
	hostName = strtok(NULL, "/");
	char* fileName = strtok(NULL, "");
	char* copyHost = (char*)malloc(strlen(hostName) + 1);
	strcpy(copyHost, hostName);
	char* pPort = strtok(copyHost, ":");
	if(strlen(pPort) != strlen(hostName)){
		port = strtok(NULL, "/");
		hostName = copyHost;
	}
	result[0] = port;
	result[1] = hostName;
	result[2] = fileName;	
	return result;
}
int main(int argc, char *argv[]){
	char** hostInfo = getHostInfo(argv[1]);
	sprintf(request, HTTP_REQ_STR, hostInfo[2], hostInfo[1]);
	getSession(hostInfo[0], hostInfo[1], hostInfo[2]);
	free(hostInfo);
}
