#include "bwt.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_roundtrip_literal(const char *literal) {
    size_t len = strlen(literal);
    uint8_t *encoded = malloc(len ? len : 1);
    uint8_t *decoded = malloc(len ? len : 1);
    assert(encoded && decoded);

    size_t primary = SIZE_MAX;
    assert(bwt_forward((const uint8_t *)literal, len, encoded, &primary) == BWT_STATUS_OK);
    assert(primary < len || len == 0);
    assert(bwt_inverse(encoded, len, primary, decoded) == BWT_STATUS_OK);
    assert(memcmp(decoded, literal, len) == 0);

    free(encoded);
    free(decoded);
}

static void test_roundtrip_alloc(const char *literal) {
    size_t len = strlen(literal);
    uint8_t *encoded = NULL;
    uint8_t *decoded = NULL;
    size_t primary = SIZE_MAX;

    assert(bwt_forward_alloc((const uint8_t *)literal, len, &encoded, &primary) == BWT_STATUS_OK);
    assert(primary < len || len == 0);
    assert(bwt_inverse_alloc(encoded, len, primary, &decoded) == BWT_STATUS_OK);
    assert(memcmp(decoded, literal, len) == 0);

    free(encoded);
    free(decoded);
}

static void test_empty(void) {
    uint8_t dummy = 0;
    size_t primary = SIZE_MAX;
    assert(bwt_forward((const uint8_t *)"", 0, &dummy, &primary) == BWT_STATUS_OK);
    assert(primary == 0);
    assert(bwt_inverse(&dummy, 0, 0, &dummy) == BWT_STATUS_OK);
}

int main(void) {
    test_roundtrip_literal("banana$");
    test_roundtrip_literal("mississippi");
    test_roundtrip_literal("abracadabra");
    test_roundtrip_alloc("banana$");
    test_roundtrip_alloc("abracadabra");
    test_empty();

    puts("BWT tests passed.");
    return 0;
}