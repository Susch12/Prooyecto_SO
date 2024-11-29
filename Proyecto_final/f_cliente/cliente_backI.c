#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_LONGITUD 256

// Estructura global para los elementos de la interfaz
typedef struct {
    GtkWidget *window;
    GtkWidget *entry_matricula;
    GtkWidget *entry_password;
    GtkWidget *combo_exam;
    GtkWidget *label_resultado;
    int socket;
} AppData;

// Función para enviar y recibir datos del servidor
void enviar_al_servidor(AppData *app, const char *mensaje, char *respuesta) {
    send(app->socket, mensaje, strlen(mensaje), 0);
    read(app->socket, respuesta, 1024);
}

// Nueva función para abrir la ventana del examen
void abrir_ventana_examen(AppData *app, const char *examen) {
    GtkWidget *exam_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(exam_window), examen);
    gtk_window_set_default_size(GTK_WINDOW(exam_window), 400, 300);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(exam_window), vbox);

    char buffer[1024] = {0};
    enviar_al_servidor(app, examen, buffer);

    // Leer y mostrar preguntas una por una
    for (int i = 0; i < 10; i++) {
        memset(buffer, 0, sizeof(buffer));
        read(app->socket, buffer, 1024);

        GtkWidget *question_label = gtk_label_new(buffer);
        gtk_box_pack_start(GTK_BOX(vbox), question_label, FALSE, FALSE, 0);

        // Crear botones de respuesta
        GtkWidget *answer_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_box_pack_start(GTK_BOX(vbox), answer_box, FALSE, FALSE, 0);

        for (int j = 0; j < 3; j++) {
            char option[1024];
            snprintf(option, sizeof(option), "Opción %d", j + 1);
            GtkWidget *answer_button = gtk_button_new_with_label(option);
            g_signal_connect_swapped(answer_button, "clicked", G_CALLBACK(enviar_al_servidor), app);
            gtk_box_pack_start(GTK_BOX(answer_box), answer_button, TRUE, TRUE, 0);
        }
    }

    // Mostrar calificación final
    read(app->socket, buffer, 1024);
    GtkWidget *final_grade_label = gtk_label_new(buffer);
    gtk_box_pack_start(GTK_BOX(vbox), final_grade_label, FALSE, FALSE, 0);

    gtk_widget_show_all(exam_window);
}

// Función de autenticación
void autenticar(GtkWidget *widget, AppData *app) {
    char matricula[MAX_LONGITUD], password[MAX_LONGITUD];
    strcpy(matricula, gtk_entry_get_text(GTK_ENTRY(app->entry_matricula)));
    strcpy(password, gtk_entry_get_text(GTK_ENTRY(app->entry_password)));

    // Enviar matrícula y contraseña al servidor
    char buffer[1024] = {0};
    enviar_al_servidor(app, matricula, buffer);
    enviar_al_servidor(app, password, buffer);

    // Imprimir el buffer para depuración
    printf("Respuesta del servidor: %s\n", buffer);

    // Mostrar autenticación exitosa o fallida
    if (strstr(buffer, "exitosa")) {
        gtk_label_set_text(GTK_LABEL(app->label_resultado), "Autenticación exitosa. Selecciona un examen.");
    } else {
        gtk_label_set_text(GTK_LABEL(app->label_resultado), "Autenticación fallida.");
    }
}
// Función para iniciar el examen seleccionado
void iniciar_examen(GtkWidget *widget, AppData *app) {
    const char *examen = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->combo_exam));

    if (!examen) {
        gtk_label_set_text(GTK_LABEL(app->label_resultado), "Seleccione un examen.");
        return;
    }

    abrir_ventana_examen(app, examen);
}

// Configuración de la interfaz gráfica
void activar_interfaz(GtkApplication *app, gpointer user_data) {
    AppData *data = (AppData *)user_data;
    
    data->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(data->window), "Exámenes en Línea");
    gtk_window_set_default_size(GTK_WINDOW(data->window), 300, 200);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(data->window), grid);

    GtkWidget *label_matricula = gtk_label_new("Matrícula:");
    data->entry_matricula = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), label_matricula, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), data->entry_matricula, 1, 0, 2, 1);

    GtkWidget *label_password = gtk_label_new("Contraseña:");
    data->entry_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(data->entry_password), FALSE);
    gtk_grid_attach(GTK_GRID(grid), label_password, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), data->entry_password, 1, 1, 2, 1);

    GtkWidget *button_login = gtk_button_new_with_label("Autenticar");
    g_signal_connect(button_login, "clicked", G_CALLBACK(autenticar), data);
    gtk_grid_attach(GTK_GRID(grid), button_login, 1, 2, 1, 1);

    GtkWidget *label_exam = gtk_label_new("Selecciona un examen:");
    data->combo_exam = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->combo_exam), "Matemáticas");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->combo_exam), "Español");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(data->combo_exam), "Inglés");
    gtk_grid_attach(GTK_GRID(grid), label_exam, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), data->combo_exam, 1, 3, 2, 1);

    GtkWidget *button_exam = gtk_button_new_with_label("Iniciar Examen");
    g_signal_connect(button_exam, "clicked", G_CALLBACK(iniciar_examen), data);
    gtk_grid_attach(GTK_GRID(grid), button_exam, 1, 4, 1, 1);

    data->label_resultado = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), data->label_resultado, 0, 5, 3, 1);

    gtk_widget_show_all(data->window);
}

int main(int argc, char **argv) {
    AppData app_data = {0};

    // Crear socket y conectar al servidor
    if ((app_data.socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear socket");
        return -1;
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0 || connect(app_data.socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error de conexión");
        return -1;
    }

    GtkApplication *app = gtk_application_new("org.gtk.examenes", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activar_interfaz), &app_data);
    int status = g_application_run(G_APPLICATION(app), argc, argv);

    close(app_data.socket);
    g_object_unref(app);

    return status;
}

