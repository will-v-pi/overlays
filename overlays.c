#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/sha256.h"


void __attribute__((noinline,section(".blink_one"))) blink_one(uint32_t ms) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    sleep_ms(ms);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    sleep_ms(ms);
}

void __attribute__((noinline,section(".blink_two"))) blink_two(uint32_t ms) {
    for (int i=0; i < 2; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(ms);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(ms);
    }
}


extern uint8_t __load_start_blink_one[];
extern uint8_t __load_stop_blink_one[];
extern uint8_t __load_start_blink_two[];
extern uint8_t __load_stop_blink_two[];
extern uint8_t __overlays_start[];
extern uint8_t __overlays_end[];


#define P99_PROTECT(...) __VA_ARGS__


bi_decl(bi_ptr_string(0, 0, blink_one_hash, P99_PROTECT({0xb4,0x04,0x2d,0x78,0x66,0xf9,0x9b,0x30,0x7a,0xe7,0x74,0x0f,0x34,0x5c,0x2c,0x20,0x96,0x7e,0xb4,0xa3,0x0e,0xc9,0xfd,0x54,0xe6,0xfe,0x10,0x4c,0xff,0xee,0x88,0xa6,}), SHA256_RESULT_BYTES + 1));
bi_decl(bi_ptr_string(0, 0, blink_two_hash, P99_PROTECT({0x73,0xb6,0xaa,0xc7,0x6c,0x27,0x0b,0xb0,0xa3,0xbc,0x2b,0x75,0xc8,0x6a,0x95,0x05,0x23,0x5c,0xed,0x7b,0xca,0x6a,0x7e,0x78,0x8d,0xa7,0x7d,0x1a,0x3e,0x4a,0xf2,0x06}), SHA256_RESULT_BYTES + 1));


bool overlay_load(uint8_t* start, uint8_t* stop, char* hash) {
    printf("loading code from %p to %p\n", start, __overlays_start);
    memcpy(__overlays_start, start, stop - start);
    memset(__overlays_start + (stop - start), 0, __overlays_end - __overlays_start - (stop - start));

    printf("checking hash\n");
    pico_sha256_state_t state;
    int rc = pico_sha256_start_blocking(&state, SHA256_BIG_ENDIAN, true); // using some DMA system resources
    hard_assert(rc == PICO_OK);
    pico_sha256_update_blocking(&state, (const uint8_t*)__overlays_start, __overlays_end - __overlays_start);

    // Get the result of the sha256 calculation
    sha256_result_t result;
    pico_sha256_finish(&state, &result);

    if(memcmp(hash, &result, SHA256_RESULT_BYTES) == 0) {
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


#define OVERLAY_FUNC(name, ...) if (overlay_load(__load_start_##name, __load_stop_##name, name##_hash)) { name(__VA_ARGS__); } else { printf("overlay failed to load\n"); }


int main()
{
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        printf("Hello, world!\n");
        printf("overlays_start: %p\n", __overlays_start);
        printf("overlays_end: %p\n", __overlays_end);
        sleep_ms(1000);
        printf("blink_one\n");
        OVERLAY_FUNC(blink_one, 1000);
        printf("blink_two\n");
        OVERLAY_FUNC(blink_two, 500);
        printf("blink_one slower\n");
        OVERLAY_FUNC(blink_one, 2000);
    }
}
