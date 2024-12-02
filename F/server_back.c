#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>

#define PORT 8080
#define MAX_USERS 20
#define MAX_QUESTIONS 100

typedef struct {
    char question[256];
    char options[3][100];
    int correct_option;
    int used; // Indicates if the question has been used
} Question;

typedef struct {
    int id;
    int completed_exams;
    char matricula[20];
} User;

// Banco de preguntas para cada materia
Question math_questions[MAX_QUESTIONS], spanish_questions[MAX_QUESTIONS], english_questions[MAX_QUESTIONS];
int math_count = 0, spanish_count = 0, english_count = 0;

// Funciones
void load_questions(const char *filename, Question questions[], int *question_count);
void generate_unique_exam(Question questions[], int question_count, Question exam[], int num_questions);
void handle_client(int client_socket);
void send_exam(int client_socket, const char *subject);

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Cargar preguntas
    load_questions("mate.bin", math_questions, &math_count);
    load_questions("español.bin", spanish_questions, &spanish_count);
    load_questions("ingles.bin", english_questions, &english_count);

    printf("Preguntas cargadas:\nMatemáticas: %d\nEspañol: %d\nInglés: %d\n", math_count, spanish_count, english_count);

    // Configuración del servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        printf("Nuevo cliente conectado.\n");
        handle_client(new_socket);
        close(new_socket);
    }

    return 0;
}

void load_questions(const char *filename, Question questions[], int *question_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("No se pudo abrir el archivo de preguntas");
        exit(EXIT_FAILURE);
    }

    while (fscanf(file, "%[^|]|%[^|]|%[^|]|%[^|]|%d\n",
                  questions[*question_count].question,
                  questions[*question_count].options[0],
                  questions[*question_count].options[1],
                  questions[*question_count].options[2],
                  &questions[*question_count].correct_option) != EOF) {
        questions[*question_count].used = 0; // Inicializar como no usado
        (*question_count)++;
    }

    fclose(file);
}

void generate_unique_exam(Question questions[], int question_count, Question exam[], int num_questions) {
    int selected_indices[num_questions];
    int selected_count = 0;

    while (selected_count < num_questions) {
        int random_index = rand() % question_count;

        // Si la pregunta no ha sido usada, seleccionarla
        if (!questions[random_index].used) {
            questions[random_index].used = 1;
            exam[selected_count] = questions[random_index];
            selected_count++;
        }
    }
}

void handle_client(int client_socket) {
    char buffer[1024];
    char subject[20];

    // Recibir la materia seleccionada por el cliente
    recv(client_socket, subject, sizeof(subject), 0);

    printf("Materia seleccionada: %s\n", subject);

    // Enviar el examen basado en la materia seleccionada
    send_exam(client_socket, subject);
}

void send_exam(int client_socket, const char *subject) {
    Question exam[10];
    if (strcmp(subject, "Matematicas") == 0) {
        generate_unique_exam(math_questions, math_count, exam, 10);
    } else if (strcmp(subject, "Español") == 0) {
        generate_unique_exam(spanish_questions, spanish_count, exam, 10);
    } else if (strcmp(subject, "Ingles") == 0) {
        generate_unique_exam(english_questions, english_count, exam, 10);
    } else {
        strcpy(exam[0].question, "Materia no válida.");
        send(client_socket, &exam, sizeof(exam), 0);
        return;
    }

    // Enviar las preguntas al cliente
    send(client_socket, exam, sizeof(exam), 0);
    printf("Examen enviado al cliente.\n");
}
