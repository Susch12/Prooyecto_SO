#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_BUF 1024
#define TIEMPO_MAXIMO 600 // 10 minutos en segundos
#define MAX_PREGUNTAS 100
#define PREGUNTAS_EXAMEN 10

// Funciones declaradas
int autenticar_usuario(const char *matricula, const char *password);
int manejar_examen(const char *archivo_preguntas);
void mostrar_menu(int examenes_realizados[]);
char obtener_respuesta_con_tiempo();
void seleccionar_preguntas_aleatorias(char preguntas[][MAX_BUF], int total_preguntas, char seleccionadas[][MAX_BUF], int n);

// Main
int main() {
    char matricula[50], password[50];
    int seleccion, aciertos_totales = 0, examenes_realizados[3] = {0, 0, 0}, examenes_completados = 0;

    printf("Bienvenido al sistema de exámenes.\n");

    // Autenticación del usuario
    printf("Ingrese su matrícula: ");
    scanf("%s", matricula);
    printf("Ingrese su contraseña: ");
    scanf("%s", password);

    if (!autenticar_usuario(matricula, password)) {
        printf("Autenticación fallida. Matrícula o contraseña incorrecta.\n");
        return 1;
    }

    printf("Autenticación exitosa. Bienvenido, %s.\n", matricula);

    // Menú principal
    while (examenes_completados < 3) {
        mostrar_menu(examenes_realizados);

        printf("Seleccione el examen que desea realizar: ");
        scanf("%d", &seleccion);

        if (seleccion < 1 || seleccion > 3 || examenes_realizados[seleccion - 1]) {
            printf("Selección no válida o examen ya realizado. Intente nuevamente.\n");
            continue;
        }

        switch (seleccion) {
            case 1:
                printf("Iniciando examen de Matemáticas...\n");
                aciertos_totales += manejar_examen("mate.txt");
                break;
            case 2:
                printf("Iniciando examen de Español...\n");
                aciertos_totales += manejar_examen("español.txt");
                break;
            case 3:
                printf("Iniciando examen de Inglés...\n");
                aciertos_totales += manejar_examen("ingles.txt");
                break;
            default:
                printf("Opción no válida.\n");
                continue;
        }

        examenes_realizados[seleccion - 1] = 1; // Marcar examen como realizado
        examenes_completados++;
    }

    // Calcular porcentaje
    double porcentaje = (double) aciertos_totales / (PREGUNTAS_EXAMEN * 3) * 100;

    // Mostrar resultados finales
    printf("\n-------------------------\n");
    printf("Resumen Final:\n");
    printf("Aciertos totales: %d\n", aciertos_totales);
    printf("Calificación final: %.2f%%\n", porcentaje);
    printf("-------------------------\n");
    printf("Gracias por participar. Cerrando el programa...\n");

    return 0;
}

// Función para autenticar al usuario
int autenticar_usuario(const char *matricula, const char *password) {
    FILE *archivo = fopen("users.txt", "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo de usuarios");
        return 0;
    }

    char linea[MAX_BUF], matricula_archivo[50], password_archivo[50];
    while (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, "%s %s", matricula_archivo, password_archivo);
        if (strcmp(matricula, matricula_archivo) == 0 && strcmp(password, password_archivo) == 0) {
            fclose(archivo);
            return 1; // Usuario autenticado
        }
    }

    fclose(archivo);
    return 0; // Autenticación fallida
}

