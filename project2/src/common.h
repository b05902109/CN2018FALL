#ifndef _COMMON_
#define _COMMON_

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/md5.h>

#define OUTPUT_LOG stderr
#define MAX_ONLINE_CLIENT 20
#define MAX_PROTOCOL_NAME_LEN 40
#define MAX_PROTOCOL_CONTENT_LEN 256
#define MAX_PROTOCOL_FILE_CONTENT_LEN 4096
#define STDIN 0
#define SLEEP_TIME 2

typedef enum{
	CLIENT_STATUS_CONNECTONLY = 0x00,
	CLIENT_STATUS_WAIT = 0x01,
	CLIENT_STATUS_CHAT_TOGETHER = 0x02,
	CLIENT_STATUS_CHAT_ALONE = 0x03,
	CLIENT_STATUS_SENDFILE = 0x04,
} Client_status;

typedef enum {
	PROTOCOL_OPTION_SIGNUP = 0x10,
	PROTOCOL_OPTION_LOGIN = 0x11,
	PROTOCOL_OPTION_STARTCHAT = 0x12,
	PROTOCOL_OPTION_SENDMSG = 0x13,
	PROTOCOL_OPTION_STOPCHAT = 0x14,
	PROTOCOL_OPTION_QUIT = 0x15,
	PROTOCOL_OPTION_SENDFILE = 0x16,
	PROTOCOL_OPTION_SENDFILECONTENT = 0x17,
	PROTOCOL_OPTION_HISTORY = 0x18,
	PROTOCOL_OPTION_GOODFRIEND = 0x19,
	PROTOCOL_OPTION_BADFRIEND = 0x20,
} Protocol_option;

typedef enum {
	PROTOCOL_STATUS_REQ = 0x90,
	PROTOCOL_STATUS_SUCC = 0x91,
	PROTOCOL_STATUS_FAIL = 0x92,
} Protocol_status;

typedef union{
	struct {
		uint8_t option;
		uint8_t status;
		uint32_t datalen;
	} head;
	uint8_t bytes[6];
} Protocol_header;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t username[MAX_PROTOCOL_NAME_LEN];
			uint8_t password[MAX_PROTOCOL_NAME_LEN];
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_NAME_LEN * 2];
} Protocol_signup;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t username[MAX_PROTOCOL_NAME_LEN];
			uint8_t password[MAX_PROTOCOL_NAME_LEN];
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_NAME_LEN * 2];
} Protocol_login;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t chatMateName[MAX_PROTOCOL_NAME_LEN];
			uint8_t unReadMsg[MAX_PROTOCOL_FILE_CONTENT_LEN];
			uint32_t unReadMsgLen;
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_NAME_LEN + MAX_PROTOCOL_FILE_CONTENT_LEN + 4];
} Protocol_startchat;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t content[MAX_PROTOCOL_CONTENT_LEN];
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_CONTENT_LEN];
} Protocol_sendmsg;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t tmp[MAX_PROTOCOL_NAME_LEN];
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_NAME_LEN];
} Protocol_stopchat;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t tmp[MAX_PROTOCOL_NAME_LEN];
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_NAME_LEN];
} Protocol_quit;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t fileName[MAX_PROTOCOL_NAME_LEN];
			uint32_t tmp;
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_NAME_LEN + 4];
} Protocol_sendfile;

typedef union {
	struct {
		Protocol_header header;
		struct {
			uint8_t fileName[MAX_PROTOCOL_NAME_LEN];
			uint8_t fileContent[MAX_PROTOCOL_FILE_CONTENT_LEN];
			uint32_t fragflag;
			uint32_t fileContentLen;
		} body;
	} message;
	uint8_t bytes[ sizeof(Protocol_header) + MAX_PROTOCOL_FILE_CONTENT_LEN + 8 + MAX_PROTOCOL_NAME_LEN];
} Protocol_sendfilecontent;

int completeMessageWithHeader(int fd, Protocol_header* header, void* result);
void md5_make(char *out, MD5_CTX *input_string);

#endif