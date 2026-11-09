#ifndef COMMON_H
#define COMMON_H

/* Necessario per accept4() e SOCK_CLOEXEC: va definito prima di ogni #include */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* Librerie standard */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>

/* Chiamate di sistema Linux e POSIX */
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Costanti di rete */
#define PORT            8080
#define DEFAULT_SERVER  "127.0.0.1"   /* il server ascolta solo in locale (vedi README) */
#define LISTEN_BACKLOG  10

/* Limiti del server */
#define MAX_CLIENTS      16   /* numero massimo di client serviti contemporaneamente */
#define RECV_TIMEOUT_SEC 30   /* timeout di attesa del comando dal client */
#define CMD_TIMEOUT_SEC  10   /* tempo massimo di esecuzione di un comando */

/* Dimensioni dei buffer */
#define CLIENT_BUFFER_SIZE 1024    /* Dimensione massima del comando inviato dal client (compreso '\n') */
#define CHUNK_BUFFER_SIZE  4096    /* Dimensione del blocco per la lettura dell'output da popen() e l'invio */

/*
 * send_all: invia esattamente len byte, gestendo gli invii parziali e EINTR.
 * Ritorna 0 se ok, -1 in caso di errore.
 */
static inline int send_all(int sockfd, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    size_t sent = 0;

    while (sent < len)
    {
        ssize_t n = send(sockfd, p + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

/*
 * parse_port: converte una stringa in numero di porta (1-65535).
 * Ritorna la porta oppure -1 se la stringa non e' valida.
 */
static inline int parse_port(const char *s)
{
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0' || v < 1 || v > 65535)
    {
        return -1;
    }
    return (int)v;
}

/*
 * ErrorManagement: stampa il messaggio con perror(), chiude il descrittore (se valido)
 * ed esce. Da usare solo per errori fatali nel main (all'avvio): chiama exit(),
 * quindi NON va usata nei thread dei client, perche' ucciderebbe tutto il server.
 */
static inline void ErrorManagement(const char *messaggio, int fd)
    __attribute__((noreturn));

static inline void ErrorManagement(const char *messaggio, int fd)
{
    perror(messaggio);
    if (fd >= 0) close(fd);
    exit(EXIT_FAILURE);
}

#endif /* COMMON_H */
