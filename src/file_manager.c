#include "file_manager.h"
#include "bwt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

#define MAX_PATH 4096
#define BUFFER_SIZE (1024 * 1024) // 1 MiB

// Estructura para metadatos de archivos en el contenedor .w
typedef struct {
    uint64_t filename_len;
    uint64_t data_len;
    uint64_t primary_index;
} file_header_t;

fm_path_type_t fm_get_path_type(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return FM_TYPE_FILE; // Por defecto, tratar como archivo
    }
    return S_ISDIR(statbuf.st_mode) ? FM_TYPE_DIRECTORY : FM_TYPE_FILE;
}

// Comprime un archivo individual con BWT
static fm_status_t compress_single_file(FILE *in, FILE *out, const char *filename) {
    if (!in || !out || !filename) {
        return FM_STATUS_INVALID_ARGUMENT;
    }

    // Leer archivo completo
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    if (file_size < 0) {
        return FM_STATUS_IO_ERROR;
    }

    uint8_t *input = (uint8_t *)malloc(file_size);
    uint8_t *output = (uint8_t *)malloc(file_size);
    if (!input || !output) {
        free(input);
        free(output);
        return FM_STATUS_ALLOCATION_FAILURE;
    }

    if (fread(input, 1, file_size, in) != (size_t)file_size) {
        free(input);
        free(output);
        return FM_STATUS_IO_ERROR;
    }

    size_t primary_index = 0;
    bwt_status_t bwt_status = bwt_forward((const uint8_t *)input, file_size, output, &primary_index);
    if (bwt_status != BWT_STATUS_OK) {
        free(input);
        free(output);
        return FM_STATUS_ERROR;
    }
    
    uint64_t filename_len = strlen(filename);
    uint64_t data_len = (uint64_t)file_size;
    uint64_t prim_idx = (uint64_t)primary_index;
    if (fwrite(&filename_len, sizeof(filename_len), 1, out) != 1 ||
        fwrite(filename, 1, filename_len, out) != filename_len ||
        fwrite(&data_len, sizeof(data_len), 1, out) != 1 ||
        fwrite(&prim_idx, sizeof(prim_idx), 1, out) != 1 ||
        fwrite(output, 1, file_size, out) != (size_t)file_size) {
        free(input);
        free(output);
        return FM_STATUS_IO_ERROR;
    }
    
    free(input);
    free(output);
    return FM_STATUS_OK;
}

// Procesa recursivamente un directorio
static fm_status_t compress_directory_recursive(const char *dir_path, FILE *out, const char *base_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return FM_STATUS_FILE_NOT_FOUND;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) != 0) {
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Recursivamente procesar subdirectorio
            fm_status_t status = compress_directory_recursive(full_path, out, base_path);
            if (status != FM_STATUS_OK) {
                closedir(dir);
                return status;
            }
        } else if (S_ISREG(statbuf.st_mode)) {
            // Comprimir archivo con ruta relativa
            FILE *in = fopen(full_path, "rb");
            if (!in) {
                continue;
            }

            // Calcular ruta relativa
            char relative_path[MAX_PATH];
            if (strlen(full_path) > strlen(base_path) + 1) {
                strcpy(relative_path, full_path + strlen(base_path) + 1);
            } else {
                strcpy(relative_path, entry->d_name);
            }

            fm_status_t status = compress_single_file(in, out, relative_path);
            fclose(in);
            if (status != FM_STATUS_OK) {
                closedir(dir);
                return status;
            }
        }
    }

    closedir(dir);
    return FM_STATUS_OK;
}

fm_status_t fm_compress(const char *input_path, const char *output_path) {
    if (!input_path || !output_path) {
        return FM_STATUS_INVALID_ARGUMENT;
    }

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        return FM_STATUS_IO_ERROR;
    }

    fm_path_type_t path_type = fm_get_path_type(input_path);
    fm_status_t status = FM_STATUS_OK;

    if (path_type == FM_TYPE_FILE) {
        FILE *in = fopen(input_path, "rb");
        if (!in) {
            fclose(out);
            return FM_STATUS_FILE_NOT_FOUND;
        }
        char *filename = basename((char *)input_path);
        status = compress_single_file(in, out, filename);
        fclose(in);
    } else {
        status = compress_directory_recursive(input_path, out, input_path);
    }

    fclose(out);
    return status;
}

