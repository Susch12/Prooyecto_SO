#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h> 

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

//Funcion para cargar usuarios 
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


//Funcion para filtrar por categorias
int filtrar_preguntas_por_categoria(Pregunta preguntas[], Pregunta preguntas_filtradas[], const char *categoria, int total_preguntas) {
    int index = 0;
    for (int i = 0; i < total_preguntas; i++) {
        if (strcmp(preguntas[i].categoria, categoria) == 0) {
            preguntas_filtradas[index++] = preguntas[i];
        }
    }
    return index; // Número de preguntas filtradas
}
// Función para autenticación

int authenticate(char *matricula, char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].matricula, matricula) == 0 && strcmp(users[i].password, password) == 0) {
            return 1; // autenticación exitosa
        }
    }
    return 0;
}

//Función para mezclar preguntas 
void mezclar_preguntas(Pregunta preguntas[], int n) {
    srand(time(NULL)); // Inicializar la semilla para aleatoriedad
    for (int i = 0; i < n; i++) {
        int j = rand() % n;
        Pregunta temp = preguntas[i];
        preguntas[i] = preguntas[j];
        preguntas[j] = temp;
    }
}

//Funcion para obtener las preguntas aleatorias
int obtener_preguntas_aleatorias(Pregunta preguntas_filtradas[], Pregunta preguntas_aleatorias[], int total_preguntas_filtradas) {
    // Asegurarnos de que no tratemos de obtener más preguntas de las que hay disponibles
    int num_preguntas = total_preguntas_filtradas < 10 ? total_preguntas_filtradas : 10;
    mezclar_preguntas(preguntas_filtradas, total_preguntas_filtradas);

    // Copiar las 10 primeras preguntas (o menos si no hay suficientes)
    for (int i = 0; i < num_preguntas; i++) {
        preguntas_aleatorias[i] = preguntas_filtradas[i];
    }
    return num_preguntas;
}

// Función para manejar las conexiones de los clientes
void manejar_cliente(int nuevo_socket) {
    char buffer[1024] = {0};
    int valread;
    char usuario[MAX_LONGITUD];
    char contrasena[MAX_LONGITUD];
    char categoria[MAX_LONGITUD];

    // Enviar mensaje solicitando el nombre de usuario
    send(nuevo_socket, "Por favor, ingrese su matrícula: ", 30, 0);

    // Leer el nombre de usuario del cliente
    valread = read(nuevo_socket, usuario, MAX_LONGITUD);
    if (valread <= 0) {
        printf("Error al recibir el nombre de usuario\n");
        close(nuevo_socket);
        return;
    }
    usuario[valread] = '\0';

    // Enviar mensaje solicitando la contraseña
    send(nuevo_socket, "Por favor, ingrese su contraseña: ", 32, 0);

    // Leer la contraseña del cliente
    valread = read(nuevo_socket, contrasena, MAX_LONGITUD);
    if (valread <= 0) {
        printf("Error al recibir la contraseña\n");
        close(nuevo_socket);
        return;
    }
    contrasena[valread] = '\0';

    // Verificar las credenciales
    if (authenticate(usuario, contrasena)) {
        send(nuevo_socket, "Autenticación exitosa. Iniciando el juego...\n", 43, 0);
    } else {
        send(nuevo_socket, "Autenticación fallida. Cerrando conexión...\n", 44, 0);
        close(nuevo_socket);
        return;
    }

    // Mostrar el menú de categorías
    send(nuevo_socket, "Elija una categoría:\n1) Historia\n2) Ciencia\n3) Geografía\n4) Literatura\n", 73, 0);
    
    // Leer la categoría seleccionada
    valread = read(nuevo_socket, categoria, MAX_LONGITUD);
    if (valread <= 0) {
        printf("Error al recibir la categoría\n");
        close(nuevo_socket);
        return;
    }
    categoria[valread] = '\0';

    // Filtrar las preguntas por categoría
    Pregunta preguntas_filtradas[MAX_PREGUNTAS];
    int total_preguntas_filtradas = filtrar_preguntas_por_categoria(preguntas, preguntas_filtradas, categoria, total_preguntas);

    if (total_preguntas_filtradas == 0) {
        send(nuevo_socket, "No hay preguntas disponibles para esta categoría.\n", 50, 0);
        close(nuevo_socket);
        return;
    }

    // Obtener 10 preguntas aleatorias de la categoría seleccionada
    Pregunta preguntas_aleatorias[10];
    int num_preguntas = obtener_preguntas_aleatorias(preguntas_filtradas, preguntas_aleatorias, total_preguntas_filtradas);

    // Enviar las preguntas aleatorias al cliente
    for (int i = 0; i < num_preguntas; i++) {
        sprintf(buffer, "Categoría: %s\nNúmero: %s\nPregunta: %s\nA) %s\nB) %s\nC) %s\nD) %s\n\n",
                preguntas_aleatorias[i].categoria, preguntas_aleatorias[i].numero, preguntas_aleatorias[i].pregunta,
                preguntas_aleatorias[i].opcionA, preguntas_aleatorias[i].opcionB, preguntas_aleatorias[i].opcionC, preguntas_aleatorias[i].opcionD);
        
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

