#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_USERS 20
#define MAX_EXAMS 3
#define MAX_QUESTIONS 10
#define TOTAL_QUESTIONS 60
#define PORT 8080
#define WORKER_THREADS 5
#define MAX_CLIENT_QUEUE 100
#define MAX_WORKERS 5

typedef struct {
    char matricula[10];
    char password[10];
} User;

typedef struct {
    char question[256];
    char options[3][256];
    int correct_option;
} Question;

typedef struct {
    Question questions[MAX_QUESTIONS];
    int question_count;
} Exam;

typedef struct {
    char matricula[10];
    int scores[MAX_EXAMS];
} Result;

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} ClientInfo;


typedef struct {
    int clients[MAX_CLIENT_QUEUE];
    int front, rear, size;
} ClientQueue;

ClientQueue client_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
int shutdown_flag = 0;

pthread_mutex_t client_queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t client_queue_cond = PTHREAD_COND_INITIALIZER;

ClientInfo *client_queue[100];
int client_queue_size = 0;

pthread_t worker_threads[WORKER_THREADS];

// Declaraciones de funciones
void signal_handler(int sig);
void *worker_thread_func(void *arg);
void enqueue_client(ClientInfo *client);
ClientInfo *dequeue_client();
void *handle_client(void *client_info);

User users[MAX_USERS];
Exam exams[MAX_EXAMS];
Result results[MAX_USERS];
int result_count = 0;
Question question_bank[TOTAL_QUESTIONS];

void load_users();
int authenticate(char *matricula, char *password);
void load_exam(const char *filename, Exam *exam);
void load_exams();
void generate_random_exam(Exam *exam);
void add_result(const char *matricula, int scores[]);
void show_results();
void *handle_client(void *client_socket);

volatile sig_atomic_t server_shutdown = 0;

void initialize_client_queue() {
    pthread_mutex_lock(&queue_mutex);
    client_queue.front = 0;
    client_queue.rear = -1;
    client_queue.size = 0;
    pthread_mutex_unlock(&queue_mutex);
}


void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nServidor detenido por señal %d.\n", sig);
        exit(0);
    } else if (sig == SIGPIPE) {
        printf("Conexión rota detectada. SIGPIPE ignorado.\n");
    }
}

void load_users() {
    FILE *file = fopen("usuarios.txt", "r");
    if (file == NULL) {
        perror("Error al abrir el archivo de usuarios");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), file) && i < MAX_USERS) {
        char *token = strtok(line, ",");
        if (token != NULL) {
            strncpy(users[i].matricula, token, sizeof(users[i].matricula) - 1);
            token = strtok(NULL, "\n");
            if (token != NULL) {
                strncpy(users[i].password, token, sizeof(users[i].password) - 1);
            }
        }
        i++;
    }

    fclose(file);
}

