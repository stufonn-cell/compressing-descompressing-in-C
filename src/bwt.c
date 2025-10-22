#include "bwt.h"
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <stdint.h> /
#include <stdio.h>  // binary file I/O

typedef struct {
    size_t index;
    int rank0;
    int rank1;
} suffix_t;

// Compare two suffixes by their ranks and index (works on binary bytes).
static int suffix_compare(const void *a, const void *b) {
    const suffix_t *sa = (const suffix_t *)a;
    const suffix_t *sb = (const suffix_t *)b;

    if (sa->rank0 != sb->rank0) {
        return (sa->rank0 < sb->rank0) ? -1 : 1;
    }
    if (sa->rank1 != sb->rank1) {
        return (sa->rank1 < sb->rank1) ? -1 : 1;
    }
    if (sa->index < sb->index) {
        return -1;
    }
    if (sa->index > sb->index) {
        return 1;
    }
    return 0;
}

// Return clamped block size (default 1 MiB).
static inline size_t clamp_block_size(size_t block_size) {
    return block_size == 0 ? (1u << 20) : block_size; /* 1 MiB default */
}

// Perform forward BWT on a binary input buffer.
// input/output are binary buffers of 'length' bytes. primary_index is the BWT primary index.
static bwt_status_t bwt_forward_core(const uint8_t *input, size_t length,
                                     uint8_t *output, size_t *primary_index,
                                     int requested_threads) {
    if (length == 0) {
        if (primary_index) {
            *primary_index = 0;
        }
        return BWT_STATUS_OK;
    }

    suffix_t *suffixes = (suffix_t *)malloc(length * sizeof(suffix_t));
    size_t *index_to_pos = (size_t *)malloc(length * sizeof(size_t));
    if (!suffixes || !index_to_pos) {
        free(suffixes);
        free(index_to_pos);
        return BWT_STATUS_ALLOCATION_FAILURE;
    }

#pragma omp parallel for schedule(static) if (length > 1024)
    for (size_t i = 0; i < length; ++i) {
        suffixes[i].index = i;
        suffixes[i].rank0 = input[i];
        suffixes[i].rank1 = (i + 1 < length) ? input[i + 1] : -1;
    }

    qsort(suffixes, length, sizeof(suffix_t), suffix_compare);

    for (size_t k = 4; k < (length << 1); k <<= 1) {
        int current_rank = 0;
        int prev_rank0 = suffixes[0].rank0;
        int prev_rank1 = suffixes[0].rank1;
        suffixes[0].rank0 = current_rank;
        index_to_pos[suffixes[0].index] = 0;

        for (size_t i = 1; i < length; ++i) {
            if (suffixes[i].rank0 == prev_rank0 && suffixes[i].rank1 == prev_rank1) {
                suffixes[i].rank0 = current_rank;
            } else {
                prev_rank0 = suffixes[i].rank0;
                prev_rank1 = suffixes[i].rank1;
                suffixes[i].rank0 = ++current_rank;
            }
            index_to_pos[suffixes[i].index] = i;
        }

        if (current_rank == (int)length - 1) {
            break; /* Early exit: all ranks are unique. */
        }

#pragma omp parallel for schedule(static) if (length > 1024)
        for (size_t i = 0; i < length; ++i) {
            size_t next_index = suffixes[i].index + (k >> 1);
            suffixes[i].rank1 = (next_index < length) ? suffixes[index_to_pos[next_index]].rank0 : -1;
        }

        qsort(suffixes, length, sizeof(suffix_t), suffix_compare);
    }

    size_t primary = 0;
    for (size_t i = 0; i < length; ++i) {
        size_t idx = suffixes[i].index;
        output[i] = input[(idx == 0) ? (length - 1) : (idx - 1)];
        if (idx == 0) {
            primary = i;
        }
    }

    free(suffixes);
    free(index_to_pos);

    if (primary_index) {
        *primary_index = primary;
    }

    return BWT_STATUS_OK;
}

// Perform inverse BWT on a binary input buffer.
// Reconstructs original binary data into output using LF-mapping.
static bwt_status_t bwt_inverse_core(const uint8_t *input, size_t length,
                                     size_t primary_index, uint8_t *output,
                                     int requested_threads) {
    if (length == 0) {
        return BWT_STATUS_OK;
    }
    if (primary_index >= length) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }

    size_t *lf = (size_t *)malloc(length * sizeof(size_t));
    if (!lf) {
        return BWT_STATUS_ALLOCATION_FAILURE;
    }

    size_t counts[256] = {0};
    (void)requested_threads;

