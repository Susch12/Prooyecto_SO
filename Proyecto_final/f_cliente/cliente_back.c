#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_LONGITUD 256

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char username[MAX_LONGITUD];
    char password[MAX_LONGITUD];
    char respuesta[MAX_LONGITUD];
    
    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket falló");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir IP de texto a formato binario
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Dirección no válida / No se puede acceder al servidor");
        return -1;
    }

    // Conectar al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Conexión fallida");
        return -1;
    }

    printf("Conectado al servidor.\n");

    // Ingresar nombre de usuario
    printf("Ingrese nombre de usuario: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // Eliminar salto de línea
    send(sock, username, strlen(username), 0);

    // Esperar solicitud de contraseña del servidor
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, sizeof(buffer));
    printf("%s", buffer);  // Mostrar solicitud de contraseña

    // Ingresar la contraseña
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0; // Eliminar salto de línea
    send(sock, password, strlen(password), 0);

    // Recibir respuesta de autenticación
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, sizeof(buffer));
    printf("Respuesta del servidor: %s\n", buffer);
    if (strstr(buffer, "incorrectas") != NULL) {
        printf("Credenciales incorrectas. Terminando la conexión.\n");
        close(sock);
        return 0;
    }

    // Recibir y mostrar el menú de selección de categoría
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, sizeof(buffer));
    printf("%s\n", buffer);

    // Ingresar selección de categoría
    printf("Ingrese el número de la categoría deseada: ");
    fgets(respuesta, sizeof(respuesta), stdin);
    respuesta[strcspn(respuesta, "\n")] = 0; // Eliminar salto de línea
    send(sock, respuesta, strlen(respuesta), 0);

    // Bucle para recibir preguntas y enviar respuestas
    while (1) {
        // Limpiar el buffer
        memset(buffer, 0, sizeof(buffer));

        // Recibir pregunta del servidor
        int valread = read(sock, buffer, sizeof(buffer));
        if (valread <= 0) {
            printf("Conexión cerrada por el servidor.\n");
            break;
        }

        // Verificar si el servidor envió el mensaje de fin
        if (strcmp(buffer, "FIN") == 0) {
            printf("No hay más preguntas en esta categoría. Finalizando el cuestionario.\n");
            break;
        }

        // Mostrar pregunta al usuario
        printf("\nPregunta recibida:\n%s\n", buffer);

        // Pedir respuesta al usuario
        printf("Ingrese su respuesta: ");
        fgets(respuesta, sizeof(respuesta), stdin);
        respuesta[strcspn(respuesta, "\n")] = 0; // Eliminar salto de línea

        // Enviar respuesta al servidor
        send(sock, respuesta, strlen(respuesta), 0);

        // Esperar retroalimentación del servidor (correcta o incorrecta)
        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, sizeof(buffer));
        if (valread > 0) {
            printf("Respuesta del servidor: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}

