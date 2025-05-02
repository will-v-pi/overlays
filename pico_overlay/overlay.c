#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/sha256.h"
#include "pico/overlay.h"


bool overlay_load(uint8_t* start, uint8_t* stop, char* hash) {
    static uint8_t* current_start = NULL;
    if (current_start == start) {
        return true;
    }
    current_start = start;
    printf("loading overlay from %p->%p into %p->%p\n", start, stop, __overlays_start, __overlays_end);
    memcpy(__overlays_start, start, stop - start);
    memset(__overlays_start + (stop - start), 0, __overlays_end - __overlays_start - (stop - start));

    printf("checking hash\n");
    pico_sha256_state_t state;
    int rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, true); // using some DMA system resources
    hard_assert(rc == PICO_OK);
    pico_sha256_update_blocking(&state, (const uint8_t*)__overlays_start, stop - start);

    // Get the result of the sha256 calculation
    sha256_result_t result;
    pico_sha256_finish(&state, &result);

    // Because the hash is stored as a string, we need to convert it to a byte array
    char *pos = hash;
    sha256_result_t expected;
    for (size_t count = 0; count < sizeof(expected.bytes); count++) {
        sscanf(pos, "%2hhx", &expected.bytes[count]);
        pos += 2;
    }

    if(memcmp(&expected, &result, SHA256_RESULT_BYTES) == 0) {
        printf("hash is correct\n");
        return true;
    } else {
        printf("hash is incorrect - expected:\n");
        for(int i = 0; i < SHA256_RESULT_BYTES; i++) {
            printf("0x%02x,", result.bytes[i]);
        }
        printf("\n");
        memset(__overlays_start, 0, __overlays_end - __overlays_start);
        return false;
    }
}
