CC = gcc  
CFLAGS = -g -Wall -Wextra -Werror -pthread 

TARGETS = Server Client
.PHONY := clean		       
.DEFAULT_GOAL := all	      

all: Server Client 

Server: server.c					
	$(CC) $(CFLAGS) server.c -o server.o  

Client: client.c					
	$(CC) $(CFLAGS) client.c -o client.o 	 

clean:
	rm -f *.o