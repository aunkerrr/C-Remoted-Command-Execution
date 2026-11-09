#include "common.h"

int send_all(int sockfd, const char *buf) {
    char msg[CLIENT_BUFFER_SIZE];
    int msg_len = snprintf(msg, sizeof(msg), "%s\n", buf);
    if (msg_len < 0 || (size_t)msg_len >= sizeof(msg)) {
        fprintf(stderr, "Errore: comando troppo lungo.\n");
        return -1;
    }

    size_t sent = 0;
    size_t total_to_send = (size_t)msg_len;
    while (sent < total_to_send) {
        ssize_t n = send(sockfd, msg + sent, total_to_send - sent, 0);
        if (n < 0) {
            perror("Errore durante l'invio (send)");
            return -1;
        }
        sent += n;
    }
    printf("- Client message status: SENT\n Content: %s", msg);

    // Ricezione dello stream di risposta inviato dal server
    char recv_buf[CHUNK_BUFFER_SIZE];
    ssize_t n;
    printf("- Server reply status: RECEIVING OUTPUT...\n");
    while ((n = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0)) > 0) {
        recv_buf[n] = '\0';
        fwrite(recv_buf, 1, n, stdout);
    }

    if (n < 0) {
        perror("Errore durante la ricezione (recv)");
        return -1;
    }

    return 0;
}

int main(int argc, char const* argv[])
{
    int status, client_fd;
    
    if (argc != 4)
    {
        printf("** WRONG ARGUMENTS **\nExample: \n./client.o <Indirizzo IP Server> <Porta> <Message>\n");
        return EXIT_FAILURE;
    }
    
    const char* server_ip = argv[1];
    int server_port = atoi(argv[2]);
    const char* text = argv[3];
    
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("\n** Client-Socket status report: socket creation failed **");
        return EXIT_FAILURE;
    }
    printf("\n- Client-Socket status report: client-socket CREATED\n");
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) 
    {
        fprintf(stderr, "** Error: IP address not valid (%s) **\n", server_ip);
        close(client_fd);
        return EXIT_FAILURE;
    }

    if ((status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0)
    {
        perror("\n** Client-Socket status report: connection FAILED **");
        close(client_fd);
        return EXIT_FAILURE;
    }
    printf("- Client-Socket status report: connection SUCCESSFUL\n");

    if (send_all(client_fd, text) < 0) {
        close(client_fd);
        return EXIT_FAILURE;
    }

    close(client_fd);
    return EXIT_SUCCESS;
}