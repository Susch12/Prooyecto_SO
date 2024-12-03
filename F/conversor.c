#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 20

typedef struct {
    char matricula[10];
    char password[10];
    int calificaciones[3];
} Usuario;

void convertir_texto_a_bin(const char *nombre_archivo_entrada, const char *nombre_archivo_salida) {
    FILE *archivo_entrada = fopen(nombre_archivo_entrada, "r");
    if (archivo_entrada == NULL) {
        perror("Error al abrir el archivo de entrada");
        exit(EXIT_FAILURE);
    }

    FILE *archivo_salida = fopen(nombre_archivo_salida, "wb");
    if (archivo_salida == NULL) {
        perror("Error al abrir el archivo de salida");
        fclose(archivo_entrada);
        exit(EXIT_FAILURE);
    }

    Usuario usuario;
    while (fscanf(archivo_entrada, "%s %s", usuario.matricula, usuario.password) == 2) {
        // Inicializar las calificaciones a 0
        memset(usuario.calificaciones, 0, sizeof(usuario.calificaciones));
        
        // Escribir el usuario en el archivo binario
        fwrite(&usuario, sizeof(Usuario), 1, archivo_salida);
    }

    fclose(archivo_entrada);
    fclose(archivo_salida);

    printf("Conversión a archivo binario completada.\n");
}

int main() {

    convertir_texto_a_bin("mate.txt", "mate.bin");
    return 0;
}
