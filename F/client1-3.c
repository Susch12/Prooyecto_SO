#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_BUF 1024
#define TIEMPO_MAXIMO 600 // 10 minutos en segundos

// Declaración de la función antes de su uso
int obtener_respuesta_con_tiempo();

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
    printf("Login successful\n");

    // Recibir el menú de selección de exámenes
    recv(server_sock, buffer, sizeof(buffer), 0);
    printf("Menú recibido: %s\n", buffer);  // Imprime el contenido del menú

    printf("Seleccione el examen (1: Matemáticas, 2: Español, 3: Inglés): ");
    scanf("%d", &seleccion);
    send(server_sock, &seleccion, sizeof(seleccion), 0);

    // Bucle para manejar los 3 exámenes
    for (int examen = 0; examen < 3; examen++) {
        // Recibir y mostrar las preguntas
        for (int i = 0; i < 10; i++) {
            char pregunta[256], opcion1[100], opcion2[100], opcion3[100];
            int respuesta_usuario, respuesta_correcta;

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
            scanf("%d", &respuesta_usuario);
            send(server_sock, &respuesta_usuario, sizeof(respuesta_usuario), 0);

            // Recibir la respuesta correcta
            recv(server_sock, &respuesta_correcta, sizeof(respuesta_correcta), 0);

            // Verificar si la respuesta es correcta
            if (respuesta_usuario == respuesta_correcta) {
                printf("Respuesta correcta!\n");
            } else {
                printf("Respuesta incorrecta.\n");
            }
        }

        // Recibir la calificación final para el examen
        int calificacion_final;  // Nueva variable para la calificación
        recv(server_sock, &calificacion_final, sizeof(calificacion_final), 0);
        printf("Tu calificación en el examen %d es: %d/10\n", examen + 1, calificacion_final);
    }

    close(server_sock);
}

// Función para obtener la respuesta del usuario con un límite de tiempo
int obtener_respuesta_con_tiempo() {
    time_t start_time, current_time;
    double elapsed_time;
    int respuesta;

    // Comienza el temporizador
    time(&start_time);

    printf("Tienes %d segundos para responder.\n", TIEMPO_MAXIMO);

    // Bucle para esperar la respuesta del usuario
    while (1) {
        time(&current_time);
        elapsed_time = difftime(current_time, start_time);

        if (elapsed_time >= TIEMPO_MAXIMO) {
            printf("Tiempo agotado. Respuesta incorrecta.\n");
            return -1; // Respuesta incorrecta si se agotó el tiempo
        }

        // Leer la respuesta del usuario
        if (scanf("%d", &respuesta) == 1 && (respuesta >= 1 && respuesta <= 3)) {
            return respuesta; // Respuesta válida
        }

        printf("Respuesta inválida. Por favor, selecciona 1, 2 o 3: ");
    }
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
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Cambiar si el servidor está en otra dirección

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
