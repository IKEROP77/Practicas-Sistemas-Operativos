/*
 * Cliente.c - Practica 2 (Comunicaciones en red)
 * Uso: Cliente IP_Servidor
 * Se conecta al servidor (puerto 9999), recibe el archivo y lo muestra por pantalla.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PUERTO  9999
#define TAM_BUF 4096

int main(int argc, char *argv[]) {
    int conector;
    struct sockaddr_in dir_servidor;
    char buffer[TAM_BUF];
    ssize_t recibidos;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s IP_Servidor\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 1. socket(): abre el canal */
    conector = socket(AF_INET, SOCK_STREAM, 0);
    if (conector < 0) { perror("socket"); exit(EXIT_FAILURE); }

    /* 2. Direccion del servidor */
    memset(&dir_servidor, 0, sizeof(dir_servidor));
    dir_servidor.sin_family = AF_INET;
    dir_servidor.sin_port   = htons(PUERTO);
    dir_servidor.sin_addr.s_addr = inet_addr(argv[1]);
    if (dir_servidor.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "IP invalida: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    /* 3. connect(): peticion de conexion */
    if (connect(conector, (struct sockaddr *)&dir_servidor, sizeof(dir_servidor)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    /* 4. Recibir hasta que el servidor cierre (read devuelve 0) y mostrar */
    while ((recibidos = read(conector, buffer, TAM_BUF)) > 0) {
        if (write(STDOUT_FILENO, buffer, recibidos) < 0) {
            perror("write");
            break;
        }
    }
    if (recibidos < 0) perror("read");

    close(conector);
    return 0;
}
