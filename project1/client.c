#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
//#include <time.h>

#define PORTLEN 30
#define TABLEMAXNUMBER	10
#define IPMAXLEN	50
#define PORTMAXLEN	10
#define PASSSTRINGMAXLEN	30

int number = 0, serverNumber = 0;
int timeout = 1000;
char serverHostTable[TABLEMAXNUMBER][IPMAXLEN];
char serverPortTable[TABLEMAXNUMBER][PORTMAXLEN];
//char clientPortTable[TABLEMAXNUMBER][PORTMAXLEN];
int serverSockfdTable[TABLEMAXNUMBER], available[TABLEMAXNUMBER];

int socketConnect(int tableIndex){
	int status;
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(serverHostTable[tableIndex], serverPortTable[tableIndex], &hints, &res);
	serverSockfdTable[tableIndex] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	setsockopt(serverSockfdTable[tableIndex], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(char));
	//setsockopt(serverSockfdTable[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	status = connect(serverSockfdTable[tableIndex], res->ai_addr, res->ai_addrlen);
	strcpy(serverHostTable[tableIndex], inet_ntoa(((struct sockaddr_in*)res->ai_addr)->sin_addr));
	return status;
}

void* agent(void *index){
	int agetNum = (int)index, retval, isloop = 0, agentNumber;
	fprintf(stderr, "agent %d create success\n", agetNum);
	struct timeval tv;
	agentNumber = number;
	if(number == 0){
		isloop = 1;
		agentNumber = 2;
	}
	for(int i = 0; i < agentNumber; i++){
		if(isloop)
			i = 0;
		if(socketConnect(agetNum) == -1){
			printf("timeout when connect to %s\n", serverHostTable[agetNum]);
			sleep(1);
			continue;
		}
		int pingTimes = i;
		//clock_t start, finish;
		//start = clock();
		struct timeval startTime, Endtime;
		gettimeofday(&startTime, NULL);
		int status = send(serverSockfdTable[agetNum], &pingTimes, sizeof(int), 0);
		if(status == -1){
			fprintf(stderr, "Aget %d send no.%d time fail\n", agetNum, pingTimes);
		}
		status = recv(serverSockfdTable[agetNum], &pingTimes, sizeof(int), 0);
		//finish = clock();
		gettimeofday(&Endtime, NULL);
		double diff_t = (double)((double)Endtime.tv_sec * 1000.0 + (double)Endtime.tv_usec / 1000.0) - (double)((double)startTime.tv_sec * 1000.0 + (double)startTime.tv_usec / 1000.0);		
		//fprintf(stderr, "%d %f\n", timeout, diff_t);
		if(status > 0 && pingTimes == i && timeout > diff_t){
				//double diff_t = (double)(finish - start) * 1000.0 / CLOCKS_PER_SEC;
				printf("recv from %s, RTT = %f msec\n", serverHostTable[agetNum], diff_t);
		}
		else{
			printf("timeout when connect to %s\n", serverHostTable[agetNum]);
			while(pingTimes != i){
				status = recv(serverSockfdTable[agetNum], &pingTimes, sizeof(int), 0);
			}
		}
		close(serverSockfdTable[agetNum]);
		//sleep(1);
	}
	pthread_exit(NULL);
}

int main(int argc , char *argv[])
{
    int i = 1, status;
    pthread_t threadTable[TABLEMAXNUMBER];
    while(i < argc){
    	if (strcmp(argv[i], "-n") == 0){
    		number = atoi(argv[i + 1]);
    		i += 2;
    	}
    	else if (strcmp(argv[i], "-t") == 0){
    		timeout = (double)atoi(argv[i + 1]);
    		i += 2;
    	}
    	else{
    		int j = 0;
    		while(argv[i][j] != ':')
    			j ++;
    		strncpy(serverHostTable[serverNumber], argv[i], j);
    		strcpy(serverPortTable[serverNumber], argv[i] + j + 1);
    		serverNumber += 1;
    		i += 1;
    	}
    }
/*
	for(int i = 0; i < serverNumber; i++){
		struct addrinfo hints, *res;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		getaddrinfo(serverHostTable[i], serverPortTable[i], &hints, &res);
		serverSockfdTable[i] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		setsockopt(serverSockfdTable[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(char));
		//setsockopt(serverSockfdTable[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	    if (serverSockfdTable[i] == -1){
	        fprintf(stderr, "Fail to create a socket.\n");
			return 0;
	    }
		status = connect(serverSockfdTable[i], res->ai_addr, res->ai_addrlen);

		available[i] = 1;
		if (status == -1){
			printf("timeout when connect to %s\n", serverHostTable[i]);
			available[i] = 0;
		}
		else
			fprintf(stderr, "server %d connect success\n", i);
	}
*/
	for(int i = 0; i < serverNumber; i++){
//		if(available[i] == 0)
//			continue;
		pthread_create(&threadTable[i], NULL, (void*)agent, (void*)i);
	}
	for(int i = 0; i < serverNumber; i++){
//		if(available[i] == 0)
//			continue;
		pthread_join(threadTable[i], NULL);
	}

	return 0;
}