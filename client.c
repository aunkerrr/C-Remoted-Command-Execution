#include <common.h>

/**
 * @brief this main() creates a client socket that sends a message to the server
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char const* argv[])
{
    /**
     * @brief initialization and argument error managment
     */
    int status, valread, client_fd;
    
    if (argc != 4)
    {
        printf("** WRONG ARGUMENTS **\nExample: \n./file.o <Indirizzo IP Server> <Porta> <Message>\n");
        return EXIT_FAILURE;
    }
    
    char* server_ip = argv[1];
    char* server_port = argv[2];
    char* text = argv[3];
    
    char buffer[1024] = {0};
    
    /**
     * @brief socket(): creates a file descriptor and lets us decide how it should be
     * 
     * @param AF_INET : assingns a domain to our socket AF_(address family) identifies the format used in the addresses 
     * @param SOCK_STREAM : specifies the comms. style of the socket
     * @param 0 : protocol value
     */
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("\n** Client-Socket status report: socket creation failed **\n Motivation: ");
        close(client_fd);
        return EXIT_FAILURE;
    }
    printf("\n- Client-Socket status report: client-socket CREATED\n");
    
    /**
     * @brief sets the address and the port
     */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    /**
     * @brief inet_pton : converts the network address structure src in the af address family into a character string. 
     */
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) 
    {
        fprintf(stderr, "** Error: IP address not valid (%s) **\n", server_ip);
        close(client_fd);
        return EXIT_FAILURE;
    }

    /**
     * @brief connect : sets up a connection with a server 
     * 
     * @param client_fd : socket file descriptor
     * @param (struct sockaddr*)&server_addr : struct with the server address 
     * @param sizeof(server_addr): the lenght of the stuct which contains the server addr.
     */
    if ((status = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0)
    {
        perror("\n** Client-Socket status report: connection FAILED **\n Motivation: ");
        close(client_fd);
        return EXIT_FAILURE;
    }
    printf("\n- Client-Socket status report: connection SUCCESSFULL\n");

    
    /**
     * @brief send : sends a message to the server socket
     *  
     * @param client_fd : socket file descriptor
     * @param text : string containing the message
     * @param strlen(text) : lenght of the string
     * @param 0 : flag that specifies how the data should be sent 
     */
    if (send(client_fd, text, strlen(text), 0) < 0)
    {
        perror("\n** Client message status: FAILED to SEND **\n Motivation: ");
        close(client_fd);
        return EXIT_FAILURE;
    }
    printf("\n- Client message status: SENT\n Content: %s\n", text);
    
    /**
     * @brief recv : receives a message from the communication socket (new_socket)
     * 
     * @param client_fd : socket file descriptor
     * @param buffer : Buffer containing the message
     * @param 1024-1 : lenght of the buffer
     * @param 0 : flag that specifies how the data should be sent 
     */
    if (valread = recv(client_fd, buffer, 1024 - 1, 0) < 0)
    {
        perror("\n** Client-Socket status report: FAILED to RECEIVE **\n Motivation: ");
        close(client_fd);
        return EXIT_FAILURE;
    }
    printf("\n- Server reply status: RECEIVED\n Content: %s\n", buffer);

    close(client_fd); //closes the file descriptor
    return 0;
}