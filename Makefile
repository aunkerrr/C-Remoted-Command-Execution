CC = gcc  
CFLAGS = -g -Wall 

TARGETS = ServSkt CliSkt
.PHONY := clean		       
.DEFAULT_GOAL := all	      

all: ServSkt CliSkt 

ServSkt: ServSkt.c					
	$(CC) $(CFLAGS) ServSkt.c -o ServSkt.o  

CliSkt: CliSkt.c					
	$(CC) $(CFLAGS) CliSkt.c -o CliSkt.o 	 

clean:
	rm -f *.o