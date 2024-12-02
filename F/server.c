#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 8080
#define MAX_USERS 20
#define MAX_QUESTIONS 100
#define MAX_EXAMS 3

typedef struct {
    char matricula[20];
    char password[20];
    int score[MAX_EXAMS]; // Scores for each exam (Mathematics, Spanish, English)
} User;

typedef struct {
    char subject[20];
    char question[256];
    char options[3][100];
    int correct_option; // 0, 1, or 2 for the correct option index
} Question;

User users[MAX_USERS];
Question questions[MAX_QUESTIONS];
int user_count = 0, question_count = 0;
int users_completed = 0;

void load_users();
void load_questions();
void send_exam(int new_socket, int exam_type);
int check_answer(int exam_type, int question_index, int answer);

int main() {
    srand(time(NULL));  // Initialize random number generator
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    load_users();
    load_questions();

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

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // Handle client connection (authentication and exam selection)
        char buffer[1024] = {0};
        int user_index = -1;
        
        // Authenticate user (send matricula and password)
        recv(new_socket, buffer, sizeof(buffer), 0);
        for (int i = 0; i < user_count; i++) {
            if (strcmp(buffer, users[i].matricula) == 0) {
                recv(new_socket, buffer, sizeof(buffer), 0); // Receive password
                if (strcmp(buffer, users[i].password) == 0) {
                    user_index = i;
                    break;
                }
            }
        }

        if (user_index == -1) {
            send(new_socket, "Authentication failed", 22, 0);
            close(new_socket);
            continue;
        }
        
        // Send exam options (Mathematics, Spanish, English)
        char exam_menu[] = "Select exam: 1. Mathematics 2. Spanish 3. English";
        send(new_socket, exam_menu, sizeof(exam_menu), 0);
        
        int exam_choice;
        recv(new_socket, &exam_choice, sizeof(exam_choice), 0);
        send_exam(new_socket, exam_choice - 1); // Pass the selected exam type

        // After exam completion, check results
        users_completed++;
        if (users_completed >= 3) {
            printf("Displaying results for the first 3 users:\n");
            for (int i = 0; i < 3; i++) {
                printf("User %d: Matricula: %s, Scores: %d, %d, %d\n",
                    i + 1, users[i].matricula, users[i].score[0], users[i].score[1], users[i].score[2]);
            }
            users_completed = 0; // Reset for next group of 3 users
        }

        close(new_socket);
    }

    return 0;
}

void load_users() {
    FILE *file = fopen("users.bin", "r");
    if (!file) {
        perror("Could not open users.txt");
        exit(EXIT_FAILURE);
    }

    while (fscanf(file, "%s %s", users[user_count].matricula, users[user_count].password) != EOF) {
        user_count++;
    }
    fclose(file);
}

void load_questions() {
    FILE *file = fopen("questions.bin", "r");
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

void send_exam(int new_socket, int exam_type) {
    // Select 10 random questions for the exam
    int selected_questions[10];
    for (int i = 0; i < 10; i++) {
        selected_questions[i] = rand() % question_count;
    }

    for (int i = 0; i < 10; i++) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "Q%d: %s\n1. %s\n2. %s\n3. %s\n",
                 i + 1, questions[selected_questions[i]].question,
                 questions[selected_questions[i]].options[0],
                 questions[selected_questions[i]].options[1],
                 questions[selected_questions[i]].options[2]);
        send(new_socket, buffer, sizeof(buffer), 0);

        int answer;
        recv(new_socket, &answer, sizeof(answer), 0);

        if (check_answer(exam_type, selected_questions[i], answer)) {
            users[exam_type].score[exam_type]++;
        }
    }

    char result_message[100];
    snprintf(result_message, sizeof(result_message), "Your score is: %d", users[exam_type].score[exam_type]);
    send(new_socket, result_message, sizeof(result_message), 0);
}

int check_answer(int exam_type, int question_index, int answer) {
    if (questions[question_index].correct_option == answer) {
        return 1; // Correct answer
    }
    return 0; // Incorrect answer
}
