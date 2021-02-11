# Computer Network Project 1
Nation Taiwan University CSIE

## Platform:
- Windows 10 linux subsystem

## How to compile:
- Use command with Makefile.
    ```
    $ make
    ```
- or you can type

    ```
    $gcc client.c -o client -pthread
    #gcc server.c -o server
    ```


## How to execute:
- As the project asking:
	```
	$ ./server [listen_port]
	$ ./client [-n number] [-t timeout] [host_1]:[port_1] [host_2:port_2]
	```

## What I have done:
- In the **server.c**, I read the argv and continuously call *getaddrinfo()*, *socket()*, *bind()*, and *listen()*.
- Therefore, I use *select()* to get the client send connect requests or send message. If it is connect request, I call *accept()* and add the fd into the select list. As for the message, server sends back 
my protocol, which will be explained in follow, and print the requesting output. Last, if getting the message of shutting down of client socket, server will delete the socket in its select list.
- In the **client.c**, I read argv and use multithread to perform simultaneously ping to multiple server. 
  In every thread, I repeatly build up the socket according to the each IP/Hostname and Port, and call 
  *connect()*. If *connect()* fails, that is a timeout situation, therefore, I print the message. Otherwise, 
  server records the start time, sends message, gets message if not timeout, records the end time, and 
  prints. If timeout, I follow the project request and the server prints the corresponding message.
- The sending protocol is a number of which times I ping. That is, if client gets a message back but 
  the ping time is wrong, then I know its a lately sending from the previous ping. It helps me control 
  the RTT time.
