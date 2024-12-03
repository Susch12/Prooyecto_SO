#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

#define PORT 8080
#define MAX_USERS 20
#define MAX_CLIENTS 10
#define MAX_QUESTIONS 30
#define MAX_LONGITUD 100
#define MAX_PREGUNTAS 10

typedef struct {
    char matricula[10];
    char password[10];
    int calificaciones[3];
} Usuario;

typedef struct {
    char pregunta[256];
    char opcionA[100];
    char opcionB[100];
    char opcionC[100];
    char respuesta_correcta;
} Pregunta;

// Base de datos de usuarios y preguntas
Usuario usuarios[MAX_USERS];
Pregunta preguntas[3][MAX_QUESTIONS]; // 3 materias, cada una con varias preguntas
int resultados_recientes[3][3]; // Últimos 3 usuarios (matrícula y calificaciones)

// Mutex para proteger datos compartidos
pthread_mutex_t lock;

int pregunta_timeout = 0; // Variable global para el control del timeout

void deshabilitar_SIGPIPE() {
    signal(SIGPIPE, SIG_IGN);  // Ignorar la señal SIGPIPE
}

// Función para cargar usuarios desde un archivo binario
void cargar_usuarios_bin(const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "rb");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de usuarios");
        exit(EXIT_FAILURE);
    }

    size_t leidos = fread(usuarios, sizeof(Usuario), MAX_USERS, archivo);
    if (leidos == 0) {
        perror("Error al leer datos de usuarios");
        fclose(archivo);
        exit(EXIT_FAILURE);
    }

    fclose(archivo);
    printf("Usuarios cargados exitosamente (%lu usuarios).\n", leidos);
}

// Función para cargar preguntas desde un archivo binario
void cargar_banco_de_preguntas_bin(const char *nombre_archivo) {
    FILE *archivo = fopen(nombre_archivo, "rb");
    if (archivo == NULL) {
        perror("Error al abrir el archivo de preguntas");
        exit(EXIT_FAILURE);
    }

    size_t leidos = fread(preguntas, sizeof(Pregunta), MAX_QUESTIONS * 3, archivo);
    if (leidos == 0) {
        perror("Error al leer datos de preguntas");
        fclose(archivo);
        exit(EXIT_FAILURE);
    }

    fclose(archivo);
    printf("Preguntas cargadas exitosamente.\n");
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

// Función para enviar el menú de selección de exámenes al cliente
void enviar_menu_examen(int client_sock) {
  int bytes_enviados = send(client_sock, "Seleccione el examen:\n1. Matemáticas\n2. Español\n3. Inglés\n", strlen("Seleccione el examen:\n1. Matemáticas\n2. Español\n3. Inglés\n") + 1, 0);
if (bytes_enviados == -1) {
    perror("Error enviando menú de examen");
}
}

// Función para manejar el timeout
void manejar_timeout(int sig) {
    pregunta_timeout = 1; // Marca que se ha agotado el tiempo
}

// Función para filtrar las preguntas según la materia
int filtrar_preguntas_por_examen(Pregunta preguntas[3][MAX_QUESTIONS], Pregunta preguntas_filtradas[MAX_PREGUNTAS], const char *examen, int total_preguntas) {
    int index = 0;
    int materia_idx = 0;
    if (strcmp(examen, "Matemáticas") == 0) {
        materia_idx = 0;
    } else if (strcmp(examen, "Español") == 0) {
        materia_idx = 1;
    } else if (strcmp(examen, "Inglés") == 0) {
        materia_idx = 2;
    }

    for (int i = 0; i < total_preguntas; i++) {
        if (strlen(preguntas[materia_idx][i].pregunta) > 0) {
            preguntas_filtradas[index++] = preguntas[materia_idx][i];
        }
    }
    return index;
}

// Función para mezclar las preguntas (aleatorias)
void mezclar_preguntas(Pregunta preguntas_filtradas[MAX_PREGUNTAS], int total_preguntas_filtradas) {
    srand(time(NULL));
    for (int i = 0; i < total_preguntas_filtradas; i++) {
        int j = rand() % total_preguntas_filtradas;
        Pregunta temp = preguntas_filtradas[i];
        preguntas_filtradas[i] = preguntas_filtradas[j];
        preguntas_filtradas[j] = temp;
    }
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

    // Enviar el menú para seleccionar el examen
    enviar_menu_examen(client_sock);

    // Leer la opción seleccionada por el cliente
    int opcion_examen;
    read(client_sock, &opcion_examen, sizeof(opcion_examen));

    if (opcion_examen < 1 || opcion_examen > 3) {
        send(client_sock, "Opción inválida", strlen("Opción inválida") + 1, 0);
        close(client_sock);
        return NULL;
    }

    // Filtrar las preguntas por examen
    Pregunta preguntas_filtradas[MAX_PREGUNTAS];
    int total_preguntas_filtradas = filtrar_preguntas_por_examen(preguntas, preguntas_filtradas, opcion_examen == 1 ? "Matemáticas" : opcion_examen == 2 ? "Español" : "Inglés", MAX_QUESTIONS);
    if (total_preguntas_filtradas == 0) {
        send(client_sock, "No hay preguntas disponibles para este examen.\n", strlen("No hay preguntas disponibles para este examen.\n") + 1, 0);
        close(client_sock);
        return NULL;
    }

    // Mezclar las preguntas
    mezclar_preguntas(preguntas_filtradas, total_preguntas_filtradas);

    int correctas = 0;
    for (int i = 0; i < 10 && i < total_preguntas_filtradas; i++) {
        pregunta_timeout = 0;
        sprintf(buffer, "Pregunta %d: %s\nA) %s\nB) %s\nC) %s\n", i + 1, preguntas_filtradas[i].pregunta, preguntas_filtradas[i].opcionA, preguntas_filtradas[i].opcionB, preguntas_filtradas[i].opcionC);
        send(client_sock, buffer, strlen(buffer), 0);

        signal(SIGALRM, manejar_timeout);
        alarm(60); // 1 minuto para responder
        read(client_sock, buffer, sizeof(buffer));
        alarm(0); // Cancelar alarma

        if (!pregunta_timeout && buffer[0] == preguntas_filtradas[i].respuesta_correcta) {
            correctas++;
        } else {
            send(client_sock, "Tiempo agotado o respuesta incorrecta.\n", strlen("Tiempo agotado o respuesta incorrecta.\n") + 1, 0);
        }
    }

    // Enviar la calificación final
    send(client_sock, &correctas, sizeof(correctas), 0);

    pthread_mutex_lock(&lock);
    usuarios[user_index].calificaciones[opcion_examen - 1] = correctas;
    pthread_mutex_unlock(&lock);

    close(client_sock);
    return NULL;
}

// Función principal del servidor
int main() {
    int server_fd, client_sock, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    deshabilitar_SIGPIPE();
    pthread_mutex_init(&lock, NULL);

    // Cargar datos desde archivos binarios
    cargar_usuarios_bin("users.bin");
    cargar_banco_de_preguntas_bin("questions.bin");

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
