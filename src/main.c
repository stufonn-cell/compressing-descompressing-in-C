#include "file_manager.h"
#include <stdio.h>
#include <string.h>

void print_status(fm_status_t status) {
    switch (status) {
        case FM_STATUS_OK:
            printf("Operación exitosa\n");
            break;
        case FM_STATUS_ERROR:
            printf("Error general\n");
            break;
        case FM_STATUS_FILE_NOT_FOUND:
            printf("Archivo no encontrado\n");
            break;
        case FM_STATUS_ALLOCATION_FAILURE:
            printf("Error de asignación de memoria\n");
            break;
        case FM_STATUS_INVALID_ARGUMENT:
            printf("Argumento inválido\n");
            break;
        case FM_STATUS_IO_ERROR:
            printf("Error de I/O\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso:\n");
        printf("  %s compress <archivo_entrada> <archivo_salida.w>\n", argv[0]);
        printf("  %s decompress <archivo.w> <archivo_salida>\n", argv[0]);
        printf("  %s type <ruta>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "compress") == 0) {
        if (argc != 4) {
            printf("Error: Se requieren entrada y salida\n");
            return 1;
        }
        printf("Comprimiendo: %s -> %s\n", argv[2], argv[3]);
        fm_status_t status = fm_compress(argv[2], argv[3]);
        print_status(status);
        return status == FM_STATUS_OK ? 0 : 1;
    }
    else if (strcmp(argv[1], "decompress") == 0) {
        if (argc != 4) {
            printf("Error: Se requieren entrada y salida\n");
            return 1;
        }
        printf("Descomprimiendo: %s -> %s\n", argv[2], argv[3]);
        fm_status_t status = fm_decompress(argv[2], argv[3]);
        print_status(status);
        return status == FM_STATUS_OK ? 0 : 1;
    }
    else if (strcmp(argv[1], "type") == 0) {
        if (argc != 3) {
            printf("Error: Se requiere una ruta\n");
            return 1;
        }
        fm_path_type_t type = fm_get_path_type(argv[2]);
        printf("Tipo: %s\n", type == FM_TYPE_FILE ? "ARCHIVO" : "DIRECTORIO");
        return 0;
    }
    else {
        printf("Comando desconocido: %s\n", argv[1]);
        return 1;
    }
}