#ifndef COMMON.H
#define COMMON.H

/* Standart libraries */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

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

/* AGGIORNAMENTI */

#endif /* COMMON.H */


