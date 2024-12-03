#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUF 1024

// Función para manejar la conexión al servidor
void manejar_conexion(int server_sock) {
    char buffer[MAX_BUF];
    char matricula[10], password[10];
    int seleccion;

    // Ingresar matrícula y contraseña
    printf("Ingrese su matrícula: ");
    scanf("%s", matricula);
    printf("Ingrese su contraseña: ");
    scanf("%s", password);

    // Enviar datos de login al servidor
    snprintf(buffer, sizeof(buffer), "%s %s", matricula, password);
    send(server_sock, buffer, strlen(buffer) + 1, 0);

    // Recibir respuesta del servidor (Login)
    recv(server_sock, buffer, sizeof(buffer), 0);
    if (strcmp(buffer, "Login failed") == 0) {
        printf("Error en el login. Intente nuevamente.\n");
        close(server_sock);
        return;
    }
    printf("%s\n", buffer); // Si el login es exitoso

    // Bucle para manejar los 3 exámenes
    for (int examen = 0; examen < 3; examen++) {
        // Recibir y mostrar el menú de exámenes
        recv(server_sock, buffer, sizeof(buffer), 0);
        printf("%s", buffer);

        // Elegir el examen
        printf("Seleccione el examen (1: Matemáticas, 2: Español, 3: Inglés): ");
        scanf("%d", &seleccion);
        send(server_sock, &seleccion, sizeof(seleccion), 0);

        // Recibir y responder las preguntas del examen
        int correctas = 0;
        for (int i = 0; i < 10; i++) {
            char pregunta[256], opcion1[100], opcion2[100], opcion3[100];
            int respuesta, respuesta_correcta;

            // Recibir pregunta y opciones
            recv(server_sock, pregunta, sizeof(pregunta), 0);
            recv(server_sock, opcion1, sizeof(opcion1), 0);
            recv(server_sock, opcion2, sizeof(opcion2), 0);
            recv(server_sock, opcion3, sizeof(opcion3), 0);

            printf("\nPregunta %d: %s\n", i + 1, pregunta);
            printf("1. %s\n", opcion1);
            printf("2. %s\n", opcion2);
            printf("3. %s\n", opcion3);

            // Elegir respuesta
            printf("Selecciona tu respuesta (1, 2 o 3): ");
            scanf("%d", &respuesta);
            send(server_sock, &respuesta, sizeof(respuesta), 0);

            // Recibir la respuesta correcta (opcional)
            recv(server_sock, &respuesta_correcta, sizeof(respuesta_correcta), 0);

            // Verificar si la respuesta es correcta
            if (respuesta == respuesta_correcta) {
                correctas++;
            }
        }

        // Recibir calificación
        recv(server_sock, &correctas, sizeof(correctas), 0);
        printf("Tu calificación en el examen de %d es: %d/10\n", examen + 1, correctas);
    }

    close(server_sock);
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;

    // Crear socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket falló");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Conectar al servidor
    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Conexión fallida");
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor en el puerto %d\n", PORT);

    // Manejar la conexión con el servidor
    manejar_conexion(server_sock);

    return 0;
}