int authenticate(char *matricula, char *password) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(users[i].matricula, matricula) == 0 && strcmp(users[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

void load_exam(const char *filename, Exam *exam) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo de preguntas");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int i = 0;
    while (fgets(line, sizeof(line), file) && i < MAX_QUESTIONS) {
        line[strcspn(line, "\n")] = 0;

        char *token = strtok(line, "|");
        if (token == NULL) continue;

        token = strtok(NULL, "|");
        if (token != NULL) {
            strncpy(exam->questions[i].question, token, sizeof(exam->questions[i].question) - 1);
            exam->questions[i].question[sizeof(exam->questions[i].question) - 1] = '\0';
        }

        for (int j = 0; j < 3; j++) {
            token = strtok(NULL, "|");
            if (token != NULL) {
                strncpy(exam->questions[i].options[j], token, sizeof(exam->questions[i].options[j]) - 1);
                exam->questions[i].options[j][sizeof(exam->questions[i].options[j]) - 1] = '\0';
            }
        }

        token = strtok(NULL, "|");
        if (token != NULL) {
            exam->questions[i].correct_option = token[0] - 'a' + 1;
        }

        i++;
    }
    exam->question_count = i;

    fclose(file);
}

void load_exams() {
    load_exam("mate.txt", &exams[0]);
    load_exam("español.txt", &exams[1]);
    load_exam("ingles.txt", &exams[2]);
}

void generate_random_exam(Exam *exam) {
    int indices[TOTAL_QUESTIONS];
    for (int i = 0; i < TOTAL_QUESTIONS; i++) {
        indices[i] = i;
    }

    for (int i = 0; i < TOTAL_QUESTIONS; i++) {
        int j = rand() % TOTAL_QUESTIONS;
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    for (int i = 0; i < MAX_QUESTIONS; i++) {
        exam->questions[i] = question_bank[indices[i]];
    }
    exam->question_count = MAX_QUESTIONS;
}

void add_result(const char *matricula, int scores[]) {
    for (int i = 0; i < result_count; i++) {
        if (strcmp(results[i].matricula, matricula) == 0) {
            for (int j = 0; j < MAX_EXAMS; j++) {
                results[i].scores[j] = scores[j];
            }
            return;
        }
    }

    if (result_count < MAX_USERS) {
        strncpy(results[result_count].matricula, matricula, sizeof(results[result_count].matricula) - 1);
        for (int i = 0; i < MAX_EXAMS; i++) {
            results[result_count].scores[i] = scores[i];
        }
        result_count++;
    }

    if (result_count == 3) {
        show_results();
    }
}

void show_results() {
    printf("Resultados de los 3 usuarios:\n");
    for (int i = 0; i < 3; i++) {
        printf("Usuario %s:\n", results[i].matricula);
        for (int j = 0; j < MAX_EXAMS; j++) {
            printf("Examen %d: %d\n", j + 1, results[i].scores[j]);
        }
    }
}

void *handle_client(void *client_info) {
    ClientInfo *info = (ClientInfo *)client_info;
    int socket = info->client_socket;
    free(info);

    char buffer[1024];
    int n;

    n = read(socket, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("Error leyendo del socket");
        close(socket);
        return NULL;
    }

    buffer[n] = '\0';
    char *matricula = strtok(buffer, ",");
    char *password = strtok(NULL, ",");

    if (authenticate(matricula, password)) {
        write(socket, "Autenticación exitosa\n", 22);
        int scores[MAX_EXAMS] = {0};
        for (int i = 0; i < MAX_EXAMS; i++) {
            n = read(socket, buffer, sizeof(buffer) - 1);
            if (n < 0) {
                perror("Error leyendo del socket");
                close(socket);
                return NULL;
            }
            buffer[n] = '\0';

            Exam *exam = NULL;
            if (strcmp(buffer, "Matemáticas") == 0) {
                exam = &exams[0];
            } else if (strcmp(buffer, "Español") == 0) {
                exam = &exams[1];
            } else if (strcmp(buffer, "Inglés") == 0) {
                exam = &exams[2];
            }

            if (exam != NULL) {
                int score = 0;
                for (int j = 0; j < exam->question_count; j++) {
                    snprintf(buffer, sizeof(buffer), "Pregunta %d: %s\nA) %s\nB) %s\nC) %s\n",
                             j + 1, exam->questions[j].question,
                             exam->questions[j].options[0], exam->questions[j].options[1],
                             exam->questions[j].options[2]);

                    write(socket, buffer, strlen(buffer));
                    n = read(socket, buffer, sizeof(buffer) - 1);
                    if (n < 0) {
                        perror("Error leyendo la respuesta");
                        close(socket);
                        return NULL;
                    }

                    buffer[n] = '\0';
                    int respuesta = atoi(buffer);
                    if (respuesta == exam->questions[j].correct_option) {
                        score++;
                    }
                }

                scores[i] = score;
            }
        }
        add_result(matricula, scores);
    } else {
        write(socket, "Autenticación fallida\n", 22);
    }

    close(socket);
    return NULL;
}

void *worker_thread_func(void *arg) {
    while (1) {
        pthread_mutex_lock(&client_queue_lock);
        while (client_queue_size == 0) {
            pthread_cond_wait(&client_queue_cond, &client_queue_lock);
        }

        ClientInfo *client = dequeue_client();
        pthread_mutex_unlock(&client_queue_lock);

        if (client != NULL) {
            handle_client(client);
        }
    }
    return NULL;
}

void enqueue_client(ClientInfo *client) {
    pthread_mutex_lock(&client_queue_lock);
    client_queue[client_queue_size++] = client;
    pthread_cond_signal(&client_queue_cond);
    pthread_mutex_unlock(&client_queue_lock);
}

ClientInfo *dequeue_client() {
    if (client_queue_size == 0) {
        return NULL;
    }
    ClientInfo *client = client_queue[0];
    for (int i = 1; i < client_queue_size; i++) {
        client_queue[i - 1] = client_queue[i];
    }
    client_queue_size--;
    return client;
}


int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    // Inicializar estructuras de señal
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    // Inicializar cola de clientes
    initialize_client_queue();

    // Crear worker threads
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (pthread_create(&worker_threads[i], NULL, worker_thread_func, NULL) != 0) {
            perror("Error al crear un worker thread");
            exit(EXIT_FAILURE);
        }
    }

    // Configurar socket del servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error al abrir el socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en el bind");
        close(server_socket);
        exit(EXIT_FAIL;
    }

    if (listen(server_socket, 5) < 0) {
        perror("Error en listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Servidor escuchando en el puerto %d\n", PORT);

    // Aceptar clientes
    while (!server_shutdown) {
        int client_socket;
        client_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);

        if (client_socket < 0) {
            if (server_shutdown) break; // Salir del bucle si el servidor se está cerrando
            perror("Error al aceptar cliente");
            continue;
        }

        // Agregar cliente a la cola
        enqueue_client(client_socket);
    }

    printf("\nCerrando el servidor...\n");

    // Notificar a los worker threads para que terminen
    pthread_mutex_lock(&queue_mutex);
    shutdown_flag = 1;
    pthread_cond_broadcast(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);

    // Esperar a que los worker threads terminen
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    close(server_socket);
    printf("Servidor cerrado correctamente.\n");

    return 0;
}
