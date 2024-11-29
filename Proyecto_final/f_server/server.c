#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define PORT 8080
#define MAX_PREGUNTAS 100
#define MAX_LONGITUD 256
#define MAX_CLIENTES 10
#define MAX_USUARIOS 20 // Definir un límite máximo de usuarios

typedef struct {
    char categoria[MAX_LONGITUD];
    char numero[MAX_LONGITUD];
    char pregunta[MAX_LONGITUD];
    char opcionA[MAX_LONGITUD];
    char opcionB[MAX_LONGITUD];
    char opcionC[MAX_LONGITUD];
    char respuesta_correcta;
} Pregunta;

typedef struct {
    char matricula[MAX_LONGITUD];
    int puntaje;
} Resultado;

Pregunta preguntas[MAX_PREGUNTAS];
int total_preguntas = 0;
Resultado resultados[3]; // Para almacenar resultados de los últimos 3 usuarios
int resultado_count = 0;

typedef struct {
    char matricula[MAX_LONGITUD];
    char password[MAX_LONGITUD];
} User;

User users[MAX_USUARIOS];
int user_count = 0;
int pregunta_timeout = 0;

// Manejador de señal para el timeout de la pregunta
void manejar_timeout(int signo) {
    pregunta_timeout = 1;
}

// Función para cargar preguntas desde un archivo
int cargar_preguntas(Pregunta preguntas[]) {
    FILE *file = fopen("preguntas.txt", "r");
    if (!file) {
        perror("Error al abrir el archivo de preguntas");
        return 0;
    }

    char linea[MAX_LONGITUD];
    while (fgets(linea, sizeof(linea), file) && total_preguntas < MAX_PREGUNTAS) {
        linea[strcspn(linea, "\n")] = 0;

        Pregunta p;
        char *token = strtok(linea, "|");

        if (token != NULL) strncpy(p.categoria, token, MAX_LONGITUD);
        token = strtok(NULL, ".");
        if (token != NULL) {
            strncpy(p.numero, token, MAX_LONGITUD);
            token = strtok(NULL, "|");
            if (token != NULL) strncpy(p.pregunta, token, MAX_LONGITUD);
        }

        token = strtok(NULL, "|");
        if (token != NULL) strncpy(p.opcionA, token + 2, MAX_LONGITUD);
        token = strtok(NULL, "|");
        if (token != NULL) strncpy(p.opcionB, token + 2, MAX_LONGITUD);
        token = strtok(NULL, "|");
        if (token != NULL) strncpy(p.opcionC, token + 2, MAX_LONGITUD);
        
        token = strtok(NULL, "|");
        if (token != NULL) p.respuesta_correcta = toupper(token[0]);

        preguntas[total_preguntas++] = p;
    }

    fclose(file);
    printf("Preguntas cargadas con éxito: %d\n", total_preguntas);
    return total_preguntas;
}

// Función para cargar usuarios
void load_users(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error abriendo el archivo de usuarios");
        exit(1);
    }

    while (fscanf(file, "%[^,],%s\n", users[user_count].matricula, users[user_count].password) == 2) {
        user_count++;
        if (user_count >= MAX_USUARIOS) break;
    }
    fclose(file);
}

// Función para filtrar preguntas por categoría (examen)
int filtrar_preguntas_por_examen(Pregunta preguntas[], Pregunta preguntas_filtradas[], const char *examen, int total_preguntas) {
    int index = 0;
    for (int i = 0; i < total_preguntas; i++) {
        if (strcmp(preguntas[i].categoria, examen) == 0) {
            preguntas_filtradas[index++] = preguntas[i];
        }
    }
    return index;
}

// Función para autenticar usuario
int authenticate(char *matricula, char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].matricula, matricula) == 0 && strcmp(users[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

// Función para mezclar preguntas
void mezclar_preguntas(Pregunta preguntas[], int n) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        int j = rand() % n;
        Pregunta temp = preguntas[i];
        preguntas[i] = preguntas[j];
        preguntas[j] = temp;
    }
}

