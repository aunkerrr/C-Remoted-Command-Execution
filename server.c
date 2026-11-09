#include "common.h"
#include <pthread.h>

typedef struct {
    int  client_fd;                  
    char cmd[CLIENT_BUFFER_SIZE];    
    int  status;                     
} client_ctx_t;

void ErrorManagement(const char *messaggio, int fd);
void* receive_command(void *arg);
void* execute_command(void *arg);

int main(void)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
       ErrorManagement("Errore durante la creazione del socket", server_fd);
    }
    printf("\n- Server-Socket status report: CREATED\n");
   
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) 
    {
        ErrorManagement("Errore nella configurazione del socket", server_fd);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    printf("- Server-Socket status report: operations set SUCCESSFULLY\n");

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        ErrorManagement("Errore durante il bind", server_fd);
    }
    printf("- Server-Socket status report: binding occurred SUCCESSFULLY\n");

    if (listen(server_fd, LISTEN_BACKLOG) < 0)
    {
        ErrorManagement("Errore durante il listen", server_fd);
    }
    printf("- Server-Socket status report: LISTENING\n");

    if (getsockname(server_fd, (struct sockaddr *)&address, &addrlen) == -1) 
    {
        ErrorManagement("Errore durante il getsocketname", server_fd);
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, ip_str, sizeof(ip_str));
    int port = ntohs(address.sin_port);
    
    printf("\n --- The server is listening on: %s:%d ---\n", ip_str, port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        if ((new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len)) < 0)
        {
            if (errno == EINTR || errno == ECONNABORTED)
            {
                continue;
            }
            ErrorManagement("Errore durante l'accept", server_fd);
        }
        printf("\n- Server-Socket status report: connection ACCEPTED\n");

        client_ctx_t *ctx = calloc(1, sizeof(*ctx));
        if (ctx == NULL)
        {
            perror("Errore calloc");
            close(new_socket);
            continue;
        }
        ctx->client_fd = new_socket;

        pthread_t tid;
        int rc = pthread_create(&tid, NULL, receive_command, ctx);
        if (rc != 0)
        {
            fprintf(stderr, "Errore pthread_create: %s\n", strerror(rc));
            close(new_socket);
            free(ctx);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}

void ErrorManagement(const char *messaggio, int fd)
{
    perror(messaggio);
    if (fd >= 0) close(fd);
    exit(EXIT_FAILURE);
}

void* receive_command(void *arg)
{
    client_ctx_t *ctx = (client_ctx_t *)arg;
    size_t size = sizeof(ctx->cmd);
    size_t i = 0;
    char c;
    ssize_t n;

    while (i < size - 1)
    {
        n = recv(ctx->client_fd, &c, 1, 0);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            perror("Errore nella ricezione");
            goto cleanup;
        }
        if (n == 0 || c == '\n') break;

        ctx->cmd[i++] = c;
    }
    ctx->cmd[i] = '\0';

    if (i > 0 && ctx->cmd[i-1] == '\r')
    {
        ctx->cmd[i-1] = '\0';
    }
    printf("- Client message status: RECEIVED\n Content: %s\n", ctx->cmd);

    if (strlen(ctx->cmd) > 0)
    {
        pthread_t exec_tid;
        int rc = pthread_create(&exec_tid, NULL, execute_command, ctx);
        if (rc != 0)
        {
            fprintf(stderr, "Errore pthread_create: %s\n", strerror(rc));
            goto cleanup;
        }
        pthread_join(exec_tid, NULL);
    }

cleanup:
    close(ctx->client_fd);
    free(ctx);
    return NULL;
}

void* execute_command(void *arg)
{
    client_ctx_t *ctx = (client_ctx_t *)arg;
    char full_cmd[CLIENT_BUFFER_SIZE + 16];
    snprintf(full_cmd, sizeof(full_cmd), "%s 2>&1", ctx->cmd);

    FILE *fp = popen(full_cmd, "r");
    if (fp == NULL) {
        const char *err = "Errore: impossibile eseguire il comando\n";
        send(ctx->client_fd, err, strlen(err), MSG_NOSIGNAL);
        ctx->status = -1;
        return NULL;
    }

    char buf[CHUNK_BUFFER_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (send(ctx->client_fd, buf, n, MSG_NOSIGNAL) == -1) {
            perror("send");
            break;
        }
    }

    ctx->status = pclose(fp);
    return NULL;
}