.PHONY: all clean

CFLAGS+=
LDFLAGS+=

all:
	gcc -o server ./src/server.c ./src/common.c -lcrypto
	gcc -o client ./src/client.c ./src/common.c -lcrypto -pthread
clean:
	$(RM) $(TARGET)
