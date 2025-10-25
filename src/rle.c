#include "rle.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void rle_encode(const uint8_t *input, size_t input_size, uint8_t *output, size_t *output_size)
{
    size_t out_index = 0;
    size_t i = 0;

    // Ensure input, output, and output_size are not NULL and input_size is not zero
    if (input == NULL || input_size == 0 || output == NULL || output_size == NULL)
    {
        return;
    }

    // The maximum possible output size is input_size * 2 (worst case: no runs)
    size_t max_output_size = (*output_size);

    while (i < input_size)
    {
        uint8_t current_byte = input[i];
        size_t run_length = 1;

        while (i + run_length < input_size && input[i + run_length] == current_byte && run_length < 255)
        {
            run_length++;
        }

        // Check for buffer overflow before writing
        if (out_index + 2 > max_output_size)
        {
            // Not enough space in output buffer
            break;
        }

        output[out_index++] = current_byte;
        output[out_index++] = (uint8_t)run_length;

        i += run_length;
    }

    *output_size = out_index;
}

void rle_decode(const uint8_t *input, size_t input_size, uint8_t *output, size_t *output_size)
{
    size_t out_index = 0;
    size_t i = 0;

    if (input == NULL || output == NULL || output_size == NULL)
    {
        return;
    }

    while (i < input_size)
    {
        uint8_t current_byte = input[i];
        uint8_t run_length = input[i + 1];

        for (size_t j = 0; j < run_length; j++)
        {
            output[out_index++] = current_byte;
        }

        i += 2;
    }

    *output_size = out_index;
}
