#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"


void __attribute__((noinline,section(".blink_one"))) blink_one() {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    sleep_ms(1000);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    sleep_ms(1000);
}

void __attribute__((noinline,section(".blink_two"))) blink_two() {
    for (int i=0; i < 2; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(200);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(200);
    }
}


extern uint8_t __load_start_blink_one[];
extern uint8_t __load_stop_blink_one[];
extern uint8_t __load_start_blink_two[];
extern uint8_t __load_stop_blink_two[];
extern uint8_t __overlays_start[];
extern uint8_t __overlays_end[];


void overlay_load(uint8_t* start, uint8_t* stop) {
    memcpy(__overlays_start + 4096, start, stop - start);
}


#define OVERLAY_FUNC(name) overlay_load(__load_start_##name, __load_stop_##name); name();


int main()
{
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
        printf("blink_one\n");
        OVERLAY_FUNC(blink_one);
        printf("blink_two\n");
        OVERLAY_FUNC(blink_two);
    }
}