#pragma omp parallel
    {
        size_t local_counts[256] = {0};
#pragma omp for schedule(static) nowait
        for (size_t i = 0; i < length; ++i) {
            local_counts[input[i]]++;
        }
#pragma omp critical
        {
            for (int c = 0; c < 256; ++c) {
                counts[c] += local_counts[c];
            }
        }
    }

    size_t totals[256];
    size_t sum = 0;
    for (int c = 0; c < 256; ++c) {
        totals[c] = sum;
        sum += counts[c];
    }

    size_t occ[256] = {0};
    for (size_t i = 0; i < length; ++i) {
        uint8_t ch = input[i];
        lf[i] = totals[ch] + occ[ch];
        occ[ch]++;
    }

    size_t idx = primary_index;
    for (size_t i = length; i-- > 0;) {
        uint8_t ch = input[idx];
        output[i] = ch;
        idx = lf[idx];
    }

    free(lf);
    return BWT_STATUS_OK;
}

void bwt_config_init(bwt_config_t *cfg) {
    if (!cfg) {
        return;
    }
    cfg->block_size = 1u << 20;
    cfg->threads = 0;
}

// Simple forward BWT API for binary buffers (validates args).
bwt_status_t bwt_forward(const uint8_t *input, size_t length,
                         uint8_t *output, size_t *primary_index) {
    if (!input || !output || !primary_index) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }
    return bwt_forward_core(input, length, output, primary_index, 0);
}

// Simple inverse BWT API for binary buffers (validates args).
bwt_status_t bwt_inverse(const uint8_t *input, size_t length,
                         size_t primary_index, uint8_t *output) {
    if (!input || !output) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }
    return bwt_inverse_core(input, length, primary_index, output, 0);
}

// Allocate output buffer and run forward BWT (binary).
bwt_status_t bwt_forward_alloc(const uint8_t *input, size_t length,
                               uint8_t **output, size_t *primary_index) {
    if (!input || !output || !primary_index) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }
    uint8_t *buffer = (uint8_t *)malloc(length ? length : 1);
    if (!buffer) {
        return BWT_STATUS_ALLOCATION_FAILURE;
    }
    bwt_status_t status = bwt_forward_core(input, length, buffer, primary_index, 0);
    if (status != BWT_STATUS_OK) {
        free(buffer);
        return status;
    }
    *output = buffer;
    return BWT_STATUS_OK;
}

// Allocate output buffer and run inverse BWT (binary).
bwt_status_t bwt_inverse_alloc(const uint8_t *input, size_t length,
                               size_t primary_index, uint8_t **output) {
    if (!input || !output) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }
    uint8_t *buffer = (uint8_t *)malloc(length ? length : 1);
    if (!buffer) {
        return BWT_STATUS_ALLOCATION_FAILURE;
    }
    bwt_status_t status = bwt_inverse_core(input, length, primary_index, buffer, 0);
    if (status != BWT_STATUS_OK) {
        free(buffer);
        return status;
    }
    *output = buffer;
    return BWT_STATUS_OK;
}

// Stream forward BWT over binary data.
// reader must read raw bytes and return number of bytes read.
// writer writes binary output and receives the primary index.
bwt_status_t bwt_forward_stream(const bwt_config_t *cfg,
                                bwt_read_cb reader, void *reader_ctx,
                                bwt_write_cb writer, void *writer_ctx) {
    if (!reader || !writer) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }

    bwt_config_t local_cfg;
    if (!cfg) {
        bwt_config_init(&local_cfg);
        cfg = &local_cfg;
    }

    size_t block_size = clamp_block_size(cfg->block_size);

    uint8_t *input_block = (uint8_t *)malloc(block_size);
    uint8_t *output_block = (uint8_t *)malloc(block_size);
    if (!input_block || !output_block) {
        free(input_block);
        free(output_block);
        return BWT_STATUS_ALLOCATION_FAILURE;
    }

    bwt_status_t status = BWT_STATUS_OK;

    while (1) {
        size_t got = reader(reader_ctx, input_block, block_size);
        if (got == 0) {
            break;
        }
        size_t primary_index = 0;
        status = bwt_forward_core(input_block, got, output_block, &primary_index, cfg->threads);
        if (status != BWT_STATUS_OK) {
            break;
        }
        if (writer(writer_ctx, output_block, got, primary_index) != 0) {
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }
    }

    free(input_block);
    free(output_block);
    return status;
}

// Stream inverse BWT over binary data.
// reader must read raw bytes and supply primary_index for each block.
// writer writes reconstructed binary data.
bwt_status_t bwt_inverse_stream(const bwt_config_t *cfg,
                                bwt_read_block_cb reader, void *reader_ctx,
                                bwt_write_cb writer, void *writer_ctx) {
    if (!reader || !writer) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }

    bwt_config_t local_cfg;
    if (!cfg) {
        bwt_config_init(&local_cfg);
        cfg = &local_cfg;
    }

    size_t capacity = clamp_block_size(cfg->block_size);

    uint8_t *input_block = (uint8_t *)malloc(capacity);
    uint8_t *output_block = (uint8_t *)malloc(capacity);
    if (!input_block || !output_block) {
        free(input_block);
        free(output_block);
        return BWT_STATUS_ALLOCATION_FAILURE;
    }

    bwt_status_t status = BWT_STATUS_OK;

    while (1) {
        size_t primary_index = 0;
        size_t got = reader(reader_ctx, input_block, capacity, &primary_index);
        if (got == 0) {
            break;
        }
        if (got > capacity) {
            uint8_t *tmp_in = (uint8_t *)realloc(input_block, got);
            if (!tmp_in) {
                status = BWT_STATUS_ALLOCATION_FAILURE;
                break;
            }
            input_block = tmp_in;

            uint8_t *tmp_out = (uint8_t *)realloc(output_block, got);
            if (!tmp_out) {
                status = BWT_STATUS_ALLOCATION_FAILURE;
                break;
            }
            output_block = tmp_out;
            capacity = got;
        }
        status = bwt_inverse_core(input_block, got, primary_index, output_block, cfg->threads);
        if (status != BWT_STATUS_OK) {
            break;
        }
        if (writer(writer_ctx, output_block, got, 0) != 0) {
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }
    }

    free(input_block);
    free(output_block);
    return status;
}

