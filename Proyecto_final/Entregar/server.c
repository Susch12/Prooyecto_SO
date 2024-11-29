#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_USERS 20
#define MAX_QUESTIONS 50
#define QUESTIONS_PER_EXAM 5
#define USER_FILE "usuarios.txt"

// Estructuras
typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} Client;

typedef struct {
    char pregunta[256];
    char opciones[3][100];
    int correcta; // Índice de la opción correcta (0, 1 o 2)
} Pregunta;

typedef struct {
    char nombre[50];
    Pregunta preguntas[MAX_QUESTIONS];
    int total_preguntas;
} Materia;

typedef struct {
    char matricula[20];
    char password[20];
} User;

// Variables globales
User usuarios[MAX_USERS];
int total_usuarios = 0;
Materia materias[3];
int total_materias = 0;

// Prototipos
void cargar_usuarios();
bool validar_usuario(const char *matricula, const char *password);
void cargar_banco_preguntas();
void realizar_examen(int client_socket, int materia_id);
void mostrar_menu_examenes(int client_socket);
void *client_handler(void *arg);

// Cargar usuarios
void cargar_usuarios() {
    FILE *file = fopen(USER_FILE, "r");
    if (!file) {
        perror("Error al abrir el archivo de usuarios");
        exit(EXIT_FAILURE);
    }

    char linea[50];
    total_usuarios = 0;

    while (fgets(linea, sizeof(linea), file)) {
        if (sscanf(linea, "%s %s", usuarios[total_usuarios].matricula, usuarios[total_usuarios].password) == 2) {
            total_usuarios++;
            if (total_usuarios >= MAX_USERS) break;
        }
    }
    fclose(file);
    printf("%d", usuarios);
    printf("Usuarios cargados: %d\n", total_usuarios);
}

// Validar usuario
bool validar_usuario(const char *matricula, const char *password) {
    for (int i = 0; i < total_usuarios; i++) {
        if (strcmp(usuarios[i].matricula, matricula) == 0 && strcmp(usuarios[i].password, password) == 0) {
            return true;
        }
    }
    return false;
}

// Cargar preguntas desde archivos
void cargar_banco_preguntas() {
    const char *archivos[] = {"matematicas.txt", "espanol.txt", "ingles.txt"};
    const char *nombres[] = {"Matemáticas", "Español", "Inglés"};

    for (int i = 0; i < 3; i++) {
        FILE *file = fopen(archivos[i], "r");
        if (!file) continue;

        strcpy(materias[i].nombre, nombres[i]);
        materias[i].total_preguntas = 0;

        char linea[512];
        while (fgets(linea, sizeof(linea), file)) {
            Pregunta *preg = &materias[i].preguntas[materias[i].total_preguntas];
            if (sscanf(linea, "%[^\n]", preg->pregunta) == 1) {
                for (int j = 0; j < 3; j++) {
                    fgets(linea, sizeof(linea), file);
                    sscanf(linea, "%[^\n]", preg->opciones[j]);
                }
                fgets(linea, sizeof(linea), file);
                preg->correcta = atoi(linea) - 1; // Convertir a índice
                materias[i].total_preguntas++;
            }
        }
        fclose(file);
    }
    total_materias = 3;
    printf("Banco de preguntas cargado exitosamente.\n");
}

// Realizar examen
void realizar_examen(int client_socket, int materia_id) {
    Materia *materia = &materias[materia_id];
    if (materia->total_preguntas < QUESTIONS_PER_EXAM) {
        write(client_socket, "No hay suficientes preguntas.\n", 31);
        return;
    }

    int indices[QUESTIONS_PER_EXAM];
    srand(time(NULL));
    for (int i = 0; i < QUESTIONS_PER_EXAM; i++) indices[i] = rand() % materia->total_preguntas;

    char buffer[512];
    int puntaje = 0;
    for (int i = 0; i < QUESTIONS_PER_EXAM; i++) {
        Pregunta *preg = &materia->preguntas[indices[i]];
        snprintf(buffer, sizeof(buffer), "Pregunta %d: %s\n1. %s\n2. %s\n3. %s\nTu respuesta: ",
                 i + 1, preg->pregunta, preg->opciones[0], preg->opciones[1], preg->opciones[2]);
        write(client_socket, buffer, strlen(buffer));

        char respuesta[10];
        int bytes_read = read(client_socket, respuesta, sizeof(respuesta) - 1);
        if (bytes_read <= 0) return;

        int respuesta_int = atoi(respuesta) - 1;
        if (respuesta_int == preg->correcta) puntaje++;
    }
    snprintf(buffer, sizeof(buffer), "Examen terminado. Puntaje: %d/%d\n", puntaje, QUESTIONS_PER_EXAM);
    write(client_socket, buffer, strlen(buffer));
}

// Mostrar menú de exámenes
void mostrar_menu_examenes(int client_socket) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Selecciona un examen:\n1. %s\n2. %s\n3. %s\n",
             materias[0].nombre, materias[1].nombre, materias[2].nombre);
    write(client_socket, buffer, strlen(buffer));

    char opcion[10];
    int bytes_read = read(client_socket, opcion, sizeof(opcion) - 1);
    if (bytes_read > 0) {
        opcion[bytes_read - 1] = '\0';
        int seleccion = atoi(opcion) - 1;
        if (seleccion >= 0 && seleccion < total_materias) {
            realizar_examen(client_socket, seleccion);
        } else {
            write(client_socket, "Opción inválida.\n", 17);
        }
    }
}

// Manejar cliente
void *client_handler(void *arg) {
    Client *client = (Client *)arg;
    char matricula[20], password[20];

    write(client->socket_fd, "Introduce tu matrícula:\n", 25);
    int bytes_read = read(client->socket_fd, matricula, sizeof(matricula) - 1);
    if (bytes_read <= 0) goto desconectar_cliente;
    matricula[bytes_read - 1] = '\0';

    write(client->socket_fd, "Introduce tu contraseña:\n", 26);
    bytes_read = read(client->socket_fd, password, sizeof(password) - 1);
    if (bytes_read <= 0) goto desconectar_cliente;
    password[bytes_read - 1] = '\0';

    if (validar_usuario(matricula, password)) {
        write(client->socket_fd, "Acceso concedido.\n", 18);
        mostrar_menu_examenes(client->socket_fd);
    } else {
        write(client->socket_fd, "Acceso denegado.\n", 17);
    }

desconectar_cliente:
    close(client->socket_fd);
    free(client);
    pthread_exit(NULL);
}
// Función principal del servidor
int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    // Cargar usuarios desde archivo
    cargar_usuarios();
    cargar_banco_preguntas();

    // Crear el socket del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Vincular el socket a la dirección y puerto
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al vincular el socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Configurar el socket para escuchar conexiones
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("Error al configurar escucha en el socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d...\n", PORT);

    // Manejar conexiones entrantes
    while (1) {
        Client *client = malloc(sizeof(Client));
        socklen_t client_len = sizeof(client->address);

        if ((client->socket_fd = accept(server_fd, (struct sockaddr *)&client->address, &client_len)) == -1) {
            perror("Error al aceptar conexión");
            free(client);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_handler, client) != 0) {
            perror("Error al crear el hilo para el cliente");
            close(client->socket_fd);
            free(client);
        } else {
            pthread_detach(client_thread);
        }
    }

    close(server_fd);
    return 0;
}
