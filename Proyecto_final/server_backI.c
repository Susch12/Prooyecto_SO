#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MAX_PREGUNTAS 100
#define MAX_LONGITUD 256
#define MAX_CLIENTES 10

typedef struct {
    char categoria[MAX_LONGITUD];
    char numero[MAX_LONGITUD];
    char pregunta[MAX_LONGITUD];
    char opcionA[MAX_LONGITUD];
    char opcionB[MAX_LONGITUD];
    char opcionC[MAX_LONGITUD];
    char opcionD[MAX_LONGITUD];
    char respuesta_correcta;
} Pregunta;

Pregunta preguntas[MAX_PREGUNTAS];
int total_preguntas = 0;
#define MAX_USUARIOS 20 // Definir un límite máximo de usuarios

typedef struct {
    char matricula[MAX_LONGITUD];
    char password[MAX_LONGITUD];
} User;

User users[MAX_USUARIOS]; // Arreglo de usuarios
int user_count = 0; // Contador de usuarios cargados

// Función para cargar preguntas desde un archivo
int cargar_preguntas(Pregunta preguntas[]) {
    FILE *file = fopen("preguntas.txt", "r");
    if (!file) {
        perror("Error al abrir el archivo de preguntas");
        return 0;
    }

    char linea[MAX_LONGITUD];

    while (fgets(linea, sizeof(linea), file) && total_preguntas < MAX_PREGUNTAS) {
        linea[strcspn(linea, "\n")] = 0; // Quitar salto de línea

        Pregunta p;
        char *token = strtok(linea, "|");

        // Procesar cada parte de la línea para obtener los campos
        if (token != NULL) strncpy(p.categoria, token, MAX_LONGITUD);

        token = strtok(NULL, ".");
        if (token != NULL) {
            strncpy(p.numero, token, MAX_LONGITUD);
            token = strtok(NULL, "|");
            if (token != NULL) strncpy(p.pregunta, token, MAX_LONGITUD);
        }

        // Opciones de respuesta
        token = strtok(NULL, "|");
        if (token != NULL) strncpy(p.opcionA, token + 2, MAX_LONGITUD); // Quitar "a)"
        token = strtok(NULL, "|");
        if (token != NULL) strncpy(p.opcionB, token + 2, MAX_LONGITUD); // Quitar "b)"
        token = strtok(NULL, "|");
        if (token != NULL) strncpy(p.opcionC, token + 2, MAX_LONGITUD); // Quitar "c)"
        token = strtok(NULL, "|");
        if (token != NULL) strncpy(p.opcionD, token + 2, MAX_LONGITUD); // Quitar "d)"
        
        // Respuesta correcta
        token = strtok(NULL, "|");
        if (token != NULL) p.respuesta_correcta = toupper(token[0]);

        preguntas[total_preguntas++] = p; // Guardar la pregunta
    }

    fclose(file);
    printf("Preguntas cargadas con éxito: %d\n", total_preguntas);
    return total_preguntas;
}

// Función para manejar las conexiones de los clientes
void manejar_cliente(int nuevo_socket) {
    char buffer[1024] = {0};
    int valread;

    // Enviar preguntas al cliente
    for (int i = 0; i < total_preguntas; i++) {
        sprintf(buffer, "Categoría: %s\nNúmero: %s\nPregunta: %s\nA) %s\nB) %s\nC) %s\nD) %s\n\n",
                preguntas[i].categoria, preguntas[i].numero, preguntas[i].pregunta,
                preguntas[i].opcionA, preguntas[i].opcionB, preguntas[i].opcionC, preguntas[i].opcionD);
        
        send(nuevo_socket, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));
    }

    // Esperar respuesta del cliente y procesarla
    valread = read(nuevo_socket, buffer, 1024);
    if (valread > 0) {
        printf("Respuesta del cliente: %s\n", buffer);
    }

    close(nuevo_socket);
}

void load_users(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error abriendo el archivo de usuarios");
        exit(1);
    }

    while (fscanf(file, "%[^,],%s\n", users[user_count].matricula, users[user_count].password) == 2) {
        user_count++;
        if (user_count >= MAX_USUARIOS) break; // Limitar la carga de usuarios
    }
    fclose(file);
}

int authenticate(char *matricula, char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].matricula, matricula) == 0 && strcmp(users[i].password, password) == 0) {
            return 1; // autenticación exitosa
        }
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    int servidor_fd, nuevo_socket, opt = 1;
    struct sockaddr_in direccion;
    int addrlen = sizeof(direccion);
    load_users("usuarios.txt");
    // Cargar las preguntas
    total_preguntas = cargar_preguntas(preguntas);
    if (total_preguntas == 0) {
        printf("No se pudieron cargar preguntas. Terminando el servidor.\n");
        return -1;
    }

    // Crear socket del servidor
    if ((servidor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Opciones del socket
    if (setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Error al configurar el socket");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    direccion.sin_family = AF_INET;
    direccion.sin_addr.s_addr = INADDR_ANY;
    direccion.sin_port = htons(PORT);

    // Asignar dirección y puerto al socket
    if (bind(servidor_fd, (struct sockaddr *)&direccion, sizeof(direccion)) < 0) {
        perror("Error en el bind");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(servidor_fd, MAX_CLIENTES) < 0) {
        perror("Error en el listen");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor en espera de conexiones en el puerto %d\n", PORT);

    // Aceptar conexiones de los clientes
    while ((nuevo_socket = accept(servidor_fd, (struct sockaddr *)&direccion, (socklen_t*)&addrlen)) >= 0) {
        printf("Nueva conexión recibida\n");
        manejar_cliente(nuevo_socket);
    }

    if (nuevo_socket < 0) {
        perror("Error al aceptar la conexión");
    }

    close(servidor_fd);
    return 0;
}