// Función para manejar un examen
int manejar_examen(const char *archivo_preguntas) {
    FILE *archivo = fopen(archivo_preguntas, "r");
    if (!archivo) {
        perror("No se pudo abrir el archivo de preguntas");
        return 0;
    }

    char preguntas[MAX_PREGUNTAS][MAX_BUF];
    char seleccionadas[PREGUNTAS_EXAMEN][MAX_BUF];
    char categoria[50], pregunta[256], opcion1[100], opcion2[100], opcion3[100], respuesta_correcta;
    char respuesta_usuario;
    int correctas = 0, total_preguntas = 0;

    // Leer todas las preguntas del archivo
    while (fgets(preguntas[total_preguntas], sizeof(preguntas[total_preguntas]), archivo)) {
        total_preguntas++;
        if (total_preguntas >= MAX_PREGUNTAS) {
            break; // Evitar exceder el límite
        }
    }
    fclose(archivo);

    // Seleccionar 10 preguntas aleatorias
    seleccionar_preguntas_aleatorias(preguntas, total_preguntas, seleccionadas, PREGUNTAS_EXAMEN);

    // Procesar las preguntas seleccionadas
    for (int i = 0; i < PREGUNTAS_EXAMEN; i++) {
        if (sscanf(seleccionadas[i], "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%c", categoria, pregunta, opcion1, opcion2, opcion3, &respuesta_correcta) != 6) {
            fprintf(stderr, "Formato incorrecto en la pregunta: %s\n", seleccionadas[i]);
            continue;
        }

        printf("\nCategoría: %s\n", categoria);
        printf("Pregunta %d: %s\n", i + 1, pregunta);
        printf("a) %s\n", opcion1);
        printf("b) %s\n", opcion2);
        printf("c) %s\n", opcion3);

        // Obtener respuesta con límite de tiempo
        printf("Selecciona tu respuesta (a, b, c): ");
        respuesta_usuario = obtener_respuesta_con_tiempo();
        if (respuesta_usuario == -1) {
            printf("Tiempo agotado. Respuesta incorrecta.\n");
        } else if (respuesta_usuario == respuesta_correcta) {
            printf("Respuesta correcta!\n");
            correctas++;
        } else {
            printf("Respuesta incorrecta.\n");
        }
    }

    printf("\nExamen terminado. Respuestas correctas: %d de %d.\n", correctas, PREGUNTAS_EXAMEN);
    return correctas;
}

// Función para mostrar el menú principal
void mostrar_menu(int examenes_realizados[]) {
    printf("\n--- EXÁMENES DISPONIBLES ---\n");
    printf("1. Matemáticas %s\n", examenes_realizados[0] ? "(Ya realizado)" : "");
    printf("2. Español %s\n", examenes_realizados[1] ? "(Ya realizado)" : "");
    printf("3. Inglés %s\n", examenes_realizados[2] ? "(Ya realizado)" : "");
    printf("---------------------------\n");
}

// Función para obtener la respuesta del usuario con límite de tiempo
char obtener_respuesta_con_tiempo() {
    time_t start_time, current_time;
    double elapsed_time;
    char respuesta;

    time(&start_time);
    printf("Tienes %d segundos para responder.\n", TIEMPO_MAXIMO);

    while (1) {
        time(&current_time);
        elapsed_time = difftime(current_time, start_time);

        if (elapsed_time >= TIEMPO_MAXIMO) {
            printf("Tiempo agotado.\n");
            return -1;
        }

        if (scanf(" %c", &respuesta) == 1 && (respuesta == 'a' || respuesta == 'b' || respuesta == 'c')) {
            return respuesta;
        }

        printf("Entrada inválida. Intente nuevamente (a, b o c): ");
    }
}

// Función para seleccionar preguntas aleatorias
void seleccionar_preguntas_aleatorias(char preguntas[][MAX_BUF], int total_preguntas, char seleccionadas[][MAX_BUF], int n) {
    srand(time(NULL)); // Inicializar la semilla aleatoria

    // Crear un arreglo con índices
    int indices[total_preguntas];
    for (int i = 0; i < total_preguntas; i++) {
        indices[i] = i;
    }

    // Algoritmo Fisher-Yates Shuffle para mezclar los índices
    for (int i = total_preguntas - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    // Seleccionar las primeras n preguntas mezcladas
    for (int i = 0; i < n; i++) {
        strcpy(seleccionadas[i], preguntas[indices[i]]);
    }
}
