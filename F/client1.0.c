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

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convertir direcciones IPv4 e IPv6 de texto a binario
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char matricula[10], password[10];
    printf("Ingrese su matrícula: ");
    scanf("%s", matricula);
    printf("Ingrese su contraseña: ");
    scanf("%s", password);

    send(sock, matricula, strlen(matricula), 0);
    send(sock, password, strlen(password), 0);

    // Recibir respuesta del servidor sobre la autenticación
    valread = read(sock, buffer, 1024);
    printf("%s\n", buffer);

    if (strcmp(buffer, "Login successful") == 0) {
        int opcion;
        do {
            printf("Seleccione un examen:\n1. Matemáticas\n2. Español\n3. Inglés\n4. Salir\n");
            scanf("%d", &opcion);

            if (opcion >= 1 && opcion <= 3) {
                send(sock, &opcion, sizeof(int), 0);

                for (int i = 0; i < 10; i++) {
                    char pregunta[256];
                    char opciones[3][100];
                    int respuesta;

                    // Recibir pregunta
                    valread = read(sock, pregunta, sizeof(pregunta));
                    printf("%s\n", pregunta);

                    // Recibir opciones
                    for (int j = 0; j < 3; j++) {
                        valread = read(sock, opciones[j], sizeof(opciones[j]));
                        printf("%d. %s\n", j + 1, opciones[j]);
                    }

                    // Enviar respuesta
                    printf("Ingrese su respuesta: ");
                    scanf("%d", &respuesta);
                    send(sock, &respuesta, sizeof(int), 0);
                }

                int correctas;
                valread = read(sock, &correctas, sizeof(int));
                printf("Calificación final: %d/10\n", correctas);
            } else if (opcion != 4) {
                printf("Opción no válida. Intente de nuevo.\n");
            }
        } while (opcion != 4);
    }

    close(sock);
    return 0;
}
