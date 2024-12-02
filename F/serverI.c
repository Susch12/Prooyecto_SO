#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_USERS 20
#define MAX_QUESTIONS 100
#define MAX_ANSWER_OPTIONS 3

typedef struct {
    char matricula[20];
    char password[20];
} User;

typedef struct {
    char subject[20];
    char question[256];
    char options[MAX_ANSWER_OPTIONS][100];
    int correct_option;
} Question;

User users[MAX_USERS];
int user_count = 0;

Question questions[MAX_QUESTIONS];
int question_count = 0;

// Función para cargar usuarios desde archivo
void load_users() {
    FILE *file = fopen("users.txt", "r");
    if (!file) {
        perror("Could not open users.txt");
        exit(EXIT_FAILURE);
    }
    while (fscanf(file, "%s %s", users[user_count].matricula, users[user_count].password) != EOF) {
        user_count++;
    }
    fclose(file);
}

// Función para cargar preguntas desde archivo
void load_questions() {
    FILE *file = fopen("questions.txt", "r");
    if (!file) {
        perror("Could not open questions.txt");
        exit(EXIT_FAILURE);
    }
    while (fscanf(file, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d",
                  questions[question_count].subject,
                  questions[question_count].question,
                  questions[question_count].options[0],
                  questions[question_count].options[1],
                  questions[question_count].options[2],
                  &questions[question_count].correct_option) != EOF) {
        question_count++;
    }
    fclose(file);
}

// Función para validar las credenciales del usuario
int validate_user(const char *matricula, const char *password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].matricula, matricula) == 0 && strcmp(users[i].password, password) == 0) {
            return 1; // Usuario válido
        }
    }
    return 0; // Usuario no válido
}

// Función para asignar preguntas aleatorias a los usuarios
void assign_exam_questions(Question *assigned_questions) {
    srand(time(NULL)); // Inicializa la semilla de números aleatorios

    int indices[MAX_QUESTIONS];
    for (int i = 0; i < 10; i++) {
        int rand_index;
        do {
            rand_index = rand() % question_count;
        } while (indices[rand_index] != 0); // Evita la repetición de preguntas

        indices[rand_index] = 1; // Marca esta pregunta como asignada
        assigned_questions[i] = questions[rand_index]; // Asigna la pregunta al usuario
    }
}

// Función para calcular la calificación
int grade_exam(int answers[MAX_QUESTIONS], Question *assigned_questions) {
    int score = 0;
    for (int i = 0; i < 10; i++) {
        if (answers[i] == assigned_questions[i].correct_option) {
            score++;
        }
    }
    return score;
}

// Función principal que maneja la conexión con el cliente
void handle_client(int client_socket) {
    char buffer[1024];
    char matricula[20], password[20];
    int valid_user = 0;
    int exam_answers[10];
    Question assigned_questions[10];

    // Recibir los datos de inicio de sesión
    recv(client_socket, buffer, sizeof(buffer), 0);
    sscanf(buffer, "LOGIN %s %s", matricula, password);

    // Validar las credenciales
    valid_user = validate_user(matricula, password);
    if (valid_user) {
        send(client_socket, "LOGIN SUCCESS", 13, 0);

        // Asignar preguntas aleatorias
        assign_exam_questions(assigned_questions);

        // Enviar las preguntas al cliente
        for (int i = 0; i < 10; i++) {
            snprintf(buffer, sizeof(buffer), "%s|%s|%s|%s|%s",
                     assigned_questions[i].subject,
                     assigned_questions[i].question,
                     assigned_questions[i].options[0],
                     assigned_questions[i].options[1],
                     assigned_questions[i].options[2]);
            send(client_socket, buffer, strlen(buffer), 0);
        }

        // Recibir las respuestas del cliente
        recv(client_socket, buffer, sizeof(buffer), 0);
        // Parsear las respuestas (debe ser un array de 10 enteros)
        sscanf(buffer, "%d %d %d %d %d %d %d %d %d %d", 
               &exam_answers[0], &exam_answers[1], &exam_answers[2], &exam_answers[3], 
               &exam_answers[4], &exam_answers[5], &exam_answers[6], &exam_answers[7], 
               &exam_answers[8], &exam_answers[9]);

        // Calificar el examen
        int score = grade_exam(exam_answers, assigned_questions);

        // Enviar la calificación al cliente
        snprintf(buffer, sizeof(buffer), "Your score: %d/10", score);
        send(client_socket, buffer, strlen(buffer), 0);
    } else {
        send(client_socket, "LOGIN FAILED", 12, 0);
    }

    close(client_socket); // Cerrar la conexión con el cliente
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    // Cargar los usuarios y las preguntas
    load_users();
    load_questions();

    // Crear socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Manejar el cliente
        handle_client(client_socket);
    }

    return 0;
}
