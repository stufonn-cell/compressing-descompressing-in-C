#ifndef MTT_H
#define MTT_H

#include <stddef.h>
#include <stdint.h>

void mtt_encode(const uint8_t *input, size_t input_size, uint8_t *output, size_t *output_size);
void mtt_decode(const uint8_t *input, size_t input_size, uint8_t *output, size_t *output_size);

#endif // MTT_H