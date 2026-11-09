#include "common.h"

#define PAUSE_US 100000   /* pausa dopo un errore temporaneo di accept (100 ms) */

typedef struct {
    int  client_fd;
    char ip[INET_ADDRSTRLEN];
    char cmd[CLIENT_BUFFER_SIZE];
    int  status;                    /* stato di uscita del comando (registrato nel log) */
} client_ctx_t;

/* Stato globale del server */
static volatile sig_atomic_t running = 1;                 /* messo a 0 da SIGINT/SIGTERM */
static int active_clients = 0;                            /* client in corso */
static pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

/* Whitelist: solo il primo programma di questa lista puo' essere eseguito */
static const char *const ALLOWED_COMMANDS[] = {
    "ls", "pwd", "whoami", "date", "uptime", "hostname",
    "uname", "id", "df", "free", "echo", "ps", NULL
};

/* Prototipi */
static void  handle_signal(int sig);
static void  setup_signal_handlers(void);
static int   create_listen_socket(const char *ip, int port);
static int   validate_command(const char *cmd);
static int   receive_command(int fd, char *cmd, size_t size);
static void  execute_command(client_ctx_t *ctx);
static void *client_thread(void *arg);
static void  release_slot(void);
static int   send_str(int fd, const char *s);

/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    int port = PORT;
    const char *bind_ip = DEFAULT_SERVER;

    if (geteuid() == 0)
    {
        fprintf(stderr, "Errore: non avviare il server come root.\n");
        return EXIT_FAILURE;
    }

    if (argc > 3)
    {
        fprintf(stderr, "Uso: %s [porta] [indirizzo_ip]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc >= 2 && (port = parse_port(argv[1])) < 0)
    {
        fprintf(stderr, "Errore: porta non valida (%s). Valori ammessi: 1-65535.\n", argv[1]);
        return EXIT_FAILURE;
    }
    if (argc == 3)
    {
        bind_ip = argv[2];
    }

    setup_signal_handlers();

    int server_fd = create_listen_socket(bind_ip, port);
    printf("\n --- Il server e' in ascolto su: %s:%d ---\n", bind_ip, port);

    while (running)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int new_socket = accept4(server_fd, (struct sockaddr *)&client_addr,
                                 &client_len, SOCK_CLOEXEC);
        if (new_socket < 0)
        {
            if (errno == EINTR || errno == ECONNABORTED)
            {
                continue;
            }
            if (errno == EBADF || errno == EINVAL || errno == ENOTSOCK)
            {
                perror("Errore fatale durante l'accept");
                break;
            }
            /* EMFILE, ENFILE, ENOBUFS, ENOMEM...: errori temporanei, non uccidere il server */
            perror("Errore temporaneo durante l'accept");
            usleep(PAUSE_US);
            continue;
        }

        /* Limite al numero di thread */
        pthread_mutex_lock(&clients_lock);
        int accepted = (active_clients < MAX_CLIENTS);
        if (accepted) active_clients++;
        pthread_mutex_unlock(&clients_lock);

        if (!accepted)
        {
            send_str(new_socket, "Errore: server occupato, riprovare piu' tardi\n");
            close(new_socket);
            continue;
        }

        client_ctx_t *ctx = calloc(1, sizeof(*ctx));
        if (ctx == NULL)
        {
            perror("Errore calloc");
            close(new_socket);
            release_slot();
            continue;
        }
        ctx->client_fd = new_socket;
        inet_ntop(AF_INET, &client_addr.sin_addr, ctx->ip, sizeof(ctx->ip));

        /* Thread detached; SIGINT/SIGTERM bloccati nel worker (li gestisce solo il main) */
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        sigset_t block, old;
        sigemptyset(&block);
        sigaddset(&block, SIGINT);
        sigaddset(&block, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &block, &old);

        pthread_t tid;
        int rc = pthread_create(&tid, &attr, client_thread, ctx);

        pthread_sigmask(SIG_SETMASK, &old, NULL);
        pthread_attr_destroy(&attr);

        if (rc != 0)
        {
            fprintf(stderr, "Errore pthread_create: %s\n", strerror(rc));
            close(new_socket);
            free(ctx);
            release_slot();
            continue;
        }
    }

    printf("\n- Arresto del server in corso...\n");
    close(server_fd);

    /* Attende (con un limite) la fine dei thread ancora attivi */
    for (int i = 0; i < (RECV_TIMEOUT_SEC + CMD_TIMEOUT_SEC + 2) * 10; i++)
    {
        pthread_mutex_lock(&clients_lock);
        int n = active_clients;
        pthread_mutex_unlock(&clients_lock);
        if (n == 0) break;
        usleep(PAUSE_US);
    }
    return 0;
}

/* ------------------------------------------------------------------ */

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

static void setup_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    /* SIGPIPE ignorato: la send su un client chiuso restituisce EPIPE invece di uccidere il server */
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        ErrorManagement("Errore sigaction SIGPIPE", -1);
    }

    /* Niente SA_RESTART: cosi' accept() viene interrotta (EINTR) e il ciclo controlla 'running' */
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0)
    {
        ErrorManagement("Errore sigaction SIGINT/SIGTERM", -1);
    }
}

static int create_listen_socket(const char *ip, int port)
{
    int opt = 1;
    struct sockaddr_in address;

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        ErrorManagement("Errore durante la creazione del socket", -1);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        ErrorManagement("Errore nella configurazione del socket", fd);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0)
    {
        fprintf(stderr, "Errore: indirizzo IP non valido (%s)\n", ip);
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        ErrorManagement("Errore durante il bind", fd);
    }
    if (listen(fd, LISTEN_BACKLOG) < 0)
    {
        ErrorManagement("Errore durante il listen", fd);
    }
    return fd;
}

