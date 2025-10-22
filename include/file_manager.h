#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    FM_STATUS_OK = 0,
    FM_STATUS_ERROR = 1,
    FM_STATUS_FILE_NOT_FOUND = 2,
    FM_STATUS_ALLOCATION_FAILURE = 3,
    FM_STATUS_INVALID_ARGUMENT = 4,
    FM_STATUS_IO_ERROR = 5
} fm_status_t;

typedef enum {
    FM_TYPE_FILE,
    FM_TYPE_DIRECTORY
} fm_path_type_t;

// Detecta si la ruta es un archivo o directorio
fm_path_type_t fm_get_path_type(const char *path);

// Comprime un archivo o directorio y guarda resultado en .w
fm_status_t fm_compress(const char *input_path, const char *output_path);

// Descomprime un archivo .w
fm_status_t fm_decompress(const char *input_path, const char *output_path);

#endif // FILE_MANAGER_H