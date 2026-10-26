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

int execute_command(int client_fd, const char *cmd)
{
    char full_cmd[1200];
    // 2>&1 per catturare anche stderr, non solo stdout
    snprintf(full_cmd, sizeof(full_cmd), "%s 2>&1", cmd);

    FILE *fp = popen(full_cmd, "r");
    if (fp == NULL) {
        const char *err = "Errore: impossibile eseguire il comando\n";
        send(client_fd, err, strlen(err), 0);
        return -1;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (send(client_fd, buf, n, 0) == -1) {
            perror("send");
            break;
        }
    }

    int status = pclose(fp);
    return status; // -1 se pclose fallisce, altrimenti stato del processo
}