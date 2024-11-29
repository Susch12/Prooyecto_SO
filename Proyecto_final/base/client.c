#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_LONGITUD 512

int main(int argc, char *argv[]) {
    int socket_desc;
    struct sockaddr_in server;
    char server_reply[1024];
    char message[MAX_LONGITUD];
    int read_size;

    // Validar argumentos
    if (argc != 3) {
        printf("Uso: %s <IP del servidor> <Puerto>\n", argv[0]);
        return 1;
    }

    // Crear socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("No se pudo crear el socket");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));

    // Conectar al servidor
    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Error de conexión");
        return 1;
    }
    printf("[+] Conectado al servidor.\n");

    // Autenticación
    read_size = recv(socket_desc, server_reply, sizeof(server_reply), 0);
    server_reply[read_size] = '\0';
    printf("%s", server_reply);
    fgets(message, MAX_LONGITUD, stdin);
    send(socket_desc, message, strlen(message), 0);

    read_size = recv(socket_desc, server_reply, sizeof(server_reply), 0);
    server_reply[read_size] = '\0';
    printf("%s", server_reply);
    fgets(message, MAX_LONGITUD, stdin);
    send(socket_desc, message, strlen(message), 0);

    // Verificar autenticación
    read_size = recv(socket_desc, server_reply, sizeof(server_reply), 0);
    server_reply[read_size] = '\0';
    printf("%s", server_reply);
    if (strstr(server_reply, "fallida")) {
        close(socket_desc);
        return 1;
    }

    // Seleccionar el examen
    read_size = recv(socket_desc, server_reply, sizeof(server_reply), 0);
    server_reply[read_size] = '\0';
    printf("%s", server_reply);
    fgets(message, MAX_LONGITUD, stdin);
    send(socket_desc, message, strlen(message), 0);

    // Recibir preguntas y responder
    while ((read_size = recv(socket_desc, server_reply, sizeof(server_reply), 0)) > 0) {
        server_reply[read_size] = '\0';
        printf("%s", server_reply);

        if (strstr(server_reply, "Examen terminado")) {
            break;
        } else if (strstr(server_reply, "agotado")) {
            continue; // Pasar a la siguiente pregunta en caso de timeout
        }

        fgets(message, MAX_LONGITUD, stdin);
        send(socket_desc, message, strlen(message), 0);
    }

    close(socket_desc);
    return 0;
}

