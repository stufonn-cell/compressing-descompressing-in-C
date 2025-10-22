#include "bwt.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_roundtrip_literal(const uint8_t *data, size_t len) {
    uint8_t *encoded = malloc(len ? len : 1);
    uint8_t *decoded = malloc(len ? len : 1);
    assert(encoded && decoded);

    size_t primary = SIZE_MAX;
    assert(bwt_forward(data, len, encoded, &primary) == BWT_STATUS_OK);
    assert(primary < len || len == 0);
    assert(bwt_inverse(encoded, len, primary, decoded) == BWT_STATUS_OK);
    assert(memcmp(decoded, data, len) == 0);

    free(encoded);
    free(decoded);
}

static void test_roundtrip_alloc(const uint8_t *data, size_t len) {
    uint8_t *encoded = NULL;
    uint8_t *decoded = NULL;
    size_t primary = SIZE_MAX;

    assert(bwt_forward_alloc(data, len, &encoded, &primary) == BWT_STATUS_OK);
    assert(primary < len || len == 0);
    assert(bwt_inverse_alloc(encoded, len, primary, &decoded) == BWT_STATUS_OK);
    assert(memcmp(decoded, data, len) == 0);

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
    const uint8_t banana[] = { 'b','a','n','a','n','a','$' };
    const uint8_t mississippi[] = { 'm','i','s','s','i','s','s','i','p','p','i' };
    const uint8_t abracadabra[] = { 'a','b','r','a','c','a','d','a','b','r','a' };
    const uint8_t with_zero[] = { 0x00, 0x41, 0x00, 0x42, 0x00 };

    test_roundtrip_literal(banana, sizeof(banana));
    test_roundtrip_literal(mississippi, sizeof(mississippi));
    test_roundtrip_literal(abracadabra, sizeof(abracadabra));
    test_roundtrip_literal(with_zero, sizeof(with_zero));

    test_roundtrip_alloc(banana, sizeof(banana));
    test_roundtrip_alloc(abracadabra, sizeof(abracadabra));
    test_empty();
    puts("BWT tests passed.");
    return 0;
}