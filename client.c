#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

size_t n_byte;
int send_all(int sockfd, const char *buf, size_t len)
{
    while (n_byte < len)
    {
        /* code */
    }
    
    
}