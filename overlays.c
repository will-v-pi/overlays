#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/overlay.h"


#define blink_one(...) __call_overlay_func(blink_one, __VA_ARGS__)
void __overlay_func(blink_one, blink_one)(uint32_t ms) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    sleep_ms(ms);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    sleep_ms(ms);
}

#define blink_two(...) __call_overlay_func(blink_two, __VA_ARGS__)
void __overlay_func(blink_two, blink_two)(uint32_t ms) {
    for (int i=0; i < 2; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(ms);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(ms);
    }
}

#define blink_three(...) __call_overlay_func(blink_three, __VA_ARGS__)
void __overlay_func(blink_three, blink_three)(uint32_t ms) {
    for (int i=0; i < 3; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(ms);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(ms);
    }
}


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
        blink_one(1000);
        printf("blink_two\n");
        blink_two(500);
        printf("blink_three\n");
        blink_three(250);
        printf("blink_one slower\n");
        blink_one(2000);
    }
}
