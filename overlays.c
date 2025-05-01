#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/overlay.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"


#define blink_one(...) __call_overlay_func(blink_one, blinking, __VA_ARGS__)
void __overlay_func(blink_one, blinking)(uint32_t ms) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    sleep_ms(ms);
    gpio_put(PICO_DEFAULT_LED_PIN, false);
    sleep_ms(ms);
}

#define blink_two(...) __call_overlay_func(blink_two, blinking, __VA_ARGS__)
void __overlay_func(blink_two, blinking)(uint32_t ms) {
    for (int i=0; i < 2; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(ms);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(ms);
    }
}

#define blink_three(...) __call_overlay_func(blink_three, blinking, __VA_ARGS__)
void __overlay_func(blink_three, blinking)(uint32_t ms) {
    for (int i=0; i < 3; i++) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(ms);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(ms);
    }
}

#define on_pwm_wrap(...) __call_overlay_func(on_pwm_wrap, fading, __VA_ARGS__)
void __overlay_func(on_pwm_wrap, fading)() {
    static int fade = 0;
    static bool going_up = true;
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN));

    if (going_up) {
        ++fade;
        if (fade > 255) {
            fade = 255;
            going_up = false;
        }
    } else {
        --fade;
        if (fade < 0) {
            fade = 0;
            going_up = true;
        }
    }
    // Square the fade value to make the LED's brightness appear more linear
    // Note this range matches with the wrap value
    pwm_set_gpio_level(PICO_DEFAULT_LED_PIN, fade * fade);
}

// Never put the actual interrupt handler in the overlay - it will break if the overlay is changed
void on_pwm_wrap_irq() {
    on_pwm_wrap();
}

#define fade_slow(...) __call_overlay_func(fade_slow, fading, __VA_ARGS__)
void __overlay_func(fade_slow, fading)(uint32_t ms) {
    // Tell the LED pin that the PWM is in charge of its value.
    gpio_set_function(PICO_DEFAULT_LED_PIN, GPIO_FUNC_PWM);
    // Figure out which slice we just connected to the LED pin
    uint slice_num = pwm_gpio_to_slice_num(PICO_DEFAULT_LED_PIN);

    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_DEFAULT_IRQ_NUM(), on_pwm_wrap_irq);
    irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);

    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 4.f);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

    sleep_ms(ms);

    irq_remove_handler(PWM_DEFAULT_IRQ_NUM(), on_pwm_wrap_irq);
    pwm_set_irq_enabled(slice_num, false);
    pwm_clear_irq(slice_num);

    gpio_set_function(PICO_DEFAULT_LED_PIN, GPIO_FUNC_SIO);
}


// Large data sections to demonstrate the actual use of overlays
pico_default_asm(
    ".section .fading.rodata\n"
    "fading_data:\n"
    ".incbin \"large_random_bin.bin\"\n"
    ".section .blinking.rodata\n"
    "blinking_data:\n"
    ".incbin \"large_random_bin.bin\"\n"
);


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
        printf("fade_slow\n");
        fade_slow(5000);
    }
}
