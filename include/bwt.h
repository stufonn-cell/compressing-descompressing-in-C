#ifndef BWT_H
#define BWT_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BWT_STATUS_OK = 0,
    BWT_STATUS_INVALID_ARGUMENT = -1,
    BWT_STATUS_ALLOCATION_FAILURE = -2,
    BWT_STATUS_INTERNAL_ERROR = -3
} bwt_status_t;

typedef struct {
    size_t block_size;
    int threads;
} bwt_config_t;

void bwt_config_init(bwt_config_t *cfg);
bwt_status_t bwt_forward(const uint8_t *input, size_t length,
                         uint8_t *output, size_t *primary_index);
bwt_status_t bwt_inverse(const uint8_t *input, size_t length,
                         size_t primary_index, uint8_t *output);
bwt_status_t bwt_forward_alloc(const uint8_t *input, size_t length,
                               uint8_t **output, size_t *primary_index);
bwt_status_t bwt_inverse_alloc(const uint8_t *input, size_t length,
                               size_t primary_index, uint8_t **output);
typedef size_t (*bwt_read_cb)(void *user_ctx, uint8_t *buffer, size_t max_len);
typedef int (*bwt_write_cb)(void *user_ctx, const uint8_t *buffer,
                            size_t length, size_t primary_index);
typedef size_t (*bwt_read_block_cb)(void *user_ctx, uint8_t *buffer,
                                    size_t max_len, size_t *primary_index);
bwt_status_t bwt_forward_stream(const bwt_config_t *cfg,
                                bwt_read_cb reader, void *reader_ctx,
                                bwt_write_cb writer, void *writer_ctx);
bwt_status_t bwt_inverse_stream(const bwt_config_t *cfg,
                                bwt_read_block_cb reader, void *reader_ctx,
                                bwt_write_cb writer, void *writer_ctx);

#endif
