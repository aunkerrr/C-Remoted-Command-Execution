# C-Remoted-Command-Execution
A lightweight C client-server application for executing remote shell commands over TCP sockets on Linux.

## Overview

The project consists of two separate programs communicating over Internet domain stream sockets (AF_INET, SOCK_STREAM):
- **Server:** Listens on a dedicated TCP port, receives shell command strings from connected clients, executes them locally via `popen()`, streams `stdout` back in chunks, and cleans up connections.
- **Client:** Connects to the server IP and port, accepts shell commands from `stdin`, sends them over the socket, and prints the returned output to the console.

## Features & Implementation Details

- **POSIX Sockets:** Robust socket lifecycle handling (`socket`, `bind`, `listen`, `accept`, `connect`).
- **I/O Redirection & Pipes:** Execution via `popen()` and chunked response streaming (`fgets`, `send`).
- **Build Automation:** Includes a simple `Makefile` to build both `server` and `client` binaries.

## Compilazione e uso

```
make                      # produce gli eseguibili ./server e ./client
./server [porta] [ip]     # default: 8080 su 127.0.0.1 (non avviare come root)
./client <ip> <porta>     # il comando si scrive su stdin
echo "ls -l" | ./client 127.0.0.1 8080
```

Il server accetta solo comandi la cui prima parola e' nella whitelist (`ls`, `pwd`, `whoami`, `date`, ...)
e rifiuta i metacaratteri della shell `; | & $ \ < > ( ) ``. Massimo 16 client contemporanei,
timeout di 30 s in ricezione e 10 s per l'esecuzione del comando.

## Security Considerations

*Educational Warning:* This system demonstrates unauthenticated remote command execution over plaintext TCP. It serves as a practical case study for several critical vulnerabilities:
- **Command Injection:** Unsanitized shell strings passed to `popen()` allow arbitrary command execution via shell metacharacters (`;`, `|`, `&&`).
- **No Authentication:** Open access without client identity verification.
- **Cleartext Transmission:** Traffic is susceptible to network sniffing.
- **Privilege Management:** Risks associated with running server daemons under elevated privileges (`root`).
