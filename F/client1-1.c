#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error al crear socket \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir dirección IP a formato binario
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\n Dirección no válida o no soportada \n");
        return -1;
    }

    // Conectarse al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Fallo en la conexión \n");
        return -1;
    }

    // Iniciar sesión
    char matricula[10], password[10];
    printf("Ingrese su matrícula: ");
    scanf("%s", matricula);
    printf("Ingrese su contraseña: ");
    scanf("%s", password);

    // Enviar credenciales
    snprintf(buffer, sizeof(buffer), "%s %s", matricula, password);
    send(sock, buffer, strlen(buffer), 0);

    // Leer respuesta del servidor
    valread = read(sock, buffer, sizeof(buffer));
    buffer[valread] = '\0';
    printf("%s\n", buffer);

    if (strcmp(buffer, "Login successful") != 0) {
        printf("No se pudo iniciar sesión. Cerrando el cliente.\n");
        close(sock);
        return 0;
    }

    // Realizar los 3 exámenes
    for (int examen = 1; examen <= 3; examen++) {
        printf("\nIniciando examen %d...\n", examen);
        int correctas = 0;

        // Responder 10 preguntas
        for (int i = 0; i < 10; i++) {
            char pregunta[256];
            char opciones[3][100];
            int respuesta;

            // Leer pregunta
            valread = read(sock, pregunta, sizeof(pregunta));
            pregunta[valread] = '\0';
            printf("\n%s\n", pregunta);

            // Leer opciones
            for (int j = 0; j < 3; j++) {
                valread = read(sock, opciones[j], sizeof(opciones[j]));
                opciones[j][valread] = '\0';
                printf("%d. %s\n", j + 1, opciones[j]);
            }

            // Leer respuesta del usuario
            do {
                printf("Ingrese su respuesta (1-3): ");
                scanf("%d", &respuesta);
                if (respuesta < 1 || respuesta > 3) {
                    printf("Respuesta no válida. Intente de nuevo.\n");
                }
            } while (respuesta < 1 || respuesta > 3);

            // Enviar respuesta
            send(sock, &respuesta, sizeof(respuesta), 0);
        }

        // Leer calificación final del examen
        valread = read(sock, &correctas, sizeof(correctas));
        printf("Examen %d finalizado. Calificación: %d/10\n", examen, correctas);
    }

    printf("\nHas completado los 3 exámenes. Gracias por participar.\n");
    close(sock);
    return 0;
}
