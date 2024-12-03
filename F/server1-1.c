#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define MAX_USERS 20
#define MAX_CLIENTS 10
#define MAX_QUESTIONS 30

typedef struct {
    char matricula[10];
    char password[10];
    int calificaciones[3];
} Usuario;

typedef struct {
    char pregunta[256];
    char opciones[3][100];
    int respuesta_correcta;
} Pregunta;

// Base de datos de usuarios y preguntas
Usuario usuarios[MAX_USERS];
Pregunta preguntas[3][MAX_QUESTIONS]; // 3 materias, cada una con varias preguntas
int resultados_recientes[3][3]; // Últimos 3 usuarios (matrícula y calificaciones)

// Mutex para proteger datos compartidos
pthread_mutex_t lock;

// Función para cargar datos iniciales (usuarios y preguntas)




void cargar_datos() {
    // Cargar usuarios
    for (int i = 0; i < MAX_USERS; i++) {
        snprintf(usuarios[i].matricula, 10, "user%d", i + 1);
        snprintf(usuarios[i].password, 10, "pass%d", i + 1);
        for (int j = 0; j < 3; j++) {
            usuarios[i].calificaciones[j] = 0;
        }
    }

    // Cargar preguntas (ejemplo sencillo)
    for (int materia = 0; materia < 3; materia++) {
        for (int i = 0; i < MAX_QUESTIONS; i++) {
            snprintf(preguntas[materia][i].pregunta, 256, "Pregunta %d de Materia %d", i + 1, materia + 1);
            for (int j = 0; j < 3; j++) {
                snprintf(preguntas[materia][i].opciones[j], 100, "Opción %d", j + 1);
            }
            preguntas[materia][i].respuesta_correcta = rand() % 3 + 1;
        }
    }
}

// Función para validar el usuario
int validar_usuario(char *matricula, char *password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(usuarios[i].matricula, matricula) == 0 &&
            strcmp(usuarios[i].password, password) == 0) {
            return i; // Retorna el índice del usuario
        }
    }
    return -1;
}

// Función para manejar a cada cliente
void *manejar_cliente(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);
    char buffer[1024];
    int user_index;

    // Validar usuario
    read(client_sock, buffer, sizeof(buffer));
    char matricula[10], password[10];
    sscanf(buffer, "%s %s", matricula, password);

    pthread_mutex_lock(&lock);
    user_index = validar_usuario(matricula, password);
    pthread_mutex_unlock(&lock);

    if (user_index == -1) {
        send(client_sock, "Login failed", strlen("Login failed") + 1, 0);
        close(client_sock);
        return NULL;
    }

    send(client_sock, "Login successful", strlen("Login successful") + 1, 0);

    // Enviar y manejar exámenes
    for (int examen = 0; examen < 3; examen++) {
        int correctas = 0;

        // Seleccionar preguntas aleatorias
        int usadas[MAX_QUESTIONS] = {0};
        for (int i = 0; i < 10; i++) {
            int pregunta_id;
            do {
                pregunta_id = rand() % MAX_QUESTIONS;
            } while (usadas[pregunta_id]);
            usadas[pregunta_id] = 1;

            // Enviar pregunta y opciones
            send(client_sock, preguntas[examen][pregunta_id].pregunta, strlen(preguntas[examen][pregunta_id].pregunta) + 1, 0);
            for (int j = 0; j < 3; j++) {
                send(client_sock, preguntas[examen][pregunta_id].opciones[j], strlen(preguntas[examen][pregunta_id].opciones[j]) + 1, 0);
            }

            // Recibir respuesta
            int respuesta;
            read(client_sock, &respuesta, sizeof(respuesta));

            // Verificar respuesta
            if (respuesta == preguntas[examen][pregunta_id].respuesta_correcta) {
                correctas++;
            }
        }

        // Enviar calificación
        send(client_sock, &correctas, sizeof(correctas), 0);

        pthread_mutex_lock(&lock);
        usuarios[user_index].calificaciones[examen] = correctas;
        pthread_mutex_unlock(&lock);
    }

    close(client_sock);
    return NULL;
}

int main() {
    int server_fd, client_sock, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    pthread_mutex_init(&lock, NULL);
    cargar_datos();

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket falló");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Enlazar socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error al enlazar");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor corriendo en el puerto %d\n", PORT);

    while (1) {
        if ((client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Error al aceptar conexión");
            continue;
        }

        pthread_t thread_id;
        new_sock = malloc(1);
        *new_sock = client_sock;
        pthread_create(&thread_id, NULL, manejar_cliente, (void *)new_sock);
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
