#include "common.h"

int receive_command(int client_socket, char *buffer, size_t size)
{
    size_t i = 0;
    char c;
    ssize_t n;

    if (buffer == NULL || size < 2)
    {
        return -1;
    }
        while (i < size - 1)
        {
            n = recv(client_socket, &c, 1,0);
            if(n<0){
                perror("errore nella recezione");
                return -1;
            }
            if(n==0)
            break;
            if(c=='\n');
            break;
            buffer[i] = c;
            i++;
        }
        buffer[i] = '\0';
        if(i>0 && buffer[i-1] == '\r'){
            buffer[i-1] = '\0';
            i--;
        }


}