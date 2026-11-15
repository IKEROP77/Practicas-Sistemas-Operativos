/*
 * Servidor.c - Practica 2 (Comunicaciones en red)
 * Servidor concurrente: escucha en el puerto 9999 y, por cada cliente,
 * crea un hijo que le envia el contenido del archivo "Google.html".
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PUERTO   9999
#define COLA     5
#define TAM_BUF  4096
#define ARCHIVO  "Google.html"

/* Recoge a los hijos terminados para que no queden procesos zombi */
void manejador_hijos(int s) {
    (void)s;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

/* Tarea del hijo: enviar el archivo completo por el socket del cliente */
void atender_cliente(int sock_cliente) {
    char buffer[TAM_BUF];
    ssize_t leidos, escritos, total;
    int fd = open(ARCHIVO, O_RDONLY);

    if (fd < 0) {
        perror("Hijo: open " ARCHIVO);
        return;
    }
    while ((leidos = read(fd, buffer, TAM_BUF)) > 0) {
        total = 0;
        while (total < leidos) {          /* write puede escribir menos de lo pedido */
            escritos = write(sock_cliente, buffer + total, leidos - total);
            if (escritos < 0) {
                perror("Hijo: write");
                close(fd);
                return;
            }
            total += escritos;
        }
    }
    if (leidos < 0) perror("Hijo: read");
    close(fd);
}

int main(void) {
    int conector, sock_cliente, opt = 1;
    struct sockaddr_in dir_servidor, dir_cliente;
    socklen_t long_cliente;
    struct sigaction sa;
    pid_t pid;

    /* Evitar zombis: SIGCHLD recoge a los hijos (SA_RESTART reanuda accept) */
    sa.sa_handler = manejador_hijos;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    /* 1. socket(): abre el canal */
    conector = socket(AF_INET, SOCK_STREAM, 0);
    if (conector < 0) { perror("socket"); exit(EXIT_FAILURE); }
    setsockopt(conector, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. bind(): publica la direccion (cualquier IP local, puerto 9999) */
    memset(&dir_servidor, 0, sizeof(dir_servidor));
    dir_servidor.sin_family      = AF_INET;
    dir_servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    dir_servidor.sin_port        = htons(PUERTO);
    if (bind(conector, (struct sockaddr *)&dir_servidor, sizeof(dir_servidor)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }

    /* 3. listen(): preparado para recibir conexiones */
    if (listen(conector, COLA) < 0) { perror("listen"); exit(EXIT_FAILURE); }
    printf("Servidor escuchando en el puerto %d...\n", PUERTO);
    fflush(stdout);

    /* 4. Bucle principal: accept() y fork() por cada cliente */
    while (1) {
        long_cliente = sizeof(dir_cliente);
        sock_cliente = accept(conector, (struct sockaddr *)&dir_cliente, &long_cliente);
        if (sock_cliente < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }
        printf("Conexion aceptada desde %s\n", inet_ntoa(dir_cliente.sin_addr));

        fflush(stdout);                   /* vaciar buffer antes de fork para no duplicar salida */
        pid = fork();
        if (pid < 0) {
            perror("fork");
            close(sock_cliente);
        } else if (pid == 0) {            /* HIJO */
            close(conector);              /* el hijo no necesita el socket de escucha */
            atender_cliente(sock_cliente);
            close(sock_cliente);
            printf("Hijo %d: archivo enviado a %s\n", getpid(), inet_ntoa(dir_cliente.sin_addr));
            fflush(stdout);
            exit(EXIT_SUCCESS);
        } else {                          /* PADRE */
            close(sock_cliente);          /* el padre vuelve a accept() */
        }
    }
    return 0;
}
