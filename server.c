#include <common.h>

void gestioneErrore(const char *messaggio)
{
    perror(messaggio);
}

/**
 * @brief this main creates a server socket that listens for a client
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char const* argv[])
{
    /**
     * @brief initalization of the FDs, Addresses and buffers
     */
    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};
    char CAPSbuffer[1024] = {0};
    

    /**
     * @brief socket(): creates a file descriptor and lets us decide how it should be
     * 
     * @param AF_INET : assingns a domain to our socket AF_(address family) identifies the format used in the addresses 
     * @param SOCK_STREAM : specifies the comm. style of the socket 
     * @param 0 : protocol value
     */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("\n** Server-Socket status report: FAILED to create **\n Motivation: ");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("\n- Server-Socket status report: CREATED\n");
    

    /**
     * @brief setsockopt : sets the behavior of the socket
     * 
     * @param server_fd : socket file descriptor
     * @param SOL_SOCKET : specifies the level of the protocol in which the option is contained
     * @param SO_REUSEADDR : Name of the option 
     * @param SO_REUSEPORT : Name of the option
     * @param &opt : value of the option 
     * @param sizeof(opt) : lenght of the buffer
     */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) 
    {
        perror("\n** Server-Socket status report: FAILED to set operations **\n Motivation: ");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    printf("\n- Server-Socket status report: operations set SUCCESSFULLY\n");


    /**
     * @brief bind : assigns a local address to a socket 
     * 
     * @param server_fd : socket file descriptor
     * @param (struct sockaddr*)&address : local address of the server socket
     * @param sizeof(address) : the lenght of the stuct which contains the loc. addr.
     */
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        perror("\n** Server-Socket status report: FAILED to bind **\n Motivation: ");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("\n- Server-Socket status report: binding occured SUCCESSFULLY\n");


    /**
     * @brief listen : uses the socket in a passive mode, it listens for connections
     * 
     * @param server_fd : socket file descriptor
     * @param 3 : it is the lenght of the queque of complete connections 
     */
    if (listen(server_fd, 3) < 0)
    {
        perror("\n** Server-Socket status report: FAILED to listen **\n Motivation: ");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("\n- Server-Socket status report: LISTENING\n");


    /**
     * @brief getsockname: returns the address structure and the port utilzed 
     * 
     * @param server_fd : file descriptor 
     * @param (struct sockaddr *)&address : local address of the server socket
     * @param &addrlen : the lenght of the stuct which contains the loc. addr.
     */
    if (getsockname(server_fd, (struct sockaddr *)&address, &addrlen) == -1) 
    {
        perror("** getsockname failed **\n Motivation: ");
        return -1;
    }


    /**
     * @brief  inet_ntop : converts the network address structure src in the af address family into a character string. 
     */
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, ip_str, sizeof(ip_str));
    
    int port = ntohs(address.sin_port);
    
    printf("\n --- the server is listening on: %s:%d ---\n", ip_str, port);


    /**
     * @brief accept: manages the connection, when the TWH is completed it returns a new FD on which we can communicate 
     * 
     * @param server_fd : file descriptor
     * @param (struct sockaddr*)&address :  local address of the server socket
     * @param &addrlen : the lenght of the stuct which contains the loc. addr.
     */
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0)
    {
        perror("\n** Server-Socket status report: FAILED to accept the connection **\n Motivation: ");
        close(new_socket);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("\n- Server-Socket status report: connection ACCEPTED\n");


    /**
     * @brief recv : receives a message from the communication socket (new_socket)
     * 
     * @param new_socket : communication socket between Cli and Serv
     * @param buffer : Buffer containing the message
     * @param 1024 - 1 : lenght of the buffer
     * @param 0 : flag that specifies how the data should be sent 
     */
    if ((valread = recv(new_socket, buffer, 1024 - 1, 0)) < 0)
    {
        perror("\n** Client message status: FAILED to RECEIVE **\n Motivation: ");
        close(new_socket);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("\n- Client message status: RECEIVED\n Content: %s\n", buffer);
     

    /**
     * @brief lowercase letters => UPPERCASE letters
     * 
     */
    for (int i = 0; i < strlen(buffer);i++)
    {
        CAPSbuffer[i] = toupper(buffer[i]);
    }


    /**
     * @brief send : sends a message trough the communication socket (new_socket)
     * 
     * @param new_socket : communication socket between Cli and Serv
     * @param CAPSbuffer : Buffer containing the message
     * @param strlen(CAPSbuffer) : lenght of the buffer
     * @param 0 : flag that specifies how the data should be sent 
     */
    if(send(new_socket, CAPSbuffer, strlen(CAPSbuffer), 0) < 0)
    {
        perror("\n** Client message status: FAILED to SEND **\n Motivation: ");
        close(new_socket);
        close(server_fd);
        exit(EXIT_FAILURE);
    }   
    printf("\n- Server reply status: SENT\n Content: %s\n", CAPSbuffer);

    close(new_socket);
    close(server_fd); //closes the file descriptor
    return 0;
}