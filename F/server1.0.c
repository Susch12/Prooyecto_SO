#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_USERS 20
#define MAX_QUESTIONS 30
#define EXAM_QUESTIONS 10

typedef struct {
    char matricula[10];
    char password[10];
} Usuario;

typedef struct {
    char pregunta[256];
    char opciones[3][100];
    int correcta;
} Pregunta;

typedef struct {
    char matricula[10];
    int calificaciones[3];
} Resultado;

Usuario usuarios[MAX_USERS];
Pregunta preguntasMatematicas[MAX_QUESTIONS];
Pregunta preguntasEspanol[MAX_QUESTIONS];
Pregunta preguntasIngles[MAX_QUESTIONS];
Resultado resultados[MAX_USERS];

void cargarUsuarios() {
    FILE *file = fopen("users.bin", "rb");
    if (file == NULL) {
        perror("No se pudo abrir el archivo de usuarios");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_USERS; i++) {
        fscanf(file, "%s %s", usuarios[i].matricula, usuarios[i].password);
    }

    fclose(file);
}

void cargarPreguntas() {
    FILE *file;
    
    file = fopen("mate.bin", "rb");
    for (int i = 0; i < MAX_QUESTIONS; i++) {
        fscanf(file, "%[^\n]%*c", preguntasMatematicas[i].pregunta);
        for (int j = 0; j < 3; j++) {
            fscanf(file, "%[^\n]%*c", preguntasMatematicas[i].opciones[j]);
        }
        fscanf(file, "%d%*c", &preguntasMatematicas[i].correcta);
    }
    fclose(file);

    file = fopen("español.bin", "rb");
    for (int i = 0; i < MAX_QUESTIONS; i++) {
        fscanf(file, "%[^\n]%*c", preguntasEspanol[i].pregunta);
        for (int j = 0; j < 3; j++) {
            fscanf(file, "%[^\n]%*c", preguntasEspanol[i].opciones[j]);
        }
        fscanf(file, "%d%*c", &preguntasEspanol[i].correcta);
    }
    fclose(file);

    file = fopen("ingles.bin", "rb");
    for (int i = 0; i < MAX_QUESTIONS; i++) {
        fscanf(file, "%[^\n]%*c", preguntasIngles[i].pregunta);
        for (int j = 0; j < 3; j++) {
            fscanf(file, "%[^\n]%*c", preguntasIngles[i].opciones[j]);
        }
        fscanf(file, "%d%*c", &preguntasIngles[i].correcta);
    }
    fclose(file);
}

int validarUsuario(char *matricula, char *password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(usuarios[i].matricula, matricula) == 0 && strcmp(usuarios[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

void realizarExamen(Pregunta *preguntas, int client_socket) {
    int indices[EXAM_QUESTIONS];
    srand(time(NULL));
    for (int i = 0; i < EXAM_QUESTIONS; i++) {
        indices[i] = rand() % MAX_QUESTIONS;
    }

    int correctas = 0;
    for (int i = 0; i < EXAM_QUESTIONS; i++) {
        send(client_socket, preguntas[indices[i]].pregunta, sizeof(preguntas[indices[i]].pregunta), 0);
        for (int j = 0; j < 3; j++) {
            send(client_socket, preguntas[indices[i]].opciones[j], sizeof(preguntas[indices[i]].opciones[j]), 0);
        }
        int respuesta;
        recv(client_socket, &respuesta, sizeof(int), 0);
        if (respuesta == preguntas[indices[i]].correcta) {
            correctas++;
        }
    }
    send(client_socket, &correctas, sizeof(int), 0);
}

void guardarResultado(char *matricula, int calificacion, int examen) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(resultados[i].matricula, matricula) == 0) {
            resultados[i].calificaciones[examen] = calificacion;
            return;
        }
    }
    // Si no se encuentra, agregar nuevo resultado
    strcpy(resultados[MAX_USERS].matricula, matricula);
    resultados[MAX_USERS].calificaciones[examen] = calificacion;
}

void mostrarResultados() {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strlen(resultados[i].matricula) > 0) {
            printf("Usuario: %s\n", resultados[i].matricula);
            printf("Matemáticas: %d\n", resultados[i].calificaciones[0]);
            printf("Español: %d\n", resultados[i].calificaciones[1]);
            printf("Inglés: %d\n", resultados[i].calificaciones[2]);
        }
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    cargarUsuarios();
    cargarPreguntas();

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Adjuntar socket al puerto 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincular socket al puerto
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(server_fd, MAX_USERS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d\n", PORT);

    // Aceptar conexiones
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        printf("Nueva conexión aceptada\n");

        char matricula[10], password[10];
        recv(new_socket, matricula, sizeof(matricula), 0);
        recv(new_socket, password, sizeof(password), 0);

        if (validarUsuario(matricula, password)) {
            send(new_socket, "Login successful", strlen("Login successful"), 0);
            int opcion;
            recv(new_socket, &opcion, sizeof(int), 0);
            switch (opcion) {
                case 1:
                    realizarExamen(preguntasMatematicas, new_socket);
                    break;
                case 2:
                    realizarExamen(preguntasEspanol, new_socket);
                    break;
                case 3:
                    realizarExamen(preguntasIngles, new_socket);
                    break;
                default:
                    send(new_socket, "Opción no válida", strlen("Opción no válida"), 0);
            }
        } else {
            send(new_socket, "Invalid credentials", strlen("Invalid credentials"), 0);
        }

        close(new_socket);
    }

    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return 0;
}
