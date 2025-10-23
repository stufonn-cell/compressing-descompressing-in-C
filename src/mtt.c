#include "mtt.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mtt_encode(const uint8_t *input, size_t input_size, uint8_t *output, size_t *output_size) {
    uint8_t symbol_table[256];
    for (int i = 0; i < 256; i++) {
        symbol_table[i] = (uint8_t)i;
    }
    size_t out_index = 0;
    for (size_t i = 0; i < input_size; i++) {
        uint8_t current_byte = input[i];
        // Find the index of the current byte in the symbol table
        size_t index = 0;
        while (symbol_table[index] != current_byte) {
            index++;
        }
        // Output the index
        output[out_index++] = (uint8_t)index;
        // Move the current byte to the front of the symbol table
        for (size_t j = index; j > 0; j--) {
            symbol_table[j] = symbol_table[j - 1];
        }
        symbol_table[0] = current_byte;
    }
    *output_size = out_index;
}

void mtt_decode(const uint8_t *input, size_t input_size, uint8_t *output, size_t *output_size) {
    uint8_t symbol_table[256];
    for (int i = 0; i < 256; i++) {
        symbol_table[i] = (uint8_t)i;
    }
    size_t out_index = 0;
    for (size_t i = 0; i < input_size; i++) {
        uint8_t index = input[i];
        uint8_t current_byte = symbol_table[index];
        // Output the current byte
        output[out_index++] = current_byte;
        // Move the current byte to the front of the symbol table
        for (size_t j = index; j > 0; j--) {
            symbol_table[j] = symbol_table[j - 1];
        }
        symbol_table[0] = current_byte;
    }
    *output_size = out_index;
}