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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include "common.h"

Client_status clientsStatus[MAX_ONLINE_CLIENT];
int clientSockfdTableSize;
int clientSockfdTable[MAX_ONLINE_CLIENT];
char clientIpTable[MAX_ONLINE_CLIENT][64];
int clientPortTable[MAX_ONLINE_CLIENT];
int isUseable[MAX_ONLINE_CLIENT];
char clientUsername[MAX_ONLINE_CLIENT][MAX_PROTOCOL_NAME_LEN];
char clientChatToName[MAX_ONLINE_CLIENT][MAX_PROTOCOL_NAME_LEN];
char sendFileNameTable[MAX_ONLINE_CLIENT][MAX_PROTOCOL_NAME_LEN];
FILE* chatHistory[MAX_ONLINE_CLIENT];

typedef struct linkList{
	char username[MAX_PROTOCOL_NAME_LEN];
	struct linkList *next;
} LinkList;

int isInTheFriendFILE(char *fileName, char *usernameTarget){
	FILE *fp = fopen(fileName, "r");
	if (fp == NULL)
		return 0;
	char tmpUserName[MAX_PROTOCOL_NAME_LEN];
	while ( (fscanf(fp, "%s", tmpUserName) != -1) ){
		if (strcmp(tmpUserName, usernameTarget) == 0){
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

void addToFriendFile(char *fileName, char *usernameTarget){
	FILE *fp = fopen(fileName, "a");
	if (fp == NULL)
		return;
	fprintf(fp, "%s\n", usernameTarget);
	fclose(fp);
	return;
}

void rmFromFriendFile(char *fileName, char *usernameTarget){
	FILE *fp = fopen(fileName, "r");
	int start = 1;
	LinkList *root = NULL, *tail, *tmp;
	if (fp == NULL)
		return;
	char tmpUserName[MAX_PROTOCOL_NAME_LEN];
	while ( (fscanf(fp, "%s", tmpUserName) != -1) ){
		if (strcmp(tmpUserName, usernameTarget) != 0){
			if (start){
				start = 0;
				tail = (LinkList*)malloc(sizeof(LinkList));
				root = tail;
			}
			else{
				tail -> next = (LinkList*)malloc(sizeof(LinkList));
				tail = tail -> next;
			}
			strcpy(tail -> username, tmpUserName);
			tail -> next = NULL;
		}
	}
	fclose(fp);
	fp = fopen(fileName, "w");
	for (tmp = root; tmp != NULL; tmp = tmp -> next){
		fprintf(OUTPUT_LOG, "%s\n", tmp -> username );
	}
	for (tmp = root; tmp != NULL; tmp = tmp -> next){
		fprintf(fp, "%s\n", tmp -> username );
	}
	tmp = root;
	while (tmp != NULL){
		tail = tmp;
		tmp = tmp->next;
		free(tail);
	}
	fclose(fp);
	return;
}
int buildServerSocket(const char *port){
	int socketfd, status;
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me
	getaddrinfo(NULL, port, &hints, &res);
	socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (socketfd == -1){
		fprintf(OUTPUT_LOG, "Fail to create a socket.\n");
		return -1;
	}

	status = bind(socketfd, res->ai_addr, res->ai_addrlen);
	if (status == -1){
		fprintf(OUTPUT_LOG, "bind fail\n");
		return -1;
	}

	status = listen(socketfd, MAX_ONLINE_CLIENT);
	if (status == -1){
		fprintf(OUTPUT_LOG, "listen fail\n");
		return -1;
	}
	return socketfd;
}

int addNewClient(int clientfd, int maxfd){
	struct sockaddr_in c;
	socklen_t cLen = sizeof(c);
	int choosePlace = 0;
	while (isUseable[choosePlace])
		choosePlace ++;
	if (choosePlace == clientSockfdTableSize) clientSockfdTableSize ++;
	getpeername(clientfd, (struct sockaddr*) &c, &cLen); 
	strcpy(clientIpTable[choosePlace], inet_ntoa(c.sin_addr));
	clientPortTable[choosePlace] = ntohs(c.sin_port);
	fprintf(OUTPUT_LOG, "%s %d\n", clientIpTable[choosePlace], clientPortTable[choosePlace]);
	clientSockfdTable[choosePlace] = clientfd;
	isUseable[choosePlace] = 1;
    clientsStatus[choosePlace] = CLIENT_STATUS_CONNECTONLY;
	return (clientfd > maxfd)? clientfd : maxfd;
}

int getChatMateIdx(char *username){
	for (int i = 0; i < clientSockfdTableSize; i ++){
		if( isUseable[i] && strcmp(username, clientUsername[i]) == 0 && clientsStatus[i] != CLIENT_STATUS_CONNECTONLY && clientsStatus[i] != CLIENT_STATUS_SENDFILE){
			return i;
		}
	}
	return -1;
}

FILE* getConfigFile(Protocol_option option){
	DIR* dir = opendir("config");
	if(ENOENT == errno){
		mkdir("config", S_IRWXU | S_IRWXG | S_IRWXO); 
	}
	/*
	else{
		fprintf(OUTPUT_LOG, "config dir check fail\n");
		return NULL;
	}
	*/
	closedir(dir);
	FILE *fp;
	if (option == PROTOCOL_OPTION_SIGNUP){
		fp = fopen("./config/userConfig.txt", "a");
	}
	else if (option == PROTOCOL_OPTION_LOGIN){
		fp = fopen("./config/userConfig.txt", "r");
	}
	return fp;
}

void getChatHistoryFileName(char *resultName, char *username1, char *username2){
	int cmpResult = strcmp(username1, username2);
	strcpy(resultName, "./chatHistory/");
	if(cmpResult < 0){
		strcat(resultName, username1);
		strcat(resultName, "_");
		strcat(resultName, username2);
	}
	else{
		strcat(resultName, username2);
		strcat(resultName, "_");
		strcat(resultName, username1);
	}
	strcat(resultName, ".txt");
	return;
}

FILE* getChatHistoryFile(char *username1, char *username2){
	DIR* dir = opendir("chatHistory");
	if(ENOENT == errno){
		mkdir("chatHistory", S_IRWXU | S_IRWXG | S_IRWXO); 
	}
	/*
	else{
		fprintf(OUTPUT_LOG, "chatHistory dir check fail\n");
		return NULL;
	}
	*/
	closedir(dir);
	FILE *fp;
	char chatHistoryFileName[MAX_PROTOCOL_NAME_LEN * 3];
	getChatHistoryFileName(chatHistoryFileName, username1, username2);
	fp = fopen(chatHistoryFileName, "a+");
	return fp;
}

int hasUserSignedUP(char *usernameTarget, char *passwordTarget){
	int cmpType = (passwordTarget == NULL)?0:1;
	fprintf(OUTPUT_LOG, "in the hasUserSignedUP, cmpType = %d\n", cmpType);
	char tmpUserName[MAX_PROTOCOL_NAME_LEN], tmpPassword[MAX_PROTOCOL_NAME_LEN];
	FILE *fp = getConfigFile(PROTOCOL_OPTION_LOGIN);
	if (fp != NULL){
		while(fscanf(fp, "%s %s", tmpUserName, tmpPassword) != -1){
			//fprintf(OUTPUT_LOG, "%s %s\n", tmpUserName, tmpPassword);
			if(strcmp(tmpUserName, usernameTarget) == 0 ){
				if (cmpType == 0){
					fprintf(OUTPUT_LOG, "match in the config file, no password\n");
					fclose(fp);
					return 0;
				}
				else if (cmpType == 1 && strcmp(tmpPassword, passwordTarget) == 0){
					fprintf(OUTPUT_LOG, "match in the config file, has password\n");
					fclose(fp);
					return 0;
				}
			}
		}
		fprintf(OUTPUT_LOG, "no match in the config file\n");
		fclose(fp);
	}
	else{
		fprintf(OUTPUT_LOG, "hasUserSignedUP fail in open file\n");
	}
	return -1;
}

int signUp(Protocol_signup *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> In the signUp\n");
	int status;
	FILE *fp;
	if( hasUserSignedUP((char*)recvPackage->message.body.username, NULL) == 0 ){
		fprintf(OUTPUT_LOG, "other user use the username\n");
		status = -1;
	}
	else{
		fprintf(OUTPUT_LOG, "signUp\n");
		fp = getConfigFile(PROTOCOL_OPTION_SIGNUP);
		fprintf(OUTPUT_LOG, "%s, %s\n", recvPackage->message.body.username, recvPackage->message.body.password);
		status = fprintf(fp, "%s %s\n", recvPackage->message.body.username, recvPackage->message.body.password);
		fclose(fp);
	}
	Protocol_signup sendPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_SIGNUP;
	if (status < 0){
		fprintf(OUTPUT_LOG, "signUp fail\n");
		sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
	}
	else{
		fprintf(OUTPUT_LOG, "signUp success\n");
		strcpy(clientUsername[clientIdx], recvPackage->message.body.username);
		sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
		sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	}
	do {
		status = send(clientSockfdTable[clientIdx], &sendPackage, sizeof(sendPackage), 0);
	} while(status == -1);
	return (status != -1);
}

int logIn(Protocol_login *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> In the logIn\n");
	fprintf(OUTPUT_LOG, "%s %s\n", recvPackage->message.body.username, recvPackage->message.body.password);
	char clientUsernameTmp[MAX_PROTOCOL_NAME_LEN];
	strcpy(clientUsernameTmp, (char*)recvPackage->message.body.username);
	int status1, status2, status;
	status1 = (hasUserSignedUP(clientUsernameTmp, recvPackage->message.body.password) == 0);
	status2 = (getChatMateIdx(clientUsernameTmp) == -1 );
	fprintf(OUTPUT_LOG, "status1: %d, status2: %d\n", status1, status2);
	Protocol_login sendPackage;
	sendPackage.message.header.head.option = PROTOCOL_OPTION_LOGIN;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	if ( status1 && status2 ){
		fprintf(OUTPUT_LOG, "logIn success\n");
		strcpy(clientUsername[clientIdx], clientUsernameTmp);
		sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
	}
	else{
		fprintf(OUTPUT_LOG, "logIn fail\n");
		sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
		if (status1 == 0)
			strcpy(sendPackage.message.body.username, "passwordIncorrect");
		else if (status2 == 0)
			strcpy(sendPackage.message.body.username, "alreadyLogIn");
	}
	do {
		status = send(clientSockfdTable[clientIdx], &sendPackage, sizeof(sendPackage), 0);
	} while(status == -1);
	if (status1 && status2)
		return 1;
	else
		return 0;
}

void getUnReadMsg(Protocol_startchat *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> in the getUnReadMsg\n");
	int chatMateIdx, offsetRead, offsetWrite, base, readLen, stringTargetLen;
	int fpRead, fpWrite;
	char stringRead[MAX_PROTOCOL_CONTENT_LEN], stringTarget[MAX_PROTOCOL_CONTENT_LEN];
	char chatHistoryFileName[MAX_PROTOCOL_NAME_LEN * 3];
	char buffer[MAX_PROTOCOL_FILE_CONTENT_LEN];
	sprintf(stringTarget, "====%s_not_read_below====\n", clientUsername[clientIdx]);
	stringTargetLen = strlen(stringTarget);
	//fprintf(OUTPUT_LOG, "%s\n", stringTarget);
	if ( clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_TOGETHER ){
		chatMateIdx = getChatMateIdx(clientChatToName[clientIdx]);
	}
	fclose(chatHistory[clientIdx]);
	getChatHistoryFileName(chatHistoryFileName, clientUsername[clientIdx], clientChatToName[clientIdx]);
	fpRead = open(chatHistoryFileName, O_RDONLY);
	offsetRead = 0;
	while(readLen = read(fpRead, buffer + offsetRead, MAX_PROTOCOL_FILE_CONTENT_LEN)){
		offsetRead += readLen;
	}
	buffer[offsetRead] = '\0';
	close(fpRead);
	//fprintf(OUTPUT_LOG, "%d\n", offset);
	offsetRead = 0;
	base = -1;
	while (1){
		if(sscanf(buffer + offsetRead, "%s", stringRead) == -1)
			break;
		//fprintf(OUTPUT_LOG, "%s, %d, %d\n", stringRead, offset, strlen(stringRead));
		strcat(stringRead, "\n");
		if ( strcmp(stringRead, stringTarget) == 0 ){
			fprintf(OUTPUT_LOG, "match\n");
			offsetWrite = offsetRead;
			base = offsetRead;
			offsetRead += strlen(stringRead);
			break;
		}
		offsetRead += strlen(stringRead);
	}
	if (base != -1){
		while(1){
			if(sscanf(buffer + offsetRead, "%s", stringRead) == -1)
				break;
			//fprintf(OUTPUT_LOG, "%s, %d\n", stringRead, strlen(stringRead));
			strcat(stringRead, "\n");
			strcpy(buffer + offsetWrite, stringRead);
			offsetRead += strlen(stringRead);
			offsetWrite += strlen(stringRead);
		}
		//write(1, buffer, offsetWrite * sizeof(char));
		//fprintf(OUTPUT_LOG, "====\n");
		//write(1, buffer + base, (offsetWrite - base) * sizeof(char));
		//write(1, buffer, offsetWrite * sizeof(char));
		//exit(1);
		fpWrite = open(chatHistoryFileName, O_WRONLY | O_TRUNC );
		write(fpWrite, buffer, offsetWrite * sizeof(char));
		close(fpWrite);
		recvPackage->message.body.unReadMsgLen = (offsetWrite - base);
		memcpy(recvPackage->message.body.unReadMsg, buffer + base, (offsetWrite - base) * sizeof(char));
	}
	else{
		recvPackage->message.body.unReadMsgLen = 0;
	}
	chatHistory[clientIdx] = getChatHistoryFile(clientUsername[clientIdx], clientChatToName[clientIdx]);
	if ( clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_TOGETHER ){
		chatHistory[chatMateIdx] = chatHistory[clientIdx];
	}
	return;
}

void startChat(Protocol_startchat *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> In the startChat\n");
	char chatMateName[MAX_PROTOCOL_NAME_LEN];
	strcpy(chatMateName, recvPackage->message.body.chatMateName);
	int isChatMateOnline = 0, chatMateIdx, requestChatResult = 0, status = -1;
	Protocol_startchat sendPackageBack;
	memset(&sendPackageBack, 0, sizeof(sendPackageBack));
	sendPackageBack.message.header.head.option = PROTOCOL_OPTION_STARTCHAT;
	sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
	/* requestChatResult
	0 for reject, 1 for acc
	*/
	char fileName[MAX_PROTOCOL_NAME_LEN];
	sprintf(fileName, "./config/%s_bf.txt", chatMateName);
	if( !isInTheFriendFILE(fileName, clientUsername[clientIdx]) && hasUserSignedUP(chatMateName, NULL) == 0 && strcmp(clientUsername[clientIdx], chatMateName) != 0 ){
		chatMateIdx = getChatMateIdx(chatMateName);
		isChatMateOnline = (chatMateIdx != -1)? 1: 0;
		if ( isChatMateOnline && ( clientsStatus[chatMateIdx] == CLIENT_STATUS_WAIT ||
								 ( clientsStatus[chatMateIdx] == CLIENT_STATUS_CHAT_ALONE && strcmp(clientChatToName[chatMateIdx], clientUsername[clientIdx]) == 0) ) ){
			fprintf(OUTPUT_LOG, "to clientUsername %s, chatMateName %s is online\n", clientUsername[clientIdx], chatMateName);
			Protocol_startchat sendPackageToChatMate, recvPackageFromChatMate;
			memset(&sendPackageToChatMate, 0, sizeof(sendPackageToChatMate));
			sendPackageToChatMate.message.header.head.option = PROTOCOL_OPTION_STARTCHAT;
			sendPackageToChatMate.message.header.head.status = PROTOCOL_STATUS_REQ;
			sendPackageToChatMate.message.header.head.datalen = sizeof(sendPackageToChatMate) - sizeof(Protocol_header);
			strcpy(sendPackageToChatMate.message.body.chatMateName, clientUsername[clientIdx]);
			do {
				status = send(clientSockfdTable[chatMateIdx], &sendPackageToChatMate, sizeof(sendPackageToChatMate), 0);
			} while (status == -1);
			fprintf(OUTPUT_LOG, "send sendPackageToChatMate\n");
			status = recv(clientSockfdTable[chatMateIdx], &recvPackageFromChatMate, sizeof(recvPackageFromChatMate), MSG_WAITALL);
			fprintf(OUTPUT_LOG, "get recvPackageFromChatMate\n");
			if( status == 0){
				fprintf(OUTPUT_LOG, "cliet %s is not online anymore.\n", clientUsername[chatMateIdx]);
			}
			if(recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_FAIL){
				strcpy(sendPackageBack.message.body.chatMateName, clientUsername[clientIdx]);
			}
			else if(status > 0){
				if ( clientsStatus[chatMateIdx] == CLIENT_STATUS_WAIT  && recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_SUCC){
					FILE *fp = getChatHistoryFile(clientUsername[clientIdx], chatMateName);
					chatHistory[clientIdx] = fp;
					chatHistory[chatMateIdx] = fp;
					clientsStatus[clientIdx] = CLIENT_STATUS_CHAT_TOGETHER;
					clientsStatus[chatMateIdx] = CLIENT_STATUS_CHAT_TOGETHER;
					strcpy(clientChatToName[clientIdx], chatMateName);
					strcpy(clientChatToName[chatMateIdx], clientUsername[clientIdx]);
				}
				else if ( clientsStatus[chatMateIdx] == CLIENT_STATUS_CHAT_ALONE ){
					chatHistory[clientIdx] = chatHistory[chatMateIdx];
					clientsStatus[clientIdx] = CLIENT_STATUS_CHAT_TOGETHER;
					clientsStatus[chatMateIdx] = CLIENT_STATUS_CHAT_TOGETHER;
					strcpy(clientChatToName[clientIdx], chatMateName);

				}
				requestChatResult = 1;
				sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
				strcpy(sendPackageBack.message.body.chatMateName, chatMateName);
			}
		}
		else if ( !isChatMateOnline ||
				( isChatMateOnline && clientsStatus[chatMateIdx] == CLIENT_STATUS_CHAT_TOGETHER) ){
			// chatMate not online or chatMate is already chatting.
			fprintf(OUTPUT_LOG, "to clientUsername %s, chatMateName %s is not online or already chatting\n", clientUsername[clientIdx], chatMateName);
			FILE *fp = getChatHistoryFile(clientUsername[clientIdx], chatMateName);
			chatHistory[clientIdx] = fp;
			clientsStatus[clientIdx] = CLIENT_STATUS_CHAT_ALONE;
			strcpy(clientChatToName[clientIdx], chatMateName);
			requestChatResult = 1;
			sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
			strcpy(sendPackageBack.message.body.chatMateName, clientUsername[clientIdx]);
		}
		else{
			fprintf(OUTPUT_LOG, "==== error ====\n");
			fprintf(OUTPUT_LOG, "%d (%s) isChatMateOnline = %d\n",chatMateIdx, clientUsername[chatMateIdx], isChatMateOnline);
			if (clientsStatus[chatMateIdx] == CLIENT_STATUS_WAIT)
				fprintf(OUTPUT_LOG, "CLIENT_STATUS_WAIT\n");
			else if (clientsStatus[chatMateIdx] == CLIENT_STATUS_CHAT_ALONE)
				fprintf(OUTPUT_LOG, "CLIENT_STATUS_CHAT_ALONE, with %s\n", clientChatToName[chatMateIdx]);
			else if (clientsStatus[chatMateIdx] == CLIENT_STATUS_CHAT_TOGETHER)
				fprintf(OUTPUT_LOG, "CLIENT_STATUS_CHAT_TOGETHER, with %s\n", clientChatToName[chatMateIdx]);
			else if (clientsStatus[chatMateIdx] == CLIENT_STATUS_CONNECTONLY)
				fprintf(OUTPUT_LOG, "CLIENT_STATUS_CONNECTONLY\n");
			else if (clientsStatus[chatMateIdx] == CLIENT_STATUS_SENDFILE)
				fprintf(OUTPUT_LOG, "CLIENT_STATUS_SENDFILE\n");
			fprintf(OUTPUT_LOG, "==== error ====\n");
		}
	}
	if ( requestChatResult == 0 )
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_FAIL;
	else{
		getUnReadMsg(&sendPackageBack, clientIdx);
		if (clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_ALONE)
			fprintf(chatHistory[clientIdx], "====%s_not_read_below====\n", clientChatToName[clientIdx]);
	}
	do {
		fprintf(OUTPUT_LOG, "ready to send sendPackageBack\n");
		status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
		fprintf(OUTPUT_LOG, "finish to sendPackageBack\n");
	} while(status == -1);
	return;
}

void sendMsg(Protocol_sendmsg *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> in the sendMsg\n");
	int status = 0;
	if ( clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_TOGETHER ){
		int chatMateIdx = getChatMateIdx(clientChatToName[clientIdx]);
		Protocol_sendmsg sendPackageToChatMate, recvPackageFromChatMate;
		sendPackageToChatMate.message.header.head.option = PROTOCOL_OPTION_SENDMSG;
		sendPackageToChatMate.message.header.head.status = PROTOCOL_STATUS_REQ;
		sendPackageToChatMate.message.header.head.datalen = sizeof(sendPackageToChatMate) - sizeof(Protocol_header);
		strcpy(sendPackageToChatMate.message.body.content, recvPackage->message.body.content);
		do{
			status = send(clientSockfdTable[chatMateIdx], &sendPackageToChatMate, sizeof(sendPackageToChatMate), 0);
		}while(status == -1);
		fprintf(OUTPUT_LOG, "send sendPackageToChatMate\n");
		status = recv(clientSockfdTable[chatMateIdx], &recvPackageFromChatMate, sizeof(recvPackageFromChatMate), MSG_WAITALL);
		fprintf(OUTPUT_LOG, "get recvPackageFromChatMate\n");
		if (status == 0){
			fprintf(OUTPUT_LOG, "cliet %s is not online anymore.\n", clientUsername[chatMateIdx]);
			status = -1;
			// condition: 聊到一半有人斷線。
		}
		else if (recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_SUCC){
			status = 0;
		}
		else{
			status = -1;
			fprintf(OUTPUT_LOG, "send message fail\n");
		}
	}
	Protocol_sendmsg sendPackageBack;
	sendPackageBack.message.header.head.option = PROTOCOL_OPTION_SENDMSG;
	if (status == 0)
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
	else
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_FAIL;
	sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
	do{
		status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
	}while(status == -1);
	if(fprintf(chatHistory[clientIdx], "%s:%s\n", clientUsername[clientIdx], recvPackage->message.body.content) < 0)
		fprintf(OUTPUT_LOG, "record to log fail\n");
	fflush(chatHistory[clientIdx]);
	return;
}

void stopChat(Protocol_stopchat *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> in the stopChat\n");
	int status = 0;
	if ( clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_TOGETHER ){
		int chatMateIdx = getChatMateIdx(clientChatToName[clientIdx]);
		Protocol_stopchat sendPackageToChatMate, recvPackageFromChatMate;
		sendPackageToChatMate.message.header.head.option = PROTOCOL_OPTION_STOPCHAT;
		sendPackageToChatMate.message.header.head.status = PROTOCOL_STATUS_REQ;
		sendPackageToChatMate.message.header.head.datalen = sizeof(sendPackageToChatMate) - sizeof(Protocol_header);
		do{
			status = send(clientSockfdTable[chatMateIdx], &sendPackageToChatMate, sizeof(sendPackageToChatMate), 0);
		}while(status == -1);
		fprintf(OUTPUT_LOG, "send sendPackageToChatMate\n");
		status = recv(clientSockfdTable[chatMateIdx], &recvPackageFromChatMate, sizeof(recvPackageFromChatMate), MSG_WAITALL);
		fprintf(OUTPUT_LOG, "get recvPackageFromChatMate\n");
		if (status == 0){
			fprintf(OUTPUT_LOG, "cliet %s is not online anymore.\n", clientUsername[chatMateIdx]);
			status = -1;
			// condition: 聊到一半有人斷線。
		}
		else if (recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_SUCC){
			status = 0;
		}
		else{
			status = -1;
			fprintf(OUTPUT_LOG, "send message fail\n");
		}
	}
	Protocol_stopchat sendPackageBack;
	sendPackageBack.message.header.head.option = PROTOCOL_OPTION_STOPCHAT;
	if (status == 0)
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
	else
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_FAIL;
	sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
	do{
		status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
	}while(status == -1);
	if (clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_TOGETHER){
		int chatMateIdx = getChatMateIdx(clientChatToName[clientIdx]);
		clientsStatus[clientIdx] = CLIENT_STATUS_WAIT;
		clientsStatus[chatMateIdx] = CLIENT_STATUS_CHAT_ALONE;
		fprintf(chatHistory[clientIdx], "====%s_not_read_below====\n", clientUsername[clientIdx]);
	}
	else if (clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_ALONE){
		clientsStatus[clientIdx] = CLIENT_STATUS_WAIT;
		fclose(chatHistory[clientIdx]);
	}
	else{
		fprintf(OUTPUT_LOG, "unknown state\n");
	}
	return;
}

void quit(Protocol_quit *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> in the quit\n");
	isUseable[clientIdx] = 0;
	int status;
	Protocol_quit sendPackageBack;
	sendPackageBack.message.header.head.option = PROTOCOL_OPTION_QUIT;
	sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
	if ( clientsStatus[clientIdx] == CLIENT_STATUS_WAIT ){
		if (recvPackage != NULL){
			sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
			do{
				status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
			}while(status == -1);
		}
	}
	else if (clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_ALONE){
		if (recvPackage != NULL){
			sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
			do{
				status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
			}while(status == -1);
		}
		fclose(chatHistory[clientIdx]);
	}
	else if (clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_TOGETHER){
		int chatMateIdx = getChatMateIdx(clientChatToName[clientIdx]);
		Protocol_stopchat sendPackageToChatMate, recvPackageFromChatMate;
		sendPackageToChatMate.message.header.head.option = PROTOCOL_OPTION_STOPCHAT;
		sendPackageToChatMate.message.header.head.status = PROTOCOL_STATUS_REQ;
		sendPackageToChatMate.message.header.head.datalen = sizeof(sendPackageToChatMate) - sizeof(Protocol_header);
		do{
			status = send(clientSockfdTable[chatMateIdx], &sendPackageToChatMate, sizeof(sendPackageToChatMate), 0);
		}while(status == -1);
		fprintf(OUTPUT_LOG, "send sendPackageToChatMate\n");
		status = recv(clientSockfdTable[chatMateIdx], &recvPackageFromChatMate, sizeof(recvPackageFromChatMate), MSG_WAITALL);
		fprintf(OUTPUT_LOG, "get recvPackageFromChatMate\n");
		if (status == 0){
			fprintf(OUTPUT_LOG, "cliet %s is not online anymore.\n", clientUsername[chatMateIdx]);
			status = -1;
			// condition: 聊到一半有人斷線。
		}
		else if (recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_SUCC){
			status = 0;
		}
		else {
			status = -1;
			fprintf(OUTPUT_LOG, "send message fail\n");
		}
		if (recvPackage != NULL){
			if (status == 0)
				sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
			else
				sendPackageBack.message.header.head.status = PROTOCOL_STATUS_FAIL;
			do{
				status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
			}while(status == -1);
		}
		fprintf(chatHistory[clientIdx], "====%s_not_read_below====\n", clientUsername[clientIdx]);
		clientsStatus[chatMateIdx] = CLIENT_STATUS_CHAT_ALONE;
	}
}

int sendFile(Protocol_sendfile *recvPackage, int clientIdx, fd_set *readset, int maxfd, int socketfd){
	fprintf(OUTPUT_LOG, "> in the sendFile\n");
	fprintf(OUTPUT_LOG, "fileName '%s'\n", recvPackage->message.body.fileName);
	int status;
	Protocol_sendfile sendPackageBack;
	sendPackageBack.message.header.head.option = PROTOCOL_OPTION_SENDFILE;
	if (clientsStatus[clientIdx] == CLIENT_STATUS_CHAT_TOGETHER){
		int chatMateIdx = getChatMateIdx(clientChatToName[clientIdx]);
		fprintf(OUTPUT_LOG, "clientIdx %d, chatMateIdx %d\n", clientIdx, chatMateIdx);
		int choosePlaceRecv, choosePlaceSend;
		Protocol_sendfile sendPackageToChatMate, recvPackageFromChatMate;
		sendPackageToChatMate.message.header.head.option = PROTOCOL_OPTION_SENDFILE;
		sendPackageToChatMate.message.header.head.status = PROTOCOL_STATUS_REQ;
		sendPackageToChatMate.message.header.head.datalen = sizeof(sendPackageToChatMate) - sizeof(Protocol_header);
		strcpy(sendPackageToChatMate.message.body.fileName, recvPackage->message.body.fileName);
		do{
			status = send(clientSockfdTable[chatMateIdx], &sendPackageToChatMate, sizeof(sendPackageToChatMate), 0);
		}while(status == -1);
		//fprintf(OUTPUT_LOG, "send sendPackageToChatMate\n");
		/* ---- send socket handle ---- */
		struct sockaddr_in client_addr;
	    socklen_t addrlen = sizeof(client_addr);
		choosePlaceSend = 0;
		while (isUseable[choosePlaceSend])
			choosePlaceSend ++;
		strcpy(sendFileNameTable[choosePlaceSend], recvPackage->message.body.fileName);
		if (choosePlaceSend == clientSockfdTableSize) clientSockfdTableSize ++;
		isUseable[choosePlaceSend] = 1;
		clientsStatus[choosePlaceSend] = CLIENT_STATUS_SENDFILE;
		strcpy(clientUsername[choosePlaceSend], clientUsername[chatMateIdx]);
		strcpy(clientChatToName[choosePlaceSend], clientUsername[clientIdx]);
		clientSockfdTable[choosePlaceSend] = accept(socketfd, (struct sockaddr*) &client_addr, &addrlen);
		if ( clientSockfdTable[choosePlaceSend] == -1)
			fprintf(OUTPUT_LOG, "accept fail\n");
		else
			fprintf(OUTPUT_LOG, "accept success, %d\n", clientSockfdTable[choosePlaceSend]);
		FD_SET(clientSockfdTable[choosePlaceSend], readset);
		maxfd = (clientSockfdTable[choosePlaceSend] > maxfd)? clientSockfdTable[choosePlaceSend]: maxfd;
		/* ---- recv ---- */
		status = recv(clientSockfdTable[chatMateIdx], &recvPackageFromChatMate, sizeof(recvPackageFromChatMate), MSG_WAITALL);
		fprintf(OUTPUT_LOG, "get recvPackageFromChatMate\n");
		if (status == 0){
			fprintf(OUTPUT_LOG, "cliet %s is not online anymore.\n", clientUsername[chatMateIdx]);
			status = -1;
			// condition: 聊到一半有人斷線。
		}
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
		sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
		do{
			status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
		}while(status == -1);
		/* ----- recv socket handle ----- */
		choosePlaceRecv = 0;
		while (isUseable[choosePlaceRecv])
			choosePlaceRecv ++;
		if (choosePlaceRecv == clientSockfdTableSize) clientSockfdTableSize ++;
		isUseable[choosePlaceRecv] = 1;
		clientsStatus[choosePlaceRecv] = CLIENT_STATUS_SENDFILE;
		strcpy(clientUsername[choosePlaceRecv], clientUsername[clientIdx]);
		strcpy(clientChatToName[choosePlaceRecv], clientUsername[chatMateIdx]);
		clientSockfdTable[choosePlaceRecv] = accept(socketfd, (struct sockaddr*) &client_addr, &addrlen);
		if ( clientSockfdTable[choosePlaceRecv] == -1)
			fprintf(OUTPUT_LOG, "accept fail\n");
		else
			fprintf(OUTPUT_LOG, "accept success, %d\n", clientSockfdTable[choosePlaceRecv]);
		FD_SET(clientSockfdTable[choosePlaceRecv], readset);
		maxfd = (clientSockfdTable[choosePlaceRecv] > maxfd)? clientSockfdTable[choosePlaceRecv]: maxfd;
	}
	else{
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_FAIL;
		sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
		do{
			status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
		}while(status == -1);
	}
	//sleep(SLEEP_TIME);
	return maxfd;
}

void sendFileContent(Protocol_sendfilecontent *recvPackage, int clientIdx, fd_set *readset){
	fprintf(OUTPUT_LOG, "> in the sendFileContent\n");
	//fprintf(OUTPUT_LOG, "clientUsername[clientIdx] = %s\n", clientUsername[clientIdx]);
	//fprintf(OUTPUT_LOG, "%s\n", recvPackage->message.body.fileName);
	int chatMateIdx = -1, status = -1;
	//char chatMateName[MAX_PROTOCOL_NAME_LEN];
	//strcpy(chatMateName, clientChatToName[clientIdx]);
	for (int i = 0; i < clientSockfdTableSize; i++){
		//fprintf(OUTPUT_LOG, "to client %s, clientChatToName %s\n", clientUsername[i], clientChatToName[i]);
		//fprintf(OUTPUT_LOG, "%s\n", sendFileNameTable[i]);
		if (isUseable[i] && clientsStatus[i] == CLIENT_STATUS_SENDFILE && strcmp(clientChatToName[i], clientUsername[clientIdx]) == 0 && strcmp(sendFileNameTable[i], recvPackage->message.body.fileName) == 0){
			chatMateIdx = i;
			break;
		}
	}
	fprintf(OUTPUT_LOG, "sendFileContent clientIdx is %s(%d),  chatMateIdx is %s(%d)\n", clientUsername[clientIdx], clientIdx, clientChatToName[clientIdx], chatMateIdx);
	//fprintf(OUTPUT_LOG, "%s send file content to %s\n", clientUsername[clientIdx], clientUsername[chatMateIdx]);
	if (chatMateIdx != -1){
		Protocol_sendfilecontent sendPackageToChatMate, recvPackageFromChatMate;
		sendPackageToChatMate.message.header.head.option = PROTOCOL_OPTION_SENDFILECONTENT;
		sendPackageToChatMate.message.header.head.status = PROTOCOL_STATUS_REQ;
		sendPackageToChatMate.message.header.head.datalen = sizeof(sendPackageToChatMate) - sizeof(Protocol_header);
		sendPackageToChatMate.message.body.fileContentLen = recvPackage->message.body.fileContentLen;
		sendPackageToChatMate.message.body.fragflag = recvPackage->message.body.fragflag;
		memcpy(sendPackageToChatMate.message.body.fileContent, recvPackage->message.body.fileContent, recvPackage->message.body.fileContentLen);
		do{
			status = send(clientSockfdTable[chatMateIdx], &sendPackageToChatMate, sizeof(sendPackageToChatMate), 0);
		}while(status == -1);
		//fprintf(OUTPUT_LOG, "send sendPackageToChatMate to %s, %d\n", clientUsername[chatMateIdx], clientSockfdTable[chatMateIdx]);
		status = recv(clientSockfdTable[chatMateIdx], &recvPackageFromChatMate, sizeof(recvPackageFromChatMate), MSG_WAITALL);
		//fprintf(OUTPUT_LOG, "get recvPackageFromChatMate, %d\n", clientSockfdTable[chatMateIdx]);
		if (status == 0){
			fprintf(OUTPUT_LOG, "cliet %s is not online anymore.\n", clientUsername[chatMateIdx]);
			status = -1;
			// condition: 聊到一半有人斷線。
		}
		else if (recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_SUCC)
			status = 0;
		else{
			if (recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_SUCC)
				fprintf(OUTPUT_LOG, "recvPackageFromChatMate.status == PROTOCOL_STATUS_SUCC\n");
			else if (recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_FAIL)
				fprintf(OUTPUT_LOG, "recvPackageFromChatMate.status == PROTOCOL_STATUS_FAIL\n");
			else if (recvPackageFromChatMate.message.header.head.status == PROTOCOL_STATUS_REQ)
				fprintf(OUTPUT_LOG, "recvPackageFromChatMate.status == PROTOCOL_STATUS_REQ\n");
			else
				fprintf(OUTPUT_LOG, "status == %d\n", status);
			status = -1;
			fprintf(OUTPUT_LOG, "send message fail\n");
		}
	}
	Protocol_sendfilecontent sendPackageBack;
	sendPackageBack.message.header.head.option = PROTOCOL_OPTION_SENDFILECONTENT;
	if (status != -1)
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
	else{
		fprintf(OUTPUT_LOG, "sendFileContent Fail\n");
		sendPackageBack.message.header.head.status = PROTOCOL_STATUS_FAIL;
	}
	sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
	do{
		status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
	}while(status == -1);
	if (chatMateIdx != -1 && recvPackage->message.body.fragflag == 0){
		fprintf(OUTPUT_LOG, "clear readset\n");
		FD_CLR(clientSockfdTable[clientIdx], readset);
		FD_CLR(clientSockfdTable[chatMateIdx], readset);
		close(clientSockfdTable[clientIdx]);
		close(clientSockfdTable[chatMateIdx]);
		isUseable[clientIdx] = 0;
		isUseable[chatMateIdx] = 0;
	}
	//sleep(SLEEP_TIME);
	return;
}

void history(Protocol_sendfilecontent *recvPackage, int clientIdx){
	Protocol_sendfilecontent sendPackageBack;
	sendPackageBack.message.header.head.option = PROTOCOL_OPTION_HISTORY;
	sendPackageBack.message.header.head.status = PROTOCOL_STATUS_SUCC;
	sendPackageBack.message.header.head.datalen = sizeof(sendPackageBack) - sizeof(Protocol_header);
	char chatHistoryFileName[MAX_PROTOCOL_NAME_LEN * 3];
	getChatHistoryFileName(chatHistoryFileName, clientUsername[clientIdx], clientChatToName[clientIdx]);
	int fp, readLen, status;
	fp = open(chatHistoryFileName, O_RDONLY);
	readLen = read(fp, sendPackageBack.message.body.fileContent, MAX_PROTOCOL_FILE_CONTENT_LEN);
	sendPackageBack.message.body.fileContentLen = readLen;
	do{
		status = send(clientSockfdTable[clientIdx], &sendPackageBack, sizeof(sendPackageBack), 0);
	}while(status == -1);
	close(fp);
	return;
}

void goodFriend(Protocol_startchat *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> in the goodFriend\n");
	Protocol_startchat sendPackage;
	char fileName[MAX_PROTOCOL_NAME_LEN];
	int status, readLen;
	sprintf(fileName, "./config/%s_gf.txt", clientUsername[clientIdx]);
	sendPackage.message.header.head.option = PROTOCOL_OPTION_GOODFRIEND;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	if ( recvPackage->message.header.head.status == PROTOCOL_STATUS_REQ ){
		int fp = open(fileName, O_RDONLY);
		if (fp <= 0){
			sendPackage.message.body.unReadMsgLen = 0;
		}
		else{
			readLen = read(fp, sendPackage.message.body.unReadMsg, MAX_PROTOCOL_FILE_CONTENT_LEN);
			sendPackage.message.body.unReadMsgLen = readLen;
			close(fp);
		}
	}
	else if ( recvPackage->message.header.head.status == PROTOCOL_STATUS_SUCC ){
		if ( !isInTheFriendFILE(fileName, recvPackage->message.body.chatMateName) && hasUserSignedUP(recvPackage->message.body.chatMateName, NULL)==0 ){
			addToFriendFile(fileName, recvPackage->message.body.chatMateName);
		}
		else{
			sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
		}
	}
	else if ( recvPackage->message.header.head.status == PROTOCOL_STATUS_FAIL ){
		if ( isInTheFriendFILE(fileName, recvPackage->message.body.chatMateName) ){
			rmFromFriendFile(fileName, recvPackage->message.body.chatMateName);
		}
		else{
			sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
		}
	}
	do{
		status = send(clientSockfdTable[clientIdx], &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	return;
}

void badFriend(Protocol_startchat *recvPackage, int clientIdx){
	fprintf(OUTPUT_LOG, "> in the goodFriend\n");
	Protocol_startchat sendPackage;
	char fileName[MAX_PROTOCOL_NAME_LEN];
	int status, readLen;
	sprintf(fileName, "./config/%s_bf.txt", clientUsername[clientIdx]);
	sendPackage.message.header.head.option = PROTOCOL_OPTION_BADFRIEND;
	sendPackage.message.header.head.status = PROTOCOL_STATUS_SUCC;
	sendPackage.message.header.head.datalen = sizeof(sendPackage) - sizeof(Protocol_header);
	if ( recvPackage->message.header.head.status == PROTOCOL_STATUS_REQ ){
		int fp = open(fileName, O_RDONLY);
		if (fp <= 0){
			sendPackage.message.body.unReadMsgLen = 0;
		}
		else{
			readLen = read(fp, sendPackage.message.body.unReadMsg, MAX_PROTOCOL_FILE_CONTENT_LEN);
			sendPackage.message.body.unReadMsgLen = readLen;
			close(fp);
		}
	}
	else if ( recvPackage->message.header.head.status == PROTOCOL_STATUS_SUCC ){
		if ( !isInTheFriendFILE(fileName, recvPackage->message.body.chatMateName) && hasUserSignedUP(recvPackage->message.body.chatMateName, NULL)==0 ){
			addToFriendFile(fileName, recvPackage->message.body.chatMateName);
		}
		else{
			sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
		}
	}
	else if ( recvPackage->message.header.head.status == PROTOCOL_STATUS_FAIL ){
		if ( isInTheFriendFILE(fileName, recvPackage->message.body.chatMateName) ){
			rmFromFriendFile(fileName, recvPackage->message.body.chatMateName);
		}
		else{
			sendPackage.message.header.head.status = PROTOCOL_STATUS_FAIL;
		}
	}
	do{
		status = send(clientSockfdTable[clientIdx], &sendPackage, sizeof(sendPackage), 0);
	}while(status == -1);
	return;
}

int main(int argc, char const *argv[]){
	if (argc < 2){
		fprintf(OUTPUT_LOG, "[Usage] $./server [port]\n");
		return 0;
	}
	/* ----- Build Server Socket ----- */
	int socketfd = buildServerSocket(argv[1]);
	if (socketfd == -1){
		fprintf(OUTPUT_LOG, "buildServerSocket fail\n");
		return 0;
	}
	/* ----- initialize ----- */
	fd_set readset, working_readset;
	FD_ZERO(&readset);
	FD_SET(socketfd, &readset);
	int status, maxfd = socketfd;
	int recvLen;

	clientSockfdTableSize = 0;
	struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    for (int i = 0; i < MAX_ONLINE_CLIENT; i++){
    	isUseable[i] = 0;
    }

	/* ----- Run Listen Loop ----- */
	while(1){
		memcpy(&working_readset, &readset, sizeof(readset));
		status = select(maxfd+1, &working_readset, NULL, NULL, 0);
		if (status > 0){
			//fprintf(OUTPUT_LOG, "select get\n");
			if(FD_ISSET(socketfd, &working_readset)){
				int clientfd = accept(socketfd, (struct sockaddr*) &client_addr, &addrlen);
				if (clientfd > -1){
					maxfd = addNewClient(clientfd, maxfd);
					FD_SET(clientfd, &readset);
					fprintf(OUTPUT_LOG, "addNewClient\n");
				}
			}
			for (int i = 0; i < clientSockfdTableSize; i ++){
				if ( isUseable[i] && FD_ISSET(clientSockfdTable[i], &working_readset) ){
					Protocol_header header;
					recvLen = recv(clientSockfdTable[i], &header, sizeof(header), MSG_WAITALL);
					if (recvLen == -1){
						fprintf(OUTPUT_LOG, "recv client %d unknown\n", i);
					}
					else if (recvLen == 0){
						quit(NULL, i);
						FD_CLR(clientSockfdTable[i], &readset);
					}
					else if (recvLen == sizeof(header)){
						//fprintf(OUTPUT_LOG, "read something\n");
						/* ----- get my protocol ----- */
						//if( header.head.status != PROTOCOL_STATUS_REQ )
						//	continue;
						switch(header.head.option){
							case PROTOCOL_OPTION_SIGNUP:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_SIGNUP\n");
								Protocol_signup signUpPackage;
								if ( completeMessageWithHeader(clientSockfdTable[i], &header, &signUpPackage) ){
									if (signUp(&signUpPackage, i))
										clientsStatus[i] = CLIENT_STATUS_WAIT;
								}
								break;
							case PROTOCOL_OPTION_LOGIN:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_LOGIN\n");
								Protocol_login logInPackage;
								if ( completeMessageWithHeader(clientSockfdTable[i], &header, &logInPackage) ){
									if (logIn(&logInPackage, i))
										clientsStatus[i] = CLIENT_STATUS_WAIT;
								}
								break;
							case PROTOCOL_OPTION_STARTCHAT:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_STARTCHAT\n");
								Protocol_startchat startChatPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &startChatPackage)){
									startChat(&startChatPackage, i);
								}
								break;
							case PROTOCOL_OPTION_SENDMSG:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_SENDMSG\n");
								Protocol_sendmsg sendMsgPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &sendMsgPackage)){
									sendMsg(&sendMsgPackage, i);
								}
								break;
							case PROTOCOL_OPTION_STOPCHAT:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_STOPCHAT\n");
								Protocol_stopchat stopChatPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &stopChatPackage)){
									stopChat(&stopChatPackage, i);
								}
								break;
							case PROTOCOL_OPTION_QUIT:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_QUIT\n");
								Protocol_quit quitPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &quitPackage)){
									quit(&quitPackage, i);
									FD_CLR(clientSockfdTable[i], &readset);
								}
								break;
							case PROTOCOL_OPTION_SENDFILE:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_SENDFILE\n");
								Protocol_sendfile sendFilePackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &sendFilePackage)){
									maxfd = sendFile(&sendFilePackage, i, &readset, maxfd, socketfd);
								}
								break;
							case PROTOCOL_OPTION_SENDFILECONTENT:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_SENDFILECONTENT\n");
								Protocol_sendfilecontent sendFileContentPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &sendFileContentPackage)){
									sendFileContent(&sendFileContentPackage, i, &readset);
								}
								break;
							case PROTOCOL_OPTION_HISTORY:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_HISTORY\n");
								Protocol_sendfilecontent historyPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &historyPackage)){
									history(&historyPackage, i);
								}
								break;
							case PROTOCOL_OPTION_GOODFRIEND:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_GOODFRIEND\n");
								Protocol_startchat goodFriendPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &goodFriendPackage)){
									goodFriend(&goodFriendPackage, i);
								}
								break;
							case PROTOCOL_OPTION_BADFRIEND:
								fprintf(OUTPUT_LOG, "PROTOCOL_OPTION_BADFRIEND\n");
								Protocol_startchat badChatPackage;
								if (completeMessageWithHeader(clientSockfdTable[i], &header, &badChatPackage)){
									badFriend(&badChatPackage, i);
								}
								break;
							default:
								fprintf(stderr, "unknown option %x, from %d\n", header.head.option, i);
								break;
						}
					}
					else{
						fprintf(OUTPUT_LOG, "recv client %d not complete\n", i);
					}
				}
			}
			for(int i = 0; i < clientSockfdTableSize; i++)
				if(isUseable[i]){
					printf("(%d) username: %s, status: ", i, clientUsername[i]);
					if (clientsStatus[i] == CLIENT_STATUS_WAIT)
						printf("CLIENT_STATUS_WAIT\n");
					else if (clientsStatus[i] == CLIENT_STATUS_CHAT_ALONE)
						printf("CLIENT_STATUS_CHAT_ALONE, with %s\n", clientChatToName[i]);
					else if (clientsStatus[i] == CLIENT_STATUS_CHAT_TOGETHER)
						printf("CLIENT_STATUS_CHAT_TOGETHER, with %s\n", clientChatToName[i]);
					else if (clientsStatus[i] == CLIENT_STATUS_CONNECTONLY)
						printf("CLIENT_STATUS_CONNECTONLY\n");
					else if (clientsStatus[i] == CLIENT_STATUS_SENDFILE)
						printf("CLIENT_STATUS_SENDFILE\n");
					else
						printf("\n");
				}
				printf("=======================\n");
		}
	}

	return 0;
}