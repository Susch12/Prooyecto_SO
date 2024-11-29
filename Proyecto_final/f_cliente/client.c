#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <gtk/gtk.h>

#define PORT 8080
#define MAX_LONGITUD 100

typedef struct {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *label_pregunta;
    GtkWidget *button_Matematicas;
    GtkWidget *button_Espanol;
    GtkWidget *button_Ingles;
    GtkWidget *label_resultado;
    int socket;
} AppData;

int conectar_con_servidor(const char *direccion_ip) {
    int sock;
    struct sockaddr_in servidor;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("No se pudo crear el socket");
        exit(EXIT_FAILURE);
    }

    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PORT);
    servidor.sin_addr.s_addr = inet_addr(direccion_ip);

    if (connect(sock, (struct sockaddr *)&servidor, sizeof(servidor)) < 0) {
        perror("Conexión fallida");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

void seleccionar_examen(GtkWidget *widget, gpointer data) {
    AppData *app = (AppData *)data;
    const char *label = gtk_button_get_label(GTK_BUTTON(widget));
    send(app->socket, label, strlen(label), 0);

    // Después de enviar la selección del examen, recibir preguntas
    char buffer[MAX_LONGITUD];
    int valread;

    // Bucle para recibir preguntas una por una hasta el puntaje final
    while ((valread = recv(app->socket, buffer, sizeof(buffer), 0)) > 0) {
        buffer[valread] = '\0';  // Asegurar que el buffer esté terminado en null
        if (strstr(buffer, "Su puntaje es") != NULL) {
            // Mostrar el puntaje en la etiqueta de resultados
            gtk_label_set_text(GTK_LABEL(app->label_resultado), buffer);
            break;
        } else {
            // Mostrar cada pregunta en la etiqueta de preguntas
            gtk_label_set_text(GTK_LABEL(app->label_pregunta), buffer);
        }
        memset(buffer, 0, sizeof(buffer));
    }
}

void iniciar_examen(AppData *app) {
    app->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(app->window), app->vbox);

    app->label_pregunta = gtk_label_new("Seleccione un examen:");
    gtk_box_pack_start(GTK_BOX(app->vbox), app->label_pregunta, FALSE, FALSE, 0);

    app->button_Matematicas = gtk_button_new_with_label("Matemáticas");
    gtk_box_pack_start(GTK_BOX(app->vbox), app->button_Matematicas, FALSE, FALSE, 0);
    g_signal_connect(app->button_Matematicas, "clicked", G_CALLBACK(seleccionar_examen), app);

    app->button_Espanol = gtk_button_new_with_label("Español");
    gtk_box_pack_start(GTK_BOX(app->vbox), app->button_Espanol, FALSE, FALSE, 0);
    g_signal_connect(app->button_Espanol, "clicked", G_CALLBACK(seleccionar_examen), app);

    app->button_Ingles = gtk_button_new_with_label("Inglés");
    gtk_box_pack_start(GTK_BOX(app->vbox), app->button_Ingles, FALSE, FALSE, 0);
    g_signal_connect(app->button_Ingles, "clicked", G_CALLBACK(seleccionar_examen), app);

    app->label_resultado = gtk_label_new("Resultados aquí");
    gtk_box_pack_start(GTK_BOX(app->vbox), app->label_resultado, FALSE, FALSE, 0);

    gtk_widget_show_all(app->window);
}

static void activar(GtkApplication *app, gpointer user_data) {
    AppData *app_data = (AppData *)user_data;
    app_data->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(app_data->window), "Examen en línea");
    gtk_window_set_default_size(GTK_WINDOW(app_data->window), 400, 300);
    iniciar_examen(app_data);
}

void cerrar_cliente(AppData *app) {
    close(app->socket);
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <IP_DEL_SERVIDOR>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    AppData app_data;
    app_data.socket = conectar_con_servidor(argv[1]);

    GtkApplication *app = gtk_application_new("com.example.ExamenOnline", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activar), &app_data);
    g_signal_connect(app, "shutdown", G_CALLBACK(cerrar_cliente), &app_data);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

