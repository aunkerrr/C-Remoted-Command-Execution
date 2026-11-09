#include "common.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "** ARGOMENTI ERRATI **\nUso: %s <indirizzo IP server> <porta>\n"
                        "Esempio: %s 127.0.0.1 8080\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int server_port = parse_port(argv[2]);
    if (server_port < 0)
    {
        fprintf(stderr, "** Errore: porta non valida (%s). Valori ammessi: 1-65535 **\n", argv[2]);
        return EXIT_FAILURE;
    }

    /* Il comando si legge da stdin */
    char cmd[CLIENT_BUFFER_SIZE];
    fprintf(stderr, "Comando da eseguire: ");
    if (fgets(cmd, sizeof(cmd), stdin) == NULL)
    {
        fprintf(stderr, "** Errore: nessun comando letto da stdin **\n");
        return EXIT_FAILURE;
    }
    if (strchr(cmd, '\n') == NULL && strlen(cmd) == sizeof(cmd) - 1)
    {
        fprintf(stderr, "** Errore: comando troppo lungo (massimo %d caratteri) **\n", CLIENT_BUFFER_SIZE - 2);
        return EXIT_FAILURE;
    }
    size_t len = strcspn(cmd, "\r\n");
    cmd[len] = '\0';
    if (len == 0)
    {
        fprintf(stderr, "** Errore: comando vuoto **\n");
        return EXIT_FAILURE;
    }

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        ErrorManagement("** Errore nella creazione del socket", -1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "** Errore: indirizzo IP non valido (%s) **\n", server_ip);
        close(client_fd);
        return EXIT_FAILURE;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ErrorManagement("** Connessione fallita", client_fd);
    }

    /* Comando + '\n', poi chiusura della scrittura: il server sa che il comando e' finito */
    if (send_all(client_fd, cmd, len) < 0 || send_all(client_fd, "\n", 1) < 0)
    {
        perror("** Errore durante l'invio");
        close(client_fd);
        return EXIT_FAILURE;
    }
    shutdown(client_fd, SHUT_WR);

    /* Ricezione dell'output completo fino alla chiusura da parte del server */
    char buf[CHUNK_BUFFER_SIZE];
    ssize_t n;
    while ((n = recv(client_fd, buf, sizeof(buf), 0)) != 0)
    {
        if (n < 0)
        {
            if (errno == EINTR) continue;
            perror("** Errore durante la ricezione");
            close(client_fd);
            return EXIT_FAILURE;
        }
        fwrite(buf, 1, (size_t)n, stdout);
    }
    fflush(stdout);

    close(client_fd);
    return EXIT_SUCCESS;
}
