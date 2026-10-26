void gestioneErrore(const char *messaggio, int fd)
{
    perror(messaggio);
    close(fd);
    exit(EXIT_FAILURE);
}