#include "common.h"

size_t n_byte;
int send_all(int sockfd, const char *buf, size_t len)
{
    while (n_byte < len)
    {
        /* code */
    }
}
int main(int argc, char const* argv[])
{
    int status, valread, client_fd;
    
    if (argc != 4)
    {
        printf("** WRONG ARGUMENTS **\nExample: \n./file.o <Indirizzo IP Server> <Porta> <Message>\n");
        return EXIT_FAILURE;
    }

    char *server_ip = argv[1];
    char* server_port = argv[2];
    char* text = argv[3];
    
    char buffer[1024] = {0};
    
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        ErrorManagementWithClosedSocket("** Client-Socket: socket creation failed **");
    }
    printf("\n- Client-Socket status report: client-socket CREATED\n");
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));


    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) 
    {
        ErrorManagementWithOpenedSocket("** Error: IP address not valid **", client_fd);
    }

    if ((status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0)
    {
        ErrorManagementWithOpenedSocket("** Client-Socket: connection FAILED **", client_fd);
    }
    printf("\n- Client-Socket status report: connection SUCCESSFULL\n");

    if (send(client_fd, text, strlen(text), 0) < 0)
    {
        ErrorManagementWithOpenedSocket("** Client message status: FAILED to SEND **", client_fd);
    }
    printf("\n- Client message status: SENT\n Content: %s\n", text);
    
    if ((valread = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) < 0)
    {
        ErrorManagementWithOpenedSocket("** Client-Socket: FAILED to RECEIVE **", client_fd);
    }
    printf("\n- Server reply status: RECEIVED\n Content: %s\n", buffer);

    close(client_fd); //closes the file descriptor
    return 0;

}