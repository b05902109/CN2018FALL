.PHONY: all clean

CFLAGS+=
LDFLAGS+=

all:
	gcc server.c -o server
	gcc client.c -o client -pthread
clean:
	$(RM) $(TARGET)
