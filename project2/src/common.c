#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/md5.h>

int completeMessageWithHeader(int fd, Protocol_header* header, void* result){
	memcpy(result, header->bytes, sizeof(Protocol_header));
	return recv(fd, result + sizeof(Protocol_header), header->head.datalen, MSG_WAITALL) == header->head.datalen;
}

void md5_make(char *out, MD5_CTX *input_string){
    unsigned char digest[17];
    int i;
    memset(digest, 0, sizeof(digest));
    MD5_Final(digest, input_string);
    for (i = 0; i < 16; i++) 
        sprintf(&(out[i*2]), "%02x", (unsigned int)digest[i]);
    out[32] = '\0';
    return;
}

/*, pic2[] = "\n
                     　＿＿＿＿\n
                     ／        ＼\n
                   ／ノ   ＼   u ＼  ！？\n
                 ／（●）（●）    ＼\n
                ︱  （__人__）   u  ︱\n
                 ＼ u ‵ㄧ′　     ／\n
                 ノ                ＼\n
\n";*/
/*
bash
cd Documents/CN/project2
*/