#ifndef ERRORMANAGEMENTSERVICE_H
#define ERRORMANAGEMENTSERVICE_H
#include "common.h"

void ErrorManagementWithClosedSocket(const char *message);

void ErrorManagementWithOpenedSocket(const char *messaggio, int fd);


void ErrorManagementWithClosedSocket(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void ErrorManagementWithOpenedSocket(const char *messaggio, int fd)
{
    perror(messaggio);
    close(fd);
    exit(EXIT_FAILURE);
}
#endif /* ERRORMANAGEMENTSERVICE_H */