// Crea directorios necesarios
static fm_status_t create_directories(const char *filepath) {
    if (!filepath || filepath[0] == '\0') {
        return FM_STATUS_OK;
    }

    char *path = (char *)malloc(strlen(filepath) + 1);
    if (!path) {
        return FM_STATUS_ALLOCATION_FAILURE;
    }

    strcpy(path, filepath);
    char *dir = dirname(path);

    // Crear directorios recursivamente
    char temp[MAX_PATH];
    strcpy(temp, dir);

    for (size_t i = 1; i < strlen(temp); i++) {
        if (temp[i] == '/') {
            temp[i] = '\0';
            mkdir(temp, 0755);
            temp[i] = '/';
        }
    }
    mkdir(temp, 0755);

    free(path);
    return FM_STATUS_OK;
}

// Descomprime archivo .w
fm_status_t fm_decompress(const char *input_path, const char *output_path) {
    if (!input_path || !output_path) {
        return FM_STATUS_INVALID_ARGUMENT;
    }

    FILE *in = fopen(input_path, "rb");
    if (!in) {
        return FM_STATUS_FILE_NOT_FOUND;
    }

    fm_status_t status = FM_STATUS_OK;

    // Crear directorio base de salida si no existe
    mkdir(output_path, 0755);

    while (1) {
        uint64_t filename_len = 0;
        if (fread(&filename_len, sizeof(filename_len), 1, in) != 1) {
            if (feof(in)) break;
            status = FM_STATUS_IO_ERROR;
            break;
        }

        char *filename = (char *)malloc(filename_len + 1);
        if (!filename) {
            status = FM_STATUS_ALLOCATION_FAILURE;
            break;
        }

        if (fread(filename, 1, filename_len, in) != filename_len) {
            free(filename);
            status = FM_STATUS_IO_ERROR;
            break;
        }
        filename[filename_len] = '\0';

        uint64_t data_len = 0;
        uint64_t primary_index = 0;
        if (fread(&data_len, sizeof(data_len), 1, in) != 1 ||
            fread(&primary_index, sizeof(primary_index), 1, in) != 1) {
            free(filename);
            status = FM_STATUS_IO_ERROR;
            break;
        }

        uint8_t *input_data = (uint8_t *)malloc(data_len);
        uint8_t *output_data = (uint8_t *)malloc(data_len);
        if (!input_data || !output_data) {
            free(filename);
            free(input_data);
            free(output_data);
            status = FM_STATUS_ALLOCATION_FAILURE;
            break;
        }

        if (fread(input_data, 1, data_len, in) != data_len) {
            free(filename);
            free(input_data);
            free(output_data);
            status = FM_STATUS_IO_ERROR;
            break;
        }

        bwt_status_t bwt_status = bwt_inverse(input_data, data_len, (size_t)primary_index, output_data);
        if (bwt_status != BWT_STATUS_OK) {
            free(filename);
            free(input_data);
            free(output_data);
            status = FM_STATUS_ERROR;
            break;
        }

        // Construir ruta completa
        char full_output_path[MAX_PATH];
        snprintf(full_output_path, sizeof(full_output_path), "%s/%s", output_path, filename);

        // Crear directorios necesarios
        create_directories(full_output_path);

        // Escribir archivo
        FILE *out = fopen(full_output_path, "wb");
        if (!out) {
            free(filename);
            free(input_data);
            free(output_data);
            status = FM_STATUS_IO_ERROR;
            break;
        }

        if (fwrite(output_data, 1, data_len, out) != data_len) {
            free(filename);
            free(input_data);
            free(output_data);
            fclose(out);
            status = FM_STATUS_IO_ERROR;
            break;
        }

        fclose(out);
        free(filename);
        free(input_data);
        free(output_data);
    }

    fclose(in);
    return status;
}