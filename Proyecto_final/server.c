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
#define MAX_USUARIOS 5  // Número máximo de usuarios
#define MAX_CLIENTES 10

typedef struct {
    char username[MAX_LONGITUD];
    char password[MAX_LONGITUD];
} Usuario;

Usuario usuarios[MAX_USUARIOS] = {
    {"202143818", "0wlsSD68"},
    {"202143819", "xYz1234"},
    // Puedes añadir más usuarios aquí
};

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

// Función para cargar preguntas desde un archivo
int cargar_preguntas(Pregunta preguntas[]) {
    // En este ejemplo, las preguntas se definen manualmente para simplificar
    strcpy(preguntas[0].categoria, "Matematicas");
    strcpy(preguntas[0].numero, "1");
    strcpy(preguntas[0].pregunta, "¿Cuánto es 2+2?");
    strcpy(preguntas[0].opcionA, "A) 3");
    strcpy(preguntas[0].opcionB, "B) 4");
    strcpy(preguntas[0].opcionC, "C) 5");
    strcpy(preguntas[0].opcionD, "D) 6");
    preguntas[0].respuesta_correcta = 'B';
    
    total_preguntas = 1;  // Solo hemos agregado una pregunta por simplicidad
    return total_preguntas;
}

// Función para autenticar usuario
int autenticar_usuario(int socket) {
    char buffer[MAX_LONGITUD];
    char username[MAX_LONGITUD], password[MAX_LONGITUD];

    // Solicitar nombre de usuario
    send(socket, "Ingrese su nombre de usuario: ", 30, 0);
    int valread = read(socket, username, sizeof(username));
    if (valread <= 0) return 0;

    // Eliminar salto de línea
    username[strcspn(username, "\n")] = 0;

    // Solicitar contraseña
    send(socket, "Ingrese su contraseña: ", 25, 0);
    valread = read(socket, password, sizeof(password));
    if (valread <= 0) return 0;

    // Eliminar salto de línea
    password[strcspn(password, "\n")] = 0;

    // Verificar credenciales
    for (int i = 0; i < MAX_USUARIOS; i++) {
        if (strcmp(username, usuarios[i].username) == 0 && strcmp(password, usuarios[i].password) == 0) {
            return 1; // Autenticado
        }
    }

    // Enviar mensaje de error si las credenciales son incorrectas
    send(socket, "Credenciales incorrectas. Conexión cerrada.\n", 45, 0);
    return 0; // No autenticado
}

// Función para manejar las conexiones de los clientes
void manejar_cliente(int nuevo_socket) {
    char buffer[1024] = {0};
    int valread;

    // Autenticar al usuario
    if (!autenticar_usuario(nuevo_socket)) {
        close(nuevo_socket);
        return;  // Terminar conexión si no es autenticado
    }

    // Enviar preguntas al cliente
    for (int i = 0; i < total_preguntas; i++) {
        sprintf(buffer, "Categoría: %s\nNúmero: %s\nPregunta: %s\nA) %s\nB) %s\nC) %s\nD) %s\n\n",
                preguntas[i].categoria, preguntas[i].numero, preguntas[i].pregunta,
                preguntas[i].opcionA, preguntas[i].opcionB, preguntas[i].opcionC, preguntas[i].opcionD);
        
        send(nuevo_socket, buffer, strlen(buffer), 0);
        memset(buffer, 0, sizeof(buffer));

        // Recibir respuesta del cliente
        valread = read(nuevo_socket, buffer, sizeof(buffer));
        if (valread <= 0) break;

        // Verificar respuesta
        if (buffer[0] == preguntas[i].respuesta_correcta) {
            send(nuevo_socket, "Respuesta correcta\n", 20, 0);
        } else {
            send(nuevo_socket, "Respuesta incorrecta\n", 22, 0);
        }
        memset(buffer, 0, sizeof(buffer));
    }

    // Finalizar la sesión del cliente
    send(nuevo_socket, "FIN", 3, 0);
    close(nuevo_socket);
}

int main() {
    int server_fd, nuevo_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Cargar preguntas
    cargar_preguntas(preguntas);

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket falló");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Enlace del socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind falló");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, MAX_CLIENTES) < 0) {
        perror("Escucha falló");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en puerto %d\n", PORT);

    // Aceptar y manejar conexiones de clientes
    while ((nuevo_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        printf("Conexión aceptada\n");
        manejar_cliente(nuevo_socket);
    }

    if (nuevo_socket < 0) {
        perror("Aceptación de cliente falló");
        exit(EXIT_FAILURE);
    }

    return 0;
}
