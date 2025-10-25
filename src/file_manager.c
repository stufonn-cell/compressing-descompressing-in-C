#include "file_manager.h"
#include "bwt.h"
#include "rle.h"
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

// Structure for file metadata in the .w container
typedef struct
{
    uint64_t filename_len;
    uint64_t data_len;
    uint64_t primary_index;
} file_header_t;

fm_path_type_t fm_get_path_type(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
    {
        return FM_TYPE_FILE; // By default, treat as file
    }
    return S_ISDIR(statbuf.st_mode) ? FM_TYPE_DIRECTORY : FM_TYPE_FILE;
}

// Compresses an individual file using BWT
static fm_status_t compress_single_file(FILE *in, FILE *out, const char *filename)
{
    if (!in || !out || !filename)
    {
        return FM_STATUS_INVALID_ARGUMENT;
    }

    // Read entire file and append a single '$' sentinel byte so BWT has a unique terminator.
    fseek(in, 0, SEEK_END);
    long file_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    if (file_size < 0)
    {
        return FM_STATUS_IO_ERROR;
    }

    // Allocate input/output buffers for file_size + 1 to accommodate the sentinel
    size_t with_sentinel = (size_t)file_size + 1;
    uint8_t *input = (uint8_t *)malloc(with_sentinel);
    uint8_t *output = (uint8_t *)malloc(with_sentinel);
    if (!input || !output)
    {
        free(input);
        free(output);
        return FM_STATUS_ALLOCATION_FAILURE;
    }

    if (file_size > 0)
    {
        if (fread(input, 1, (size_t)file_size, in) != (size_t)file_size)
        {
            free(input);
            free(output);
            return FM_STATUS_IO_ERROR;
        }
    }

    // Append sentinel '$' at the end
    input[with_sentinel - 1] = (uint8_t)'$';

    size_t primary_index = 0;
    bwt_status_t bwt_status = bwt_forward((const uint8_t *)input, with_sentinel, output, &primary_index);
    if (bwt_status != BWT_STATUS_OK)
    {
        free(input);
        free(output);
        return FM_STATUS_ERROR;
    }

    // Allocate worst-case size: RLE can expand to 2x if no runs exist
    size_t max_rled_size = with_sentinel * 2;
    uint8_t *rled_output = (uint8_t *)malloc(max_rled_size);
    if (!rled_output)
    {
        free(input);
        free(output);
        return FM_STATUS_ALLOCATION_FAILURE;
    }
    size_t rled_size = max_rled_size;
    rle_encode(output, with_sentinel, rled_output, &rled_size);
    // rled_size now contains the actual compressed size

    uint64_t filename_len = strlen(filename);
    // Store data_len as original_size + 1 to include the sentinel in the compressed container
    uint64_t data_len = (uint64_t)with_sentinel;
    uint64_t prim_idx = (uint64_t)primary_index;
    uint64_t compressed_len = (uint64_t)rled_size;

    if (fwrite(&filename_len, sizeof(filename_len), 1, out) != 1 ||
        fwrite(filename, 1, filename_len, out) != filename_len ||
        fwrite(&data_len, sizeof(data_len), 1, out) != 1 ||
        fwrite(&prim_idx, sizeof(prim_idx), 1, out) != 1 ||
        fwrite(&compressed_len, sizeof(compressed_len), 1, out) != 1 ||
        fwrite(rled_output, 1, rled_size, out) != rled_size)
    {
        free(input);
        free(output);
        free(rled_output);
        return FM_STATUS_IO_ERROR;
    }

    free(input);
    free(output);
    free(rled_output);
    return FM_STATUS_OK;
}

// Recursively processes a directory
static fm_status_t compress_directory_recursive(const char *dir_path, FILE *out, const char *base_path)
{
    DIR *dir = opendir(dir_path);
    if (!dir)
    {
        return FM_STATUS_FILE_NOT_FOUND;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) != 0)
        {
            continue;
        }

        if (S_ISDIR(statbuf.st_mode))
        {
            // Recursively process subdirectory
            fm_status_t status = compress_directory_recursive(full_path, out, base_path);
            if (status != FM_STATUS_OK)
            {
                closedir(dir);
                return status;
            }
        }
        else if (S_ISREG(statbuf.st_mode))
        {
            // Compress file with relative path
            FILE *in = fopen(full_path, "rb");
            if (!in)
            {
                continue;
            }

            // Calculate relative path
            char relative_path[MAX_PATH];
            if (strlen(full_path) > strlen(base_path) + 1)
            {
                strcpy(relative_path, full_path + strlen(base_path) + 1);
            }
            else
            {
                strcpy(relative_path, entry->d_name);
            }

            fm_status_t status = compress_single_file(in, out, relative_path);
            fclose(in);
            if (status != FM_STATUS_OK)
            {
                closedir(dir);
                return status;
            }
        }
    }

    closedir(dir);
    return FM_STATUS_OK;
}

fm_status_t fm_compress(const char *input_path, const char *output_path)
{
    if (!input_path || !output_path)
    {
        return FM_STATUS_INVALID_ARGUMENT;
    }

    FILE *out = fopen(output_path, "wb");
    if (!out)
    {
        return FM_STATUS_IO_ERROR;
    }

    fm_path_type_t path_type = fm_get_path_type(input_path);
    fm_status_t status = FM_STATUS_OK;

    if (path_type == FM_TYPE_FILE)
    {
        FILE *in = fopen(input_path, "rb");
        if (!in)
        {
            fclose(out);
            return FM_STATUS_FILE_NOT_FOUND;
        }
        char *filename = basename((char *)input_path);
        status = compress_single_file(in, out, filename);
        fclose(in);
    }
    else
    {
        status = compress_directory_recursive(input_path, out, input_path);
    }

    fclose(out);
    return status;
}

