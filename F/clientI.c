#include <gtk/gtk.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"

// Estructura para pasar datos entre hilos
typedef struct {
    GtkWidget *login_window;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *status_label;
    int socket;
} LoginData;

// Función para autenticar el usuario en el servidor
void *authenticate_user(void *data) {
    LoginData *login_data = (LoginData *)data;
    const gchar *username = gtk_entry_get_text(GTK_ENTRY(login_data->username_entry));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(login_data->password_entry));

    // Enviar matrícula y contraseña al servidor
    send(login_data->socket, username, strlen(username) + 1, 0);
    send(login_data->socket, password, strlen(password) + 1, 0);

    // Recibir respuesta del servidor (simplificado)
    char response[256];
    recv(login_data->socket, response, sizeof(response), 0);

    // Si la autenticación es exitosa
    if (strcmp(response, "success") == 0) {
        g_idle_add((GSourceFunc)gtk_label_set_text, login_data->status_label, "Autenticación exitosa");
    } else {
        g_idle_add((GSourceFunc)gtk_label_set_text, login_data->status_label, "Autenticación fallida");
    }

    // Cerrar el hilo
    return NULL;
}

// Función de conexión y autenticación del usuario
void on_login_button_clicked(GtkButton *button, gpointer user_data) {
    LoginData *login_data = (LoginData *)user_data;

    // Mostrar mensaje de "Cargando..."
    gtk_label_set_text(GTK_LABEL(login_data->status_label), "Autenticando...");

    // Crear el hilo para autenticar al usuario
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, authenticate_user, login_data);

    // No esperamos a que el hilo termine aquí, GTK se encargará de manejar los eventos
}

// Crear una conexión de socket al servidor
int create_socket_connection() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error de conexión");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

// Función principal de la aplicación
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Crear la ventana de inicio de sesión y otros widgets
    GtkWidget *login_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(login_window), "Inicio de sesión");
    gtk_window_set_default_size(GTK_WINDOW(login_window), 400, 300);
    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Matrícula");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Contraseña");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

    GtkWidget *login_button = gtk_button_new_with_label("Iniciar sesión");
    GtkWidget *status_label = gtk_label_new("Ingresa tus credenciales");

    // Crear el socket de conexión
    int sock = create_socket_connection();

    // Estructura para pasar los datos de inicio de sesión al hilo
    LoginData login_data = {
        .login_window = login_window,
        .username_entry = username_entry,
        .password_entry = password_entry,
        .status_label = status_label,
        .socket = sock
    };

    // Conectar el botón de inicio de sesión con la función de autenticación
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button_clicked), &login_data);

    // Organizar la interfaz y mostrarla
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(box), username_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), password_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), login_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), status_label, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(login_window), box);

    gtk_widget_show_all(login_window);
    gtk_main();

    // Cerrar el socket cuando se termine
    close(sock);

    return 0;
}