// Función para manejar las conexiones de los clientes
void manejar_cliente(int nuevo_socket) {
    char buffer[1024] = {0};
    int valread;
    char usuario[MAX_LONGITUD];
    char contrasena[MAX_LONGITUD];
    char examen[MAX_LONGITUD];
    int puntaje = 0;

    send(nuevo_socket, "Por favor, ingrese su matrícula: ", 32, 0);
    valread = read(nuevo_socket, usuario, MAX_LONGITUD);
    if (valread <= 0) {
        close(nuevo_socket);
        return;
    }
    usuario[valread] = '\0';

    send(nuevo_socket, "Por favor, ingrese su contraseña: ", 34, 0);
    valread = read(nuevo_socket, contrasena, MAX_LONGITUD);
    if (valread <= 0) {
        close(nuevo_socket);
        return;
    }
    contrasena[valread] = '\0';

    if (authenticate(usuario, contrasena)) {
        send(nuevo_socket, "Autenticación exitosa. Iniciando el juego...\n", 44, 0);
    } else {
        send(nuevo_socket, "Autenticación fallida. Cerrando conexión...\n", 44, 0);
        close(nuevo_socket);
        return;
    }

    send(nuevo_socket, "Seleccione el examen:\nI. Matemáticas\nII. Español\nIII. Inglés\n", 63, 0);
    valread = read(nuevo_socket, examen, MAX_LONGITUD);
    if (valread <= 0) {
        close(nuevo_socket);
        return;
    }
    examen[valread] = '\0';
    for (int i = 0; examen[i]; i++) examen[i] = tolower(examen[i]);

    if (strstr(examen, "matematicas") != NULL) {
        strcpy(examen, "Matemáticas");
    } else if (strstr(examen, "espanol") != NULL || strstr(examen, "español") != NULL) {
        strcpy(examen, "Español");
    } else if (strstr(examen, "ingles") != NULL) {
        strcpy(examen, "Inglés");
    } else {
        send(nuevo_socket, "Examen no válido. Cerrando conexión...\n", 38, 0);
        close(nuevo_socket);
        return;
    }

    Pregunta preguntas_filtradas[MAX_PREGUNTAS];
    int total_preguntas_filtradas = filtrar_preguntas_por_examen(preguntas, preguntas_filtradas, examen, total_preguntas);
    if (total_preguntas_filtradas == 0) {
        send(nuevo_socket, "No hay preguntas disponibles para este examen.\n", 47, 0);
        close(nuevo_socket);
        return;
    }

    Pregunta preguntas_aleatorias[10];
    int num_preguntas = total_preguntas_filtradas < 10 ? total_preguntas_filtradas : 10;
    mezclar_preguntas(preguntas_filtradas, total_preguntas_filtradas);
    for (int i = 0; i < num_preguntas; i++) {
        preguntas_aleatorias[i] = preguntas_filtradas[i];
    }

    signal(SIGALRM, manejar_timeout);

    for (int i = 0; i < num_preguntas; i++) {
        pregunta_timeout = 0;
        sprintf(buffer, "Pregunta: %s\nA) %s\nB) %s\nC) %s\n\n",
                preguntas_aleatorias[i].pregunta,
                preguntas_aleatorias[i].opcionA,
                preguntas_aleatorias[i].opcionB,
                preguntas_aleatorias[i].opcionC);
        send(nuevo_socket, buffer, strlen(buffer), 0);

        alarm(600);
        valread = read(nuevo_socket, buffer, 1024);

        if (!pregunta_timeout && valread > 0) {
            alarm(0);
            if (toupper(buffer[0]) == preguntas_aleatorias[i].respuesta_correcta) {
                puntaje++;
            }
        } else {
            send(nuevo_socket, "Tiempo de respuesta agotado para esta pregunta.\n", 48, 0);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    sprintf(buffer, "Examen terminado. Su puntaje es %d/%d.\n", puntaje, num_preguntas);
    send(nuevo_socket, buffer, strlen(buffer), 0);

    strncpy(resultados[resultado_count].matricula, usuario, MAX_LONGITUD);
    resultados[resultado_count].puntaje = puntaje;
    resultado_count++;

    if (resultado_count == 3) {
        printf("Resultados de los últimos 3 usuarios:\n");
        for (int i = 0; i < 3; i++) {
            printf("Usuario: %s - Puntaje: %d/%d\n", resultados[i].matricula, resultados[i].puntaje, num_preguntas);
        }
        resultado_count = 0;
    }

    close(nuevo_socket);
}

int main(int argc, char const *argv[]) {
    int servidor_fd, nuevo_socket, opt = 1;
    struct sockaddr_in direccion;
    int addrlen = sizeof(direccion);

    load_users("usuarios.txt");
    total_preguntas = cargar_preguntas(preguntas);
    if (total_preguntas == 0) {
        printf("No se pudieron cargar preguntas. Terminando el servidor.\n");
        return -1;
    }

    if ((servidor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Error al configurar el socket");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = INADDR_ANY;
    direccion.sin_port = htons(PORT);

    if (bind(servidor_fd, (struct sockaddr *)&direccion, sizeof(direccion)) < 0) {
        perror("Error en el bind");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(servidor_fd, MAX_CLIENTES) < 0) {
        perror("Error en el listen");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor en espera de conexiones en el puerto %d\n", PORT);

    while ((nuevo_socket = accept(servidor_fd, (struct sockaddr *)&direccion, (socklen_t*)&addrlen)) >= 0) {
        manejar_cliente(nuevo_socket);
    }

    if (nuevo_socket < 0) {
        perror("Error al aceptar la conexión");
    }

    close(servidor_fd);
    return 0;
}