// Create necessary directories
static fm_status_t create_directories(const char *filepath)
{
    if (!filepath || filepath[0] == '\0')
    {
        return FM_STATUS_OK;
    }

    char *path = (char *)malloc(strlen(filepath) + 1);
    if (!path)
    {
        return FM_STATUS_ALLOCATION_FAILURE;
    }

    strcpy(path, filepath);
    char *dir = dirname(path);

    // Create directories recursively
    char temp[MAX_PATH];
    strcpy(temp, dir);

    for (size_t i = 1; i < strlen(temp); i++)
    {
        if (temp[i] == '/')
        {
            temp[i] = '\0';
            mkdir(temp, 0755);
            temp[i] = '/';
        }
    }
    mkdir(temp, 0755);

    free(path);
    return FM_STATUS_OK;
}

// Decompress .w file
fm_status_t fm_decompress(const char *input_path, const char *output_path)
{
    if (!input_path || !output_path)
    {
        return FM_STATUS_INVALID_ARGUMENT;
    }

    FILE *in = fopen(input_path, "rb");
    if (!in)
    {
        return FM_STATUS_FILE_NOT_FOUND;
    }

    fm_status_t status = FM_STATUS_OK;

    // Create base output directory if it does not exist
    mkdir(output_path, 0755);

    while (1)
    {
        uint64_t filename_len = 0;
        if (fread(&filename_len, sizeof(filename_len), 1, in) != 1)
        {
            printf("aqui 1\n");
            if (feof(in))
                break;
            status = FM_STATUS_IO_ERROR;
            break;
        }

        char *filename = (char *)malloc(filename_len + 1);
        if (!filename)
        {
            status = FM_STATUS_ALLOCATION_FAILURE;
            break;
        }

        if (fread(filename, 1, filename_len, in) != filename_len)
        {
            free(filename);
            status = FM_STATUS_IO_ERROR;
            printf("aqui 2\n");
            break;
        }
        filename[filename_len] = '\0';

        uint64_t data_len = 0;
        uint64_t primary_index = 0;
        uint64_t compressed_len = 0;
        if (fread(&data_len, sizeof(data_len), 1, in) != 1 ||
            fread(&primary_index, sizeof(primary_index), 1, in) != 1 ||
            fread(&compressed_len, sizeof(compressed_len), 1, in) != 1)
        {
            free(filename);
            status = FM_STATUS_IO_ERROR;
            printf("aqui 3\n");
            break;
        }

        // Allocate buffer for compressed (RLE) data
        uint8_t *compressed_data = (uint8_t *)malloc(compressed_len);
        // Allocate buffer for decompressed (BWT) data
        uint8_t *bwt_data = (uint8_t *)malloc(data_len);
        // Allocate buffer for final output data
        uint8_t *output_data = (uint8_t *)malloc(data_len);

        if (!compressed_data || !bwt_data || !output_data)
        {
            free(filename);
            free(compressed_data);
            free(bwt_data);
            free(output_data);
            status = FM_STATUS_ALLOCATION_FAILURE;
            break;
        }

        // Read the compressed (RLE) data
        if (fread(compressed_data, 1, compressed_len, in) != compressed_len)
        {
            free(filename);
            free(compressed_data);
            free(bwt_data);
            free(output_data);
            status = FM_STATUS_IO_ERROR;
            printf("aqui 4\n");
            printf("compressed_len: %lu\n", compressed_len);
            break;
        }

        // First, decode RLE to get BWT data
        size_t bwt_size = data_len;
        rle_decode(compressed_data, compressed_len, bwt_data, &bwt_size);

        if (bwt_size != data_len)
        {
            free(filename);
            free(compressed_data);
            free(bwt_data);
            free(output_data);
            status = FM_STATUS_ERROR;
            break;
        }

        // Then, reverse BWT to get original data
        bwt_status_t bwt_status = bwt_inverse(bwt_data, bwt_size, (size_t)primary_index, output_data);
        if (bwt_status != BWT_STATUS_OK)
        {
            free(filename);
            free(compressed_data);
            free(bwt_data);
            free(output_data);
            status = FM_STATUS_ERROR;
            break;
        }

        // Construct full path
        char full_output_path[MAX_PATH];
        snprintf(full_output_path, sizeof(full_output_path), "%s/%s", output_path, filename);

        // Create necessary directories
        create_directories(full_output_path);

        // Write file
        FILE *out = fopen(full_output_path, "wb");
        if (!out)
        {
            free(filename);
            free(compressed_data);
            free(bwt_data);
            free(output_data);
            status = FM_STATUS_IO_ERROR;
            printf("aqui 5\n");
            break;
        }

        // If we appended a sentinel during compression, strip the trailing '$' when writing
        // so the decompressed file matches the original.
        size_t write_len = (size_t)data_len;
        if (write_len > 0 && output_data[write_len - 1] == (uint8_t)'$')
        {
            write_len -= 1; // remove sentinel from output
        }

        if (fwrite(output_data, 1, write_len, out) != write_len)
        {
            free(filename);
            free(compressed_data);
            free(bwt_data);
            free(output_data);
            fclose(out);
            status = FM_STATUS_IO_ERROR;
            printf("aqui 6\n");
            break;
        }

        fclose(out);
        free(filename);
        free(compressed_data);
        free(bwt_data);
        free(output_data);
    }

    fclose(in);
    return status;
}