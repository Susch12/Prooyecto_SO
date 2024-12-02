#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define MAX_QUESTIONS 10
#define PORT 12345

GtkWidget *window, *username_entry, *password_entry, *message_label, *exam_window;
GtkWidget *exam_box, *question_label, *option1, *option2, *option3;
GtkWidget *next_button;
int socket_desc;
struct sockaddr_in server_addr;

int current_question = 0;
int selected_answer = -1;
int current_exam = 0;

// Declaraciones de funciones
void send_login_data();
void on_login_button_clicked(GtkWidget *widget, gpointer data);
void connect_to_server();
void on_exam_button_clicked(GtkWidget *widget, gpointer data);
void create_exam_window();
void start_exam();
void load_question();
void on_next_question_clicked(GtkWidget *widget, gpointer data);

// Función para enviar los datos de inicio de sesión
void send_login_data() {
    const gchar *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

    char message[256];
    snprintf(message, sizeof(message), "%s %s", username, password);
    send(socket_desc, message, strlen(message), 0);
}

// Función para manejar el clic del botón de inicio de sesión
void on_login_button_clicked(GtkWidget *widget, gpointer data) {
    send_login_data();

    char buffer[1024];
    recv(socket_desc, buffer, sizeof(buffer), 0);
    
    if (strcmp(buffer, "Autenticación exitosa") == 0) {
        gtk_label_set_text(GTK_LABEL(message_label), "Autenticación exitosa");
        // Ocultar la ventana de login y mostrar el menú de exámenes
        gtk_widget_hide(window);
        create_exam_window();
    } else {
        gtk_label_set_text(GTK_LABEL(message_label), "Autenticación fallida");
    }
}

// Función para conectar al servidor
void connect_to_server() {
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("No se pudo crear el socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Conexión fallida");
        exit(1);
    }

    printf("Conectado al servidor\n");
}

// Función para manejar el clic en un botón de examen
void on_exam_button_clicked(GtkWidget *widget, gpointer data) {
    // Obtén el índice del examen seleccionado (Matemáticas, Español, Inglés)
    current_exam = GPOINTER_TO_INT(data);
    gtk_widget_hide(exam_window);
    start_exam();
}

// Función para crear la ventana de selección de examen
void create_exam_window() {
    // Crear la ventana del examen
    exam_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(exam_window), "Seleccionar Examen");
    gtk_window_set_default_size(GTK_WINDOW(exam_window), 300, 200);
    g_signal_connect(exam_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *exam_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(exam_window), exam_box);

    GtkWidget *math_button = gtk_button_new_with_label("Matemáticas");
    g_signal_connect(math_button, "clicked", G_CALLBACK(on_exam_button_clicked), GINT_TO_POINTER(0));
    gtk_box_pack_start(GTK_BOX(exam_box), math_button, FALSE, FALSE, 0);

    GtkWidget *spanish_button = gtk_button_new_with_label("Español");
    g_signal_connect(spanish_button, "clicked", G_CALLBACK(on_exam_button_clicked), GINT_TO_POINTER(1));
    gtk_box_pack_start(GTK_BOX(exam_box), spanish_button, FALSE, FALSE, 0);

    GtkWidget *english_button = gtk_button_new_with_label("Inglés");
    g_signal_connect(english_button, "clicked", G_CALLBACK(on_exam_button_clicked), GINT_TO_POINTER(2));
    gtk_box_pack_start(GTK_BOX(exam_box), english_button, FALSE, FALSE, 0);

    gtk_widget_show_all(exam_window);
}

// Función para iniciar el examen
void start_exam() {
    // Crear ventana para la primera pregunta
    GtkWidget *exam_frame = gtk_frame_new("Examen");
    gtk_container_add(GTK_CONTAINER(exam_window), exam_frame);

    exam_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(exam_frame), exam_box);

    question_label = gtk_label_new("Pregunta aquí");
    gtk_box_pack_start(GTK_BOX(exam_box), question_label, FALSE, FALSE, 0);

    // Opciones de respuesta
    option1 = gtk_radio_button_new_with_label(NULL, "Opción 1");
    gtk_box_pack_start(GTK_BOX(exam_box), option1, FALSE, FALSE, 0);

    option2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(option1), "Opción 2");
    gtk_box_pack_start(GTK_BOX(exam_box), option2, FALSE, FALSE, 0);

    option3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(option1), "Opción 3");
    gtk_box_pack_start(GTK_BOX(exam_box), option3, FALSE, FALSE, 0);

    next_button = gtk_button_new_with_label("Siguiente");
    g_signal_connect(next_button, "clicked", G_CALLBACK(on_next_question_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(exam_box), next_button, FALSE, FALSE, 0);

    gtk_widget_show_all(exam_window);
    load_question();
}

// Función para cargar una pregunta
void load_question() {
    char question[256] = "Pregunta de ejemplo";
    char option_1[100] = "Respuesta 1";
    char option_2[100] = "Respuesta 2";
    char option_3[100] = "Respuesta 3";

    // Aquí deberíamos recibir la pregunta y las opciones del servidor
    snprintf(question, sizeof(question), "Pregunta %d del Examen", current_question + 1);
    snprintf(option_1, sizeof(option_1), "Opción 1");
    snprintf(option_2, sizeof(option_2), "Opción 2");
    snprintf(option_3, sizeof(option_3), "Opción 3");

    gtk_label_set_text(GTK_LABEL(question_label), question);
    gtk_button_set_label(GTK_BUTTON(option1), option_1);
    gtk_button_set_label(GTK_BUTTON(option2), option_2);
    gtk_button_set_label(GTK_BUTTON(option3), option_3);
}

// Función para manejar el clic en el botón de siguiente pregunta
void on_next_question_clicked(GtkWidget *widget, gpointer data) {
    // Enviar respuesta seleccionada al servidor
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(option1))) {
        selected_answer = 0;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(option2))) {
        selected_answer = 1;
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(option3))) {
        selected_answer = 2;
    }

    if (selected_answer == -1) {
        printf("Selecciona una respuesta antes de continuar.\n");
        return;
    }

    // Enviar la respuesta al servidor
    send(socket_desc, &selected_answer, sizeof(selected_answer), 0);

    // Incrementar la pregunta y cargar la siguiente
    current_question++;
    if (current_question < MAX_QUESTIONS) {
        load_question();
    } else {
        // Fin del examen
        gtk_label_set_text(GTK_LABEL(question_label), "Examen terminado. Gracias.");
    }
}

// Función principal
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Login de Exámenes en Línea");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Matrícula");
    gtk_box_pack_start(GTK_BOX(box), username_entry, FALSE, FALSE, 0);

    password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Contraseña");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(box), password_entry, FALSE, FALSE, 0);

    GtkWidget *login_button = gtk_button_new_with_label("Iniciar sesión");
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(box), login_button, FALSE, FALSE, 0);

    message_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(box), message_label, FALSE, FALSE, 0);

    gtk_widget_show_all(window);

    connect_to_server();

    gtk_main();

    return 0;
}