/*
 * validate_command: ritorna 1 se il comando e' ammesso, 0 altrimenti.
 *  - rifiuta i metacaratteri della shell  ; | & $ \ < > ( ) `  e i caratteri di controllo
 *  - il primo programma deve essere nella whitelist
 */
static int validate_command(const char *cmd)
{
    static const char FORBIDDEN[] = ";|&$\\<>()`";

    for (const char *p = cmd; *p != '\0'; p++)
    {
        if (strchr(FORBIDDEN, *p) != NULL) return 0;
        if (iscntrl((unsigned char)*p) && *p != '\t') return 0;
    }

    while (*cmd == ' ' || *cmd == '\t') cmd++;
    size_t wlen = strcspn(cmd, " \t");
    if (wlen == 0) return 0;

    for (int i = 0; ALLOWED_COMMANDS[i] != NULL; i++)
    {
        if (strlen(ALLOWED_COMMANDS[i]) == wlen && strncmp(cmd, ALLOWED_COMMANDS[i], wlen) == 0)
        {
            return 1;
        }
    }
    return 0;
}

/*
 * receive_command: legge a blocchi fino a '\n' (o EOF se il client ha chiuso la scrittura).
 * Ritorna la lunghezza del comando (0 = vuoto), -1 errore, -2 troppo lungo, -3 timeout.
 */
static int receive_command(int fd, char *cmd, size_t size)
{
    size_t len = 0;
    int found = 0, eof = 0;

    while (!found && !eof && len < size - 1)
    {
        ssize_t n = recv(fd, cmd + len, size - 1 - len, 0);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -3;
            perror("Errore nella ricezione");
            return -1;
        }
        if (n == 0)
        {
            eof = 1;
            break;
        }
        char *nl = memchr(cmd + len, '\n', (size_t)n);
        if (nl != NULL)
        {
            len = (size_t)(nl - cmd);
            found = 1;
        }
        else
        {
            len += (size_t)n;
        }
    }

    if (!found && !eof)
    {
        return -2;   /* buffer pieno senza '\n' */
    }

    cmd[len] = '\0';
    if (len > 0 && cmd[len - 1] == '\r')
    {
        cmd[--len] = '\0';
    }
    return (int)strlen(cmd);
}

static void execute_command(client_ctx_t *ctx)
{
    char full_cmd[CLIENT_BUFFER_SIZE + 64];

    /* timeout limita la durata; la subshell ( ) applica 2>&1 a tutto; stderr incluso nell'output */
    int w = snprintf(full_cmd, sizeof(full_cmd), "( timeout %d %s ) 2>&1", CMD_TIMEOUT_SEC, ctx->cmd);
    if (w < 0 || (size_t)w >= sizeof(full_cmd))
    {
        send_str(ctx->client_fd, "Errore: comando troppo lungo\n");
        ctx->status = -1;
        return;
    }

    FILE *fp = popen(full_cmd, "r");
    if (fp == NULL)
    {
        send_str(ctx->client_fd, "Errore: impossibile eseguire il comando\n");
        ctx->status = -1;
        return;
    }

    char buf[CHUNK_BUFFER_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
    {
        if (send_all(ctx->client_fd, buf, n) < 0)
        {
            perror("Errore durante l'invio");
            break;
        }
    }
    ctx->status = pclose(fp);
}

static void *client_thread(void *arg)
{
    client_ctx_t *ctx = (client_ctx_t *)arg;

    /* Timeout: un client silenzioso o lento non blocca il thread per sempre */
    struct timeval tv = { .tv_sec = RECV_TIMEOUT_SEC, .tv_usec = 0 };
    setsockopt(ctx->client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(ctx->client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    int len = receive_command(ctx->client_fd, ctx->cmd, sizeof(ctx->cmd));

    if (len == -2)
    {
        printf("- [%s] comando troppo lungo: RIFIUTATO\n", ctx->ip);
        send_str(ctx->client_fd, "Errore: comando troppo lungo\n");
    }
    else if (len == -3)
    {
        printf("- [%s] timeout in ricezione\n", ctx->ip);
        send_str(ctx->client_fd, "Errore: timeout in attesa del comando\n");
    }
    else if (len == 0)
    {
        printf("- [%s] comando vuoto\n", ctx->ip);
        send_str(ctx->client_fd, "Errore: comando vuoto\n");
    }
    else if (len > 0)
    {
        printf("- [%s] comando ricevuto: %s\n", ctx->ip, ctx->cmd);
        if (!validate_command(ctx->cmd))
        {
            printf("- [%s] comando RIFIUTATO (non consentito)\n", ctx->ip);
            send_str(ctx->client_fd, "Errore: comando non consentito\n");
        }
        else
        {
            execute_command(ctx);
            printf("- [%s] comando eseguito, stato di uscita: %d\n", ctx->ip, ctx->status);
        }
    }

    close(ctx->client_fd);
    free(ctx);
    release_slot();
    return NULL;
}

static void release_slot(void)
{
    pthread_mutex_lock(&clients_lock);
    active_clients--;
    pthread_mutex_unlock(&clients_lock);
}

static int send_str(int fd, const char *s)
{
    return send_all(fd, s, strlen(s));
}
