#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>

#define PORTLEN 30
#define TABLEMAXNUMBER	20
#define IPMAXLEN	50

int main(int argc , char *argv[])
{
	if(argc < 2){
		fprintf(stderr, "Need a port.\n");
		return 0;
	}

	int sockfd, status, retval, maxfd;
	int clientSockfdTableSize = 0, clientSockfdTable[TABLEMAXNUMBER], clientfd;
	char clientIpTable[TABLEMAXNUMBER][IPMAXLEN];
	int clientPortTable[TABLEMAXNUMBER];
	struct sockaddr_in client_addr;
	fd_set readset, working_readset;

    socklen_t addrlen = sizeof(client_addr);

	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me
	getaddrinfo(NULL, argv[1], &hints, &res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1){
        fprintf(stderr, "Fail to create a socket.\n");
		return 0;
    }
	status = bind(sockfd, res->ai_addr, res->ai_addrlen);

    if (status == -1){
    	fprintf(stderr, "bind fail\n");
		return 0;
    }
    status = listen(sockfd, TABLEMAXNUMBER);
    if (status == -1){
    	fprintf(stderr, "listen fail\n");
		return 0;
    }

    FD_ZERO(&readset);
	FD_SET(sockfd, &readset);
	maxfd = sockfd;

	for(int i = 0; i < TABLEMAXNUMBER; i++)
		clientSockfdTable[i] = -1;

    while(1){
		memcpy(&working_readset, &readset, sizeof(readset));
		retval = select(maxfd+1, &working_readset, NULL, NULL, 0);
		if(retval>0){
			if(FD_ISSET(sockfd, &working_readset)){		//socket fd
				//fprintf(stderr, "--------------\nsocket get client\n");
				clientfd = accept(sockfd, (struct sockaddr*) &client_addr, &addrlen);
				if(clientfd > -1){
					int ableIndex;
					for(ableIndex = 0; clientSockfdTable[ableIndex] != -1; ableIndex++);
					struct sockaddr_in c;
					socklen_t cLen = sizeof(c);
					getpeername(clientfd, (struct sockaddr*) &c, &cLen); 
					strcpy(clientIpTable[ableIndex], inet_ntoa(c.sin_addr));
					clientPortTable[ableIndex] = ntohs(c.sin_port);
					clientSockfdTable[ableIndex] = clientfd;
					//clientSockfdTable[clientSockfdTableSize++] = clientfd;
					FD_SET(clientfd, &readset);
					maxfd = (clientfd > maxfd)?clientfd:maxfd;
					//fprintf(stderr, "accept success\n");
				}
				else
					fprintf(stderr, "accept error\n");
			}
			else{
				for(int i = 0; clientSockfdTable[i] != -1; i++){
					if(FD_ISSET(clientSockfdTable[i], &working_readset)){
						int recvLen, recvInt;
						recvLen = recv(clientSockfdTable[i], &recvInt, sizeof(int), 0);
						if (recvLen == -1){
							fprintf(stderr, "recv client %d fail\n", i);
						}
						else if (recvLen == 0){
							FD_CLR(clientSockfdTable[i], &readset);
							close(clientSockfdTable[i]);
							clientSockfdTable[i] = -1;
						}
						else if (recvLen == sizeof(int)){
							printf("recv from %s:%d\n", clientIpTable[i], clientPortTable[i]);
							status = send(clientSockfdTable[i], &recvInt, sizeof(int), 0);
							if (status == -1){
								fprintf(stderr, "send to client %d fail\n", i);
							}
						}
						else{
							fprintf(stderr, "recv unexpected error\n");
						}
					}
				}
			}
		}
    }


    return 0;
}