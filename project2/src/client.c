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
#include <fcntl.h>
#include <openssl/md5.h>

#include "common.h"

char serverHost[128], serverPort[8];
char username[MAX_PROTOCOL_NAME_LEN], password[MAX_PROTOCOL_NAME_LEN], chatMateName[MAX_PROTOCOL_NAME_LEN];
char unReadMsg[MAX_PROTOCOL_FILE_CONTENT_LEN];
int unReadMsgLen, hasUnReadMsg = 0;

void pic(){
	printf(" ||====================================================================|| \n");
	printf(" ||//$\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\//$\\|| \n");
	printf(" ||(100)==================| RESERVE BANK OF INDIA|================(100)|| \n");
	printf(" ||\\\\$//        ~         '------========--------'                \\\\$//|| \n");
	printf(" ||<< /        /$\\              // ____ \\\\                         \\ >>|| \n");
	printf(" ||>>|        //L\\\\            // ///..) \\\\              XXXX       |<<|| \n");
	printf(" ||<<|        \\\\ //           || <||  >\\  ||                        |>>|| \n");
	printf(" ||>>|         \\$/            ||  $$ --/  ||          XXXXXXXXX     |<<|| \n");
	printf(" ||<<|     Free to Use        *\\\\  |\\_/  //*                        |>>|| \n");
	printf(" ||>>|                         *\\\\/___\\_//*                         |<<|| \n");
	printf(" ||<<\\      Rating: E     _____/ M GANDHI \\________    XX XXXXX     />>|| \n");
	printf(" ||//$\\                 ~|    REPUBLIC OF INDIA   |~               /$\\\\|| \n");
	printf(" ||(100)===================   ONE HUNDRED RUPEES =================(100)|| \n");
	printf(" ||\\$//\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\$//|| \n");
	printf(" ||====================================================================|| \n");
}

int buildClientSocket(const char* serverAddress){
	int idx = 0, status;
	while(serverAddress[idx] != ':')
		idx ++;
	strncpy(serverHost, serverAddress, idx);
    strcpy(serverPort, serverAddress + idx + 1);
	
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(serverHost, serverPort, &hints, &res);
	int socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	//setsockopt(serverSockfdTable[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(char));
	if (socketfd == -1){
        fprintf(OUTPUT_LOG, "Fail to create a socket.\n");
		return -1;
    }
	status = connect(socketfd, res->ai_addr, res->ai_addrlen);
	if (status == -1){
		fprintf(OUTPUT_LOG, "connect fail\n");
	}
	return socketfd;
}

int signedUp(int socketfd){
	int status, recvLen;
	printf("username: ");
	scanf("%s", username);
	int usernameLen = strlen(username);
	for(int i = 0; i < usernameLen; i++)
		if(username[i] == '_'){
			printf("Username can not include '_'\n");
			return -1;
		}
	printf("password: ");
	scanf("%s", password);
	Protocol_signup sendPackage, recvPackage;
	memset(&sendPackage, 0, sizeof(sendPackage));
	sendPackage.message.header.head.option = PROTOCOL_OPTION_SIGNUP;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	strcpy(sendPackage.message.body.username, username);

	MD5_CTX tmp_CTX;
    char out[33];
    MD5_Init(&tmp_CTX);
    MD5_Update(&tmp_CTX, password, strlen(password));
    md5_make(out, &tmp_CTX);
	//fprintf(OUTPUT_LOG, "%s\n", out);

	strcpy(sendPackage.message.body.password, out);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if (recvPackage.message.header.head.option == PROTOCOL_OPTION_SIGNUP &&
		 recvPackage.message.header.head.status == PROTOCOL_STATUS_SUCC ){
		printf("sign-up success\n");
		return 1;
	}
	else {
		printf("sign-up unacceptable, maybe there is other user uses the same username.\n");
		return 0;
	}
}

int logIn(int socketfd){
	int status, recvLen;
	Protocol_login sendPackage, recvPackage;
	memset(&sendPackage, 0, sizeof(sendPackage));
	sendPackage.message.header.head.option = PROTOCOL_OPTION_LOGIN;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	strcpy(sendPackage.message.body.username, username);

	MD5_CTX tmp_CTX;
    char out[33];
    MD5_Init(&tmp_CTX);
    MD5_Update(&tmp_CTX, password, strlen(password));
    md5_make(out, &tmp_CTX);
	//fprintf(OUTPUT_LOG, "%s\n", out);

	strcpy(sendPackage.message.body.password, out);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if (recvPackage.message.header.head.option == PROTOCOL_OPTION_LOGIN &&
		 recvPackage.message.header.head.status == PROTOCOL_STATUS_SUCC ){
		return 1;
	}
	else {
		printf("login unacceptable\n");
		if (strcmp(recvPackage.message.body.username, "passwordIncorrect") == 0)
			printf("password incorrect\n");
		else if (strcmp(recvPackage.message.body.username, "alreadyLogIn") == 0)
			printf("already log in\n");
		return 0;
	}
}

int quitSend(int socketfd){
	int status, recvLen;
	Protocol_quit sendPackage, recvPackage;
	memset(&sendPackage, 0, sizeof(sendPackage));
	sendPackage.message.header.head.option = PROTOCOL_OPTION_QUIT;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
		return -1;
	}
	if (recvPackage.message.header.head.option == PROTOCOL_OPTION_QUIT &&
		 recvPackage.message.header.head.status == PROTOCOL_STATUS_SUCC ){
		close(socketfd);
		printf("bye bye ~\n");
		exit(1);
		return 0;
	}
	else {
		printf("quit unacceptable\n");
		return -1;
	}
}

int startChatSend(int socketfd){
	printf("Chat mate name: ");
	scanf("%s", chatMateName);
	if (strcmp(chatMateName, username) == 0)
		return 5;
	int status, recvLen;
	Protocol_startchat sendPackage, recvPackage;
	memset(&sendPackage, 0, sizeof(sendPackage));
	sendPackage.message.header.head.option = PROTOCOL_OPTION_STARTCHAT;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	strcpy(sendPackage.message.body.chatMateName, chatMateName);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	//fprintf(OUTPUT_LOG, "wait recvPackage\n");
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	//fprintf(OUTPUT_LOG, "get recvPackage\n");
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
		return -1;
	}
	if (recvPackage.message.header.head.option == PROTOCOL_OPTION_STARTCHAT && recvPackage.message.header.head.status == PROTOCOL_STATUS_SUCC ){
		if(recvPackage.message.body.unReadMsgLen > 0){
			memcpy(unReadMsg, recvPackage.message.body.unReadMsg, recvPackage.message.body.unReadMsgLen);
			unReadMsgLen = recvPackage.message.body.unReadMsgLen;
			hasUnReadMsg = 1;
		}
		if ( strcmp(recvPackage.message.body.chatMateName, chatMateName) == 0 )
			return 2;
		else if (strcmp(recvPackage.message.body.chatMateName, username) == 0)
			return 3;
		else
			return 1;
	}
	else if ( recvPackage.message.header.head.status == PROTOCOL_STATUS_FAIL && strcmp(recvPackage.message.body.chatMateName, username) == 0){
		return 4;
	}
	else {
		printf("startchat unacceptable\n");
		return -1;
	}
}

int sendMsgSend(int socketfd, char *inputString){
	int status, recvLen;
	//char inputString[MAX_PROTOCOL_CONTENT_LEN];
	//scanf("%s", inputString);
	Protocol_sendmsg sendPackage, recvPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_SENDMSG;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	strcpy(sendPackage.message.body.content, inputString);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if(recvPackage.message.header.head.status != PROTOCOL_STATUS_SUCC)
		return -1;
	else
		return 0;
}

int stopChatSend(int socketfd){
	int status, recvLen;
	Protocol_stopchat sendPackage, recvPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_STOPCHAT;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	//fprintf(OUTPUT_LOG, "send stopChatSend\n");
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	//fprintf(OUTPUT_LOG, "get stopChatSend\n");
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if(recvPackage.message.header.head.status == PROTOCOL_STATUS_SUCC)
		return 0;
	else
		return -1;
}

void* sendFileContent(void* arg){
	//fprintf(OUTPUT_LOG, "> in the sendFileContent\n");
	char *inputString = (char*) arg;
	int status, recvLen;
	/* ----- create socket ----- */
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(serverHost, serverPort, &hints, &res);
	int socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	//setsockopt(serverSockfdTable[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(char));
	if (socketfd == -1){
        fprintf(OUTPUT_LOG, "Fail to create a socket.\n");
        close(socketfd);
		pthread_exit(NULL);
    }
	status = connect(socketfd, res->ai_addr, res->ai_addrlen);
	if (status == -1){
		fprintf(OUTPUT_LOG, "connect fail\n");
        close(socketfd);
		pthread_exit(NULL);
	}
	int fp = open(inputString, O_RDONLY), readLen, index = 0;
	//fprintf(OUTPUT_LOG, "open file O_RDONLY\n");
	Protocol_sendfilecontent sendPackage, recvPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_SENDFILECONTENT;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	while ( (readLen = read(fp, sendPackage.message.body.fileContent, MAX_PROTOCOL_FILE_CONTENT_LEN)) != 0 ){
		sendPackage.message.header.head.option = PROTOCOL_OPTION_SENDFILECONTENT;
		sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
		sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
		strcpy(sendPackage.message.body.fileName, inputString);
		//fprintf(OUTPUT_LOG, "sendFileContent read %d, index = %d\n", readLen, index);
		index ++;
		if ( readLen == MAX_PROTOCOL_FILE_CONTENT_LEN )
			sendPackage.message.body.fragflag = 1;
		else
			sendPackage.message.body.fragflag = 0;
		sendPackage.message.body.fileContentLen = readLen;
		do{
			status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
		}while(status == -1);
		//fprintf(OUTPUT_LOG, "send sendFileContent\n");
		recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
		//fprintf(OUTPUT_LOG, "get sendFileContent\n");
		if(recvLen == 0){
			fprintf(OUTPUT_LOG, "server is out\n");
			exit(1);
		}
		if(recvPackage.message.header.head.status != PROTOCOL_STATUS_SUCC){
			printf("send fileContent fail\n");
			pthread_exit(NULL);
		}
		if(sendPackage.message.body.fragflag == 0)
			break;
	}
	close(socketfd);
	close(fp);
	free(arg);
	pthread_exit(NULL);
}

void sendFileSend(int socketfd){
	//fprintf(OUTPUT_LOG, "> in the sendFileSend\n");
	int status, recvLen;
	char inputString[MAX_PROTOCOL_CONTENT_LEN];
	scanf("%s", inputString);
	if( access( inputString, F_OK ) == -1 ){
		printf("file doesn't exist\n");
		return;
	}
	Protocol_sendfile sendPackage, recvPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_SENDFILE;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	strcpy(sendPackage.message.body.fileName, inputString);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	//fprintf(OUTPUT_LOG, "send stopChatSend\n");
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	//fprintf(OUTPUT_LOG, "get stopChatSend\n");
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if(recvPackage.message.header.head.status != PROTOCOL_STATUS_SUCC){
		printf("send file fail\n");
		return;
	}
	char *arg = (char*)malloc(sizeof(inputString));
	memcpy(arg, inputString, sizeof(inputString));
	pthread_t thread;
	pthread_create(&thread, NULL, (void*)sendFileContent, (void*)arg);
	return;
}

void historySend(int socketfd){
	//fprintf(OUTPUT_LOG, "> in the historySend\n");
	int status, recvLen;
	Protocol_sendfilecontent sendPackage, recvPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_HISTORY;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	//fprintf(OUTPUT_LOG, "send stopChatSend\n");
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	//fprintf(OUTPUT_LOG, "get stopChatSend\n");
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if(recvPackage.message.header.head.status != PROTOCOL_STATUS_SUCC){
		printf("get history fail\n");
		return;
	}
	printf("%s\n", recvPackage.message.body.fileContent);
	return;
}

void goodFriendSend(int socketfd){
	int status, recvLen;
	char inputString[MAX_PROTOCOL_CONTENT_LEN];
	scanf("%s", inputString);
	Protocol_startchat sendPackage, recvPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_GOODFRIEND;
	if ( strcmp(inputString, "s") == 0 )
		sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	else if ( strcmp(inputString, "a") == 0 ){
		sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
		scanf("%s", inputString);
		strcpy(sendPackage.message.body.chatMateName, inputString);
	}
	else if ( strcmp(inputString, "d") == 0 ){
		sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
		scanf("%s", inputString);
		strcpy(sendPackage.message.body.chatMateName, inputString);
	}
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	//fprintf(OUTPUT_LOG, "send goodFriendSend\n");
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	//fprintf(OUTPUT_LOG, "get goodFriendSend\n");
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if ( recvPackage.message.header.head.status == PROTOCOL_STATUS_FAIL ){
		printf("request fail\n");
	}
	else if ( sendPackage.message.header.head.status == PROTOCOL_STATUS_REQ ){
		printf("\n==== good-friend ====\n");
		if(recvPackage.message.body.unReadMsgLen == 0){
			printf("      [empty]\n");
		}
		else{
			write(1, recvPackage.message.body.unReadMsg, recvPackage.message.body.unReadMsgLen);
		}
		printf("=====================\n\n");
	}
	else if ( sendPackage.message.header.head.status == PROTOCOL_STATUS_SUCC ){
		printf("add good-friend success\n");
	}
	else if ( sendPackage.message.header.head.status == PROTOCOL_STATUS_FAIL ){
		printf("delete good-friend success\n");
	}

}

void badFriendSend(int socketfd){
	int status, recvLen;
	char inputString[MAX_PROTOCOL_CONTENT_LEN];
	scanf("%s", inputString);
	Protocol_startchat sendPackage, recvPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_BADFRIEND;
	if ( strcmp(inputString, "s") == 0 )
		sendPackage.message.header.head.status = PROTOCOL_STATUS_REQ;
	else if ( strcmp(inputString, "a") == 0 ){
		sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
		scanf("%s", inputString);
		strcpy(sendPackage.message.body.chatMateName, inputString);
	}
	else if ( strcmp(inputString, "d") == 0 ){
		sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
		scanf("%s", inputString);
		strcpy(sendPackage.message.body.chatMateName, inputString);
	}
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	//fprintf(OUTPUT_LOG, "send badFriendSend\n");
	recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
	//fprintf(OUTPUT_LOG, "get badFriendSend\n");
	if(recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	if ( recvPackage.message.header.head.status == PROTOCOL_STATUS_FAIL ){
		printf("request fail\n");
	}
	else if ( sendPackage.message.header.head.status == PROTOCOL_STATUS_REQ ){
		printf("\n==== bad-friend ====\n");
		if(recvPackage.message.body.unReadMsgLen == 0){
			printf("      [empty]\n");
		}
		else{
			write(1, recvPackage.message.body.unReadMsg, recvPackage.message.body.unReadMsgLen);
		}
		printf("=====================\n\n");
	}
	else if ( sendPackage.message.header.head.status == PROTOCOL_STATUS_SUCC ){
		printf("add bad-friend success\n");
	}
	else if ( sendPackage.message.header.head.status == PROTOCOL_STATUS_FAIL ){
		printf("delete bad-friend success\n");
	}

}

Client_status getInput(int socketfd, Client_status clientStatusNow){
	int status;
	char inputString[MAX_PROTOCOL_CONTENT_LEN], tmp[MAX_PROTOCOL_CONTENT_LEN];
	scanf("%s", inputString);
	if (clientStatusNow == CLIENT_STATUS_WAIT){
		if (strcmp(inputString, "c") == 0){
			status = startChatSend(socketfd);
			if (status == 2){
				return CLIENT_STATUS_CHAT_TOGETHER;
			}
			else if (status == 3){
				write(1, unReadMsg, unReadMsgLen);
				printf("user '%s' is not online, but you can still send the message.\n", chatMateName);
				return CLIENT_STATUS_CHAT_ALONE;
			}
			else if (status == 4){
				printf("He don't want to talk to you\n");
			}
			else if (status == 5){
				printf("You can not talk to yourself\n");
			}
			else{
				printf("no this user in the config file.\n");
			}
		}
		else if (strcmp(inputString, "q") == 0){
			if (quitSend(socketfd) < 0)
				printf("quit fail, you may try later...\n");
		}
		else if (strcmp(inputString, "gf") == 0){
			goodFriendSend(socketfd);
		}
		else if (strcmp(inputString, "bf") == 0){
			badFriendSend(socketfd);
		}
		else{
			printf("unknown command, please re-enter\n");
		}
		return clientStatusNow;
	}
	else{
		if (strcmp(inputString, "$") == 0){
			scanf("%s", inputString);
			if (strcmp(inputString, "pic") == 0){
				sprintf(tmp, "$ %s", inputString);
				sendMsgSend(socketfd, tmp);
				return clientStatusNow;
			}
			if (strcmp(inputString, "s") == 0){
				sendFileSend(socketfd);
				return clientStatusNow;
			}
			else if (strcmp(inputString, "sc") == 0){
				if (stopChatSend(socketfd) < 0 ){
					printf(">>> stop chat request fail <<<\n");
					return clientStatusNow;
				}
				else
					return CLIENT_STATUS_WAIT;
			}
			else if (strcmp(inputString, "h") == 0){
				historySend(socketfd);
				return clientStatusNow;
			}
			else{
				printf("known command\n");
				return clientStatusNow;
			}
		}
		else{
			if (sendMsgSend(socketfd, inputString) < 0){
				printf(">>> send message fail <<<\n");
				return clientStatusNow;
			}
			else
				return clientStatusNow;
		}
	}
}

int startChatGet(int socketfd, Client_status clientStatusNow, Protocol_startchat *recvPackage){
	//fprintf(OUTPUT_LOG, "> in the startChatGet\n");
	int status;
	Protocol_startchat sendPackage;
	if (recvPackage->message.header.head.option == PROTOCOL_OPTION_STARTCHAT &&
		 recvPackage->message.header.head.status == PROTOCOL_STATUS_REQ ){
		//fprintf(OUTPUT_LOG, "get a startchat request, and accept\n");
		memset(&sendPackage, 0, sizeof(sendPackage));
		sendPackage.message.header.head.option = PROTOCOL_OPTION_STARTCHAT;
		if ( clientStatusNow == CLIENT_STATUS_WAIT ){
			/*
			printf("user '%s' want to chat with you, accept[a] or reject[r]\n", recvPackage->message.body.chatMateName);
			char inputString[MAX_PROTOCOL_CONTENT_LEN];
			scanf("%s", inputString);
			if (strcmp(inputString, "a") == 0){
				sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
				strcpy(chatMateName, recvPackage->message.body.chatMateName);
			}
			else{
				sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
			}
			*/
			sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
			strcpy(chatMateName, recvPackage->message.body.chatMateName);
		}
		else
			sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
		sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
		do{
			status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
		}while(status == -1);
		//fprintf(OUTPUT_LOG, "finish to sendPackage\n");
		if(sendPackage.message.header.head.status == PROTOCOL_STATUS_SUCC)
			return 0;
		else
			return -1;
	}
	else{
		fprintf(OUTPUT_LOG, "startChatGet recvPackage error\n");
		return -1;
	}
}

int sendMsgGet(int socketfd, Protocol_sendmsg *recvPackage){
	int status, recvLen;
	Protocol_sendmsg sendPackage;
	if(strcmp(recvPackage->message.body.content, "$ pic") == 0){
		printf("%s: \n", chatMateName);
		pic();
	}
	else
		printf("%s: %s\n", chatMateName, recvPackage->message.body.content);
	sendPackage.message.header.head.option = PROTOCOL_OPTION_SENDMSG;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	return 0;
}

int stopChatGet(int socketfd, Protocol_stopchat *recvPackage){
	//fprintf(OUTPUT_LOG, "> in the stopChatGet\n");
	int status, recvLen;
	Protocol_stopchat sendPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_STOPCHAT;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	do{
		status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	return 0;
}

void* sendFileGet(void* arg){
	//fprintf(OUTPUT_LOG, "> in the sendFileGet\n");
	int status, recvLen;
	Protocol_sendfile *argPackage = (Protocol_sendfile*)arg;
	/* ----- create socket ----- */
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(serverHost, serverPort, &hints, &res);
	int socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	//fprintf(OUTPUT_LOG, "origin socketfd %d, new socketfd %d\n", argPackage->message.body.tmp, socketfd);
	//setsockopt(serverSockfdTable[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(char));
	if (socketfd == -1){
        fprintf(OUTPUT_LOG, "Fail to create a socket.\n");
        close(socketfd);
		pthread_exit(NULL);
    }
	status = connect(socketfd, res->ai_addr, res->ai_addrlen);
	if (status == -1){
		fprintf(OUTPUT_LOG, "connect fail\n");
        close(socketfd);
		pthread_exit(NULL);
	}
	//fprintf(OUTPUT_LOG, "socket create\n");
	/* ----- send back ----- */
	Protocol_sendfile sendPackageToChatMate;
	sendPackageToChatMate.message.header.head.option = PROTOCOL_OPTION_SENDFILE;
	sendPackageToChatMate.message.header.head.status = PROTOCOL_STATUS_SUCC;
	sendPackageToChatMate.message.header.head.datalen = sizeof(sendPackageToChatMate) - sizeof(Protocol_header);
	do{
		status = send(argPackage->message.body.tmp, &sendPackageToChatMate, sizeof(sendPackageToChatMate), 0);
	}while(status == -1);
	//fprintf(OUTPUT_LOG, "sendFileGet send back success\n");

	int fp = open(argPackage->message.body.fileName, O_WRONLY | O_CREAT | O_TRUNC), index = 0;
	Protocol_sendfilecontent sendPackage, recvPackage;
	printf("=> get file from '%s', fileName is '%s', transmit start\n", chatMateName, argPackage->message.body.fileName);
	//fprintf(OUTPUT_LOG, "open file O_WRONLY fp = %d\n", fp);
	do {
		recvLen = recv(socketfd, &recvPackage, sizeof(recvPackage), MSG_WAITALL);
		//fprintf(OUTPUT_LOG, "sendFileContentGet recvPackage recvLen %d, fileContentLen %d, index = %d\n", recvLen, recvPackage.message.body.fileContentLen, index);
		index ++;
		if (recvLen == 0){
			fprintf(OUTPUT_LOG, "socket is out\n");
			exit(1);
		}
		write(fp, recvPackage.message.body.fileContent, recvPackage.message.body.fileContentLen);
		//fprintf(OUTPUT_LOG, "%s\n", recvPackage.message.body.fileContent);
		sendPackage.message.header.head.option = PROTOCOL_OPTION_SENDFILECONTENT;
		sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
		sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
		do{
			status = send(socketfd, &sendPackage, sizeof(sendPackage), 0);
		}while(status == -1);
	} while(recvPackage.message.body.fragflag == 1);
	printf("=> get file from '%s', fileName is '%s', transmit finish\n", chatMateName, argPackage->message.body.fileName);
	close(fp);
	close(socketfd);
	free(arg);
	pthread_exit(NULL);
}

Client_status getPackage(int socketfd, Client_status clientStatusNow){
	Protocol_header header;
	Client_status clientStatusAfter;
	int recvLen = recv(socketfd, &header, sizeof(header), 0);
	if (recvLen == -1){
		fprintf(OUTPUT_LOG, "recv unknown\n");
		clientStatusAfter = clientStatusNow;
	}
	else if (recvLen == 0){
		fprintf(OUTPUT_LOG, "server is out\n");
		exit(1);
	}
	else if (recvLen == sizeof(header)){
		if( header.head.status != PROTOCOL_STATUS_REQ )
			return clientStatusNow;
		if (clientStatusNow == CLIENT_STATUS_WAIT){
			switch(header.head.option){
				case PROTOCOL_OPTION_STARTCHAT:{
					Protocol_startchat startChatPackage;
					if ( completeMessageWithHeader(socketfd, &header, &startChatPackage) ){
						if (startChatGet(socketfd, clientStatusNow, &startChatPackage) < 0)
							return clientStatusNow;
						else
							return CLIENT_STATUS_CHAT_TOGETHER;
					}
					break;
				}
				default:
					fprintf(stderr, "unknown option %x\n", header.head.option);
					return clientStatusNow;
					break;
			}
		}
		else{
			switch(header.head.option){
				case PROTOCOL_OPTION_STARTCHAT:{
					Protocol_startchat startChatPackage;
					if ( completeMessageWithHeader(socketfd, &header, &startChatPackage) ){
						if (startChatGet(socketfd, clientStatusNow, &startChatPackage) < 0)
							return clientStatusNow;
						else
							return CLIENT_STATUS_CHAT_TOGETHER;
					}
					break;
				}
				case PROTOCOL_OPTION_SENDMSG:{
					Protocol_sendmsg sendMsgPackage;
					if ( completeMessageWithHeader(socketfd, &header, &sendMsgPackage) ){
						if (sendMsgGet(socketfd, &sendMsgPackage) < 0)
							return clientStatusNow;
						else
							return clientStatusNow;
					}
					break;
				}
				case PROTOCOL_OPTION_STOPCHAT:{
					Protocol_stopchat stopChatPackage;
					if ( completeMessageWithHeader(socketfd, &header, &stopChatPackage) ){
						if (stopChatGet(socketfd, &stopChatPackage) < 0)
							return CLIENT_STATUS_CHAT_TOGETHER;
						else{
							return CLIENT_STATUS_CHAT_ALONE;
						}
					}
					break;
				}
				case PROTOCOL_OPTION_SENDFILE:{
					Protocol_sendfile sendFilePackage;
					if ( completeMessageWithHeader(socketfd, &header, &sendFilePackage) ){
						pthread_t thread;
						//sendFilePackage.message.body.tmp = socketfd;
						Protocol_sendfile *arg = (Protocol_sendfile*)malloc(sizeof(Protocol_sendfile));
						memcpy(arg, &sendFilePackage, sizeof(Protocol_sendfile));
						arg->message.body.tmp = socketfd;
						pthread_create(&thread, NULL, (void*)sendFileGet, (void*)arg);
						return clientStatusNow;
					}
					break;
				}
				default:{
					fprintf(stderr, "unknown option %x\n", header.head.option);
					return clientStatusNow;
					break;
				}
			}
		}
	}
}

int main(int argc, char const *argv[]){
	if (argc < 2){
		fprintf(OUTPUT_LOG, "[Usage] $./client [server_addr:port]\n");
		return 0;
	}
	/* Build Client Socket */
	int socketfd = buildClientSocket(argv[1]);
	if (socketfd == -1){
		fprintf(OUTPUT_LOG, "buildClientSocket fail\n");
		return 0;
	}
	/* ----- client identification ----- */
	int notLogIn = 1;
	fprintf(OUTPUT_LOG, "%d\n", argc);
	if (argc == 5 /*&& strcmp(argv[2], "-auto")*/){
		//fprintf(OUTPUT_LOG, "%s %s %s\n", argv[2], argv[3], argv[4]);
		strcpy(username, argv[3]);
		strcpy(password, argv[4]);
		if(logIn(socketfd)){
			notLogIn = 0;
		}
		//fprintf(OUTPUT_LOG, "notLogIn %d\n", notLogIn);
	}
	Client_status clientStatus = CLIENT_STATUS_CONNECTONLY;
	while(notLogIn){
		system("clear");
		char inputString[MAX_PROTOCOL_CONTENT_LEN];
		printf("Welcome new user.\nPlease enter sign-up[s] or login[l] or quit[q]\n");
		scanf("%s", inputString);
		if ( strcmp(inputString, "s") == 0 ){
			if (signedUp(socketfd))
				break;
		}
		else if ( strcmp(inputString, "l") == 0 ){
			printf("username: ");
			scanf("%s", username);
			printf("password: ");
			scanf("%s", password);
			if(logIn(socketfd)){
				notLogIn = 0;
				break;
			}
		}
		else if ( strcmp(inputString, "q") == 0 ){
			printf("bye bye ~\n");
			exit(1);
		}
		else
			printf("Unknown command, please enter again ...\n");
		sleep(SLEEP_TIME);
	}
	/* ----- client loop -----*/
	clientStatus = CLIENT_STATUS_WAIT;
	fd_set readset, working_readset;
	int status, maxfd = (socketfd > STDIN)? socketfd : STDIN;
	char inputString[MAX_PROTOCOL_CONTENT_LEN];
	FD_ZERO(&readset);
	FD_SET(socketfd, &readset);
	FD_SET(STDIN, &readset);
	system("clear");
	printf("You are in the lobby now.\nEnter specific command\n");
	printf("   - chat-with-friend[c]\n   - quit[q]\n   - good-friend show/add/delete[gf s/a/d]\n   - bad-friend show/add/delete[bf s/a/d]\n");
	while(1){
		while (clientStatus == CLIENT_STATUS_WAIT){
			while(clientStatus == CLIENT_STATUS_WAIT){
				memcpy(&working_readset, &readset, sizeof(readset));
				status = select(maxfd+1, &working_readset, NULL, NULL, 0);
				if( status > 0 ){
					if( FD_ISSET(STDIN, &working_readset) ){
						clientStatus = getInput(socketfd, clientStatus);
					}
					else if( FD_ISSET(socketfd, &working_readset) ){
						clientStatus = getPackage(socketfd, clientStatus);
					}
					//if (clientStatus == CLIENT_STATUS_WAIT)
					//	sleep(SLEEP_TIME);
					break;
				}
			}
		}
		while (clientStatus == CLIENT_STATUS_CHAT_TOGETHER || clientStatus == CLIENT_STATUS_CHAT_ALONE){
			system("clear");
			printf("You are in the chat room now.\nFeel free to chat with %s!\nEnter specific command\n   - history[$ h]\n   - send-file[$ s [fileName]]\n   - stop-chat[$ sc]\n", chatMateName);
			if (hasUnReadMsg){
				fprintf(OUTPUT_LOG, "===== un read message =====\n");
				write(1, unReadMsg, unReadMsgLen);
				fprintf(OUTPUT_LOG, "===========================\n");
				hasUnReadMsg = 0;
			}
			while(clientStatus == CLIENT_STATUS_CHAT_TOGETHER || clientStatus == CLIENT_STATUS_CHAT_ALONE){
				memcpy(&working_readset, &readset, sizeof(readset));
				status = select(maxfd+1, &working_readset, NULL, NULL, 0);
				if( status > 0 ){
					if( FD_ISSET(STDIN, &working_readset) ){
						clientStatus = getInput(socketfd, clientStatus);
					}
					else if( FD_ISSET(socketfd, &working_readset) ){
						if (clientStatus == CLIENT_STATUS_CHAT_TOGETHER){
							clientStatus = getPackage(socketfd, clientStatus);
							if (clientStatus == CLIENT_STATUS_CHAT_ALONE){
								printf("user '%s' is not online, but you can still send the message.\n", chatMateName);
							}
						}
						else if (clientStatus == CLIENT_STATUS_CHAT_ALONE){
							clientStatus = getPackage(socketfd, clientStatus);
							if (clientStatus == CLIENT_STATUS_CHAT_TOGETHER)
								printf("user '%s' is online, now.\n", chatMateName);
						}
					}
				}
				//fprintf(OUTPUT_LOG, "CLIENT_STATUS_WAIT: %d\n", (clientStatus == CLIENT_STATUS_WAIT));
				//fprintf(OUTPUT_LOG, "CLIENT_STATUS_CHAT_ALONE: %d\n", (clientStatus == CLIENT_STATUS_CHAT_ALONE));
				//fprintf(OUTPUT_LOG, "CLIENT_STATUS_CHAT_TOGETHER: %d\n", (clientStatus == CLIENT_STATUS_CHAT_TOGETHER));
				if (clientStatus == CLIENT_STATUS_WAIT){
					system("clear");
					printf("You are in the lobby now.\nEnter specific command\n");
					printf("   - chat-with-friend[c]\n   - quit[q]\n   - good-friend show/add/delete[gf s/a/d]\n   - bad-friend show/add/delete[bf s/a/d]\n");
				}
			}
		}
	}
	return 0;
}