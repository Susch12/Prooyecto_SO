#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 512

// Función para leer la respuesta del usuario con un límite de tiempo
int leer_respuesta_con_timeout(int socket_fd, char *buffer, size_t buffer_size, int tiempo_limite) {
    fd_set readfds;
    struct timeval tv;
    int n;

    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    tv.tv_sec = tiempo_limite;  // Tiempo límite en segundos
    tv.tv_usec = 0;

    n = select(socket_fd + 1, &readfds, NULL, NULL, &tv);
    if (n > 0) {
        // Hay datos disponibles para leer
        return read(socket_fd, buffer, buffer_size);
    } else if (n == 0) {
        // Tiempo de espera agotado
        return -1;
    } else {
        perror("select");
        return -1;
    }
}

// Función para manejar la autenticación del usuario
int autenticar_usuario(int socket_fd) {
    char matricula[20], password[20];
    int bytes_read;

    // Solicitar matrícula
    printf("Introduce tu matrícula: ");
    fgets(matricula, sizeof(matricula), stdin);
    matricula[strcspn(matricula, "\n")] = '\0';  // Eliminar salto de línea

    write(socket_fd, matricula, strlen(matricula) + 1);  // Enviar matrícula

    // Solicitar contraseña
    printf("Introduce tu contraseña: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';  // Eliminar salto de línea

    write(socket_fd, password, strlen(password) + 1);  // Enviar contraseña

    // Leer respuesta del servidor
    char respuesta[BUFFER_SIZE];
    bytes_read = read(socket_fd, respuesta, sizeof(respuesta) - 1);
    if (bytes_read <= 0) {
        perror("Error al leer respuesta del servidor");
        return 0;
    }
    respuesta[bytes_read] = '\0';
    printf("%s\n", respuesta);

    if (strstr(respuesta, "Acceso concedido") != NULL) {
        return 1;  // Usuario autenticado correctamente
    }
    return 0;  // Autenticación fallida
}

// Función para seleccionar el examen
int seleccionar_examen(int socket_fd) {
    char opcion[10];
    int bytes_read;

    // Leer el menú de exámenes
    bytes_read = read(socket_fd, opcion, sizeof(opcion) - 1);
    if (bytes_read <= 0) {
        perror("Error al leer el menú de exámenes");
        return -1;
    }
    opcion[bytes_read] = '\0';
    printf("%s\n", opcion);

    // Solicitar la opción del examen
    printf("Selecciona un examen (1-3): ");
    fgets(opcion, sizeof(opcion), stdin);
    opcion[strcspn(opcion, "\n")] = '\0';  // Eliminar salto de línea

    write(socket_fd, opcion, strlen(opcion) + 1);  // Enviar la opción seleccionada

    return atoi(opcion) - 1;  // Retornar el índice del examen seleccionado
}

// Función para contestar las preguntas del examen
void contestar_examen(int socket_fd) {
    char respuesta[10];
    int bytes_read;

    // Leer y responder las preguntas
    while (1) {
        bytes_read = read(socket_fd, respuesta, sizeof(respuesta) - 1);
        if (bytes_read <= 0) {
            perror("Error al leer pregunta");
            break;
        }

        respuesta[bytes_read] = '\0';
        printf("%s", respuesta);  // Mostrar la pregunta y opciones

        // Leer respuesta del usuario
        printf("Tu respuesta: ");
        fgets(respuesta, sizeof(respuesta), stdin);
        respuesta[strcspn(respuesta, "\n")] = '\0';  // Eliminar salto de línea

        write(socket_fd, respuesta, strlen(respuesta) + 1);  // Enviar la respuesta

        // Verificar si el examen ha terminado
        if (strstr(respuesta, "Examen terminado") != NULL) {
            break;
        }
    }
}

// Función principal del cliente
int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Crear el socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al conectar con el servidor");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Autenticación
    if (!autenticar_usuario(sockfd)) {
        printf("Autenticación fallida. Cerrando conexión...\n");
        close(sockfd);
        return 0;
    }

    // Seleccionar examen
    int examen_seleccionado = seleccionar_examen(sockfd);
    if (examen_seleccionado < 0) {
        printf("Opción inválida. Cerrando conexión...\n");
        close(sockfd);
        return 0;
    }

    // Contestar examen
    contestar_examen(sockfd);

    // Cerrar la conexión
    close(sockfd);
    return 0;
}

