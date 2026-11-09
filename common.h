#ifndef COMMON_H
#define COMMON_H

/* Standart libraries */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>

/* System calls Linux and POSIX */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Network constaints and settings */
#define PORT 8080
#define DEFAULT_SERVER "127.0.0.1"
#define LISTEN_BACKLOG 10

/* Buffer sizes */
#define CLIENT_BUFFER_SIZE 1024    /* Dimensione massima della stringa del comando inviato dal client */
#define CHUNK_BUFFER_SIZE  4096    /* Dimensione del blocco per la lettura dell'output da popen() e l'invio */

/* Prototypes */
int calculate_optimal_backlog(int requested_backlog);
int create_listen_socket(int optimal_backlog);

void ErrorManagementWithClosedSocket(const char *message);
void ErrorManagementWithOpenedSocket(const char *messaggio, int fd);


#endif /* COMMON_H */