/* 
  Forward BWT reading from FILE* 'in' and writing binary blocks to FILE* 'out'.
  Block format written: [uint64_t length][uint64_t primary_index][length bytes of data]
  Uses fread/fwrite in binary mode; callers must open FILE* with "rb"/"wb".
*/
bwt_status_t bwt_forward_file(const bwt_config_t *cfg, FILE *in, FILE *out) {
    if (!in || !out) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }

    bwt_config_t local_cfg;
    if (!cfg) {
        bwt_config_init(&local_cfg);
        cfg = &local_cfg;
    }

    size_t block_size = clamp_block_size(cfg->block_size);

    uint8_t *input_block = (uint8_t *)malloc(block_size);
    uint8_t *output_block = (uint8_t *)malloc(block_size);
    if (!input_block || !output_block) {
        free(input_block);
        free(output_block);
        return BWT_STATUS_ALLOCATION_FAILURE;
    }

    bwt_status_t status = BWT_STATUS_OK;

    while (1) {
        size_t got = fread(input_block, 1, block_size, in);
        if (got == 0) {
            if (feof(in)) break;
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }

        size_t primary_index = 0;
        status = bwt_forward_core(input_block, got, output_block, &primary_index, cfg->threads);
        if (status != BWT_STATUS_OK) {
            break;
        }

        uint64_t len64 = (uint64_t)got;
        uint64_t prim64 = (uint64_t)primary_index;
        if (fwrite(&len64, sizeof(len64), 1, out) != 1 ||
            fwrite(&prim64, sizeof(prim64), 1, out) != 1 ||
            fwrite(output_block, 1, got, out) != got) {
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }
    }

    free(input_block);
    free(output_block);
    return status;
}

/* 
  Inverse BWT reading binary blocks from FILE* 'in' and writing reconstructed bytes to FILE* 'out'.
  Expects file format: [uint64_t length][uint64_t primary_index][length bytes of data]
  Uses fread/fwrite in binary mode; callers must open FILE* with "rb"/"wb".
*/
bwt_status_t bwt_inverse_file(const bwt_config_t *cfg, FILE *in, FILE *out) {
    if (!in || !out) {
        return BWT_STATUS_INVALID_ARGUMENT;
    }

    bwt_config_t local_cfg;
    if (!cfg) {
        bwt_config_init(&local_cfg);
        cfg = &local_cfg;
    }

    size_t capacity = clamp_block_size(cfg->block_size);

    uint8_t *input_block = (uint8_t *)malloc(capacity);
    uint8_t *output_block = (uint8_t *)malloc(capacity);
    if (!input_block || !output_block) {
        free(input_block);
        free(output_block);
        return BWT_STATUS_ALLOCATION_FAILURE;
    }

    bwt_status_t status = BWT_STATUS_OK;

    while (1) {
        uint64_t len64 = 0;
        uint64_t prim64 = 0;
        if (fread(&len64, sizeof(len64), 1, in) != 1) {
            if (feof(in)) { status = BWT_STATUS_OK; break; }
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }
        if (fread(&prim64, sizeof(prim64), 1, in) != 1) {
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }

        size_t got = (size_t)len64;
        size_t primary_index = (size_t)prim64;

        if (got > capacity) {
            uint8_t *tmp_in = (uint8_t *)realloc(input_block, got);
            if (!tmp_in) { status = BWT_STATUS_ALLOCATION_FAILURE; break; }
            input_block = tmp_in;

            uint8_t *tmp_out = (uint8_t *)realloc(output_block, got);
            if (!tmp_out) { status = BWT_STATUS_ALLOCATION_FAILURE; break; }
            output_block = tmp_out;
            capacity = got;
        }

        if (fread(input_block, 1, got, in) != got) {
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }

        status = bwt_inverse_core(input_block, got, primary_index, output_block, cfg->threads);
        if (status != BWT_STATUS_OK) {
            break;
        }

        if (fwrite(output_block, 1, got, out) != got) {
            status = BWT_STATUS_INTERNAL_ERROR;
            break;
        }
    }

    free(input_block);
    free(output_block);
    return status;
}
