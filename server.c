#include "common.h"



/* Contesto di una connessione: passato ai thread receive_command ed execute_command */
typedef struct {
    int  client_fd;                  /* socket del client */
    char cmd[CLIENT_BUFFER_SIZE];    /* comando ricevuto */
    int  status;                     /* esito di execute_command (stato di pclose) */
} client_ctx_t;

void setup_signal_handlers(void);


int validate_command(const char *cmd);

void* receive_command(void *arg);

void* execute_command(void *arg);

/**
 * @brief this main creates a server socket that listens for a client
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{
    int user_backlog = (argc >= 3) ? atoi(argv[2]) : 0;
    int optimal_backlog = calculate_optimal_backlog(user_backlog);
    
    printf("[INIT] Logical cores in system: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("[INIT] Final TCP backlog size: %d\n", optimal_backlog);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    
    //
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
       ErrorManagementWithOpenedSocket("Errore durante la creazione del socket", server_fd);
    }
    printf("\n- Server-Socket status report: CREATED\n");
   
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) 
    {
        ErrorManagementWithOpenedSocket("Errore nella configurazione del socket", server_fd);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    printf("\n- Server-Socket status report: operations set SUCCESSFULLY\n");

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        ErrorManagementWithOpenedSocket("Errore durante il bind", server_fd);
 
    }
    printf("\n- Server-Socket status report: binding occured SUCCESSFULLY\n");

    if (listen(server_fd, 3) < 0)
    {
        ErrorManagementWithOpenedSocket("Errore durante il listen", server_fd);
    }
    printf("\n- Server-Socket status report: LISTENING\n");

    if (getsockname(server_fd, (struct sockaddr *)&address, &addrlen) == -1) 
    {
        ErrorManagementWithOpenedSocket("Errore durante il getsocketname", server_fd);
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, ip_str, sizeof(ip_str));
    
    int port = ntohs(address.sin_port);
    
    printf("\n --- the server is listening on: %s:%d ---\n", ip_str, port);

    /* Server loop: ogni client accettato viene gestito da un thread dedicato */
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
            ErrorManagementWithOpenedSocket("Errore durante il accept", server_fd);
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

        /* Thread detached: libera da solo le proprie risorse quando termina */
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

    close(server_fd); //closes the file descriptor
    return 0;
}

void setup_signal_handlers(void){
    //TO DO
}

int validate_command(const char *cmd)
{
    // TO DO
}

/**
 * @brief Thread: legge il comando dal client (fino a '\n'), poi avvia il thread
 *        execute_command e ne attende la fine. Chiude la connessione e libera il contesto.
 *
 * @param arg client_ctx_t* allocato dal main (il thread ne diventa proprietario)
 */
void* receive_command(void *arg)
{
    client_ctx_t *ctx = (client_ctx_t *)arg;
    size_t size = sizeof(ctx->cmd);
    size_t i = 0;
    char c;
    ssize_t n;
    int troppo_lungo = 0;

    while (1)
    {
        n = recv(ctx->client_fd, &c, 1, 0);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("errore nella recezione");
            goto cleanup;
        }
        if (n == 0)
        {
            break;
        }

        if (c == '\n')
        {
            break;
        }

       if(i < size -1){
        ctx->cmd[i] = c;
        i++;
       }
       else {
        troppo_lungo= 1;
       }
    }
    ctx->cmd[i] = '\0';
    if (i > 0 && ctx->cmd[i-1] == '\r')
    {
        ctx->cmd[i-1] = '\0';
        i--;
    }
    if(troppo_lungo){
    const char *msg = "errore: comando troppo lungo\n";
    send(ctx->client_fd, msg, strlen(msg), MSG_NOSIGNAL);
    goto cleanup;
    }
    printf("\n- Client message status: RECEIVED\n Content: %s\n", ctx->cmd);

    if (i > 0)
    {
        pthread_t exec_tid;
        int rc = pthread_create(&exec_tid, NULL, execute_command, ctx);
        if (rc != 0)
        {
            fprintf(stderr, "Errore pthread_create: %s\n", strerror(rc));
            goto cleanup;
        }
        pthread_join(exec_tid, NULL);

        /**
         * @brief lowercase letters => UPPERCASE letters
         */
        char CAPSbuffer[CLIENT_BUFFER_SIZE] = {0};
        for (size_t k = 0; k < strlen(ctx->cmd); k++)
        {
            CAPSbuffer[k] = toupper((unsigned char)ctx->cmd[k]);
        }

        if (send(ctx->client_fd, CAPSbuffer, strlen(CAPSbuffer), MSG_NOSIGNAL) < 0)
        {
            perror("Errore durante il send");
        }
        else
        {
            printf("\n- Server reply status: SENT\n Content: %s\n", CAPSbuffer);
        }
    }

cleanup:
    close(ctx->client_fd);
    free(ctx);
    return NULL;
}

/**
 * @brief Thread: esegue ctx->cmd con popen() e invia l'output (stdout+stderr) al client.
 *        Lo stato di pclose() viene salvato in ctx->status.
 *
 * @param arg client_ctx_t* (di proprieta' del thread receive_command, che fa la join)
 */
void* execute_command(void *arg)
{
    client_ctx_t *ctx = (client_ctx_t *)arg;
    char full_cmd[CLIENT_BUFFER_SIZE + 16];
    // 2>&1 per catturare anche stderr, non solo stdout
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
        /* MSG_NOSIGNAL: se il client chiude, niente SIGPIPE che ucciderebbe tutto il server */
        if (send(ctx->client_fd, buf, n, MSG_NOSIGNAL) == -1) {
            perror("send");
            break;
        }
    }

    ctx->status = pclose(fp); // -1 se pclose fallisce, altrimenti stato del processo
    return NULL;
}