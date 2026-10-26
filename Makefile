CC = gcc  
CFLAGS = -g -Wall 

TARGETS = server client
.PHONY := clean		       
.DEFAULT_GOAL := all	      

all: server client

server: server.c					
	$(CC) $(CFLAGS) server.c -o server.o  

client: client.c					
	$(CC) $(CFLAGS) client.c -o client.o 	 

clean:
	rm -f *.o