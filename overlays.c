#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/overlay.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "mbedtls/aes.h"

#if LIB_PICO_CYW43_ARCH
#include "pico/cyw43_arch.h"
#endif

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 5
#endif

__define_overlay_func(blink_one, blinking, (uint32_t ms), (ms), void) {
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

#define aes_tst(...) __call_overlay_func(aes_tst, pico_mbedtls, __VA_ARGS__)
void __overlay_func(aes_tst, pico_mbedtls)() {
    printf("aes_tst\n");
    mbedtls_aes_context aes;

    #define TST_BUF_SIZE 16 * 4

    uint32_t aes_key[8];
    for (int i=0; i < count_of(aes_key); i++) {
        aes_key[i] = 0x01020304;
    }

    uint8_t iv[16];
    for (int i=0; i < sizeof(iv); i++) {
        iv[i] = i;
    }

    uint8_t buf[TST_BUF_SIZE];
    for (int i=0; i < sizeof(buf); i++) {
        buf[i] = i;
    }

    printf("buf: ");
    for (int i=0; i < sizeof(buf); i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");

    int len = sizeof(buf);

    mbedtls_aes_setkey_enc(&aes, (uint8_t*)aes_key, 256);

    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;
    mbedtls_aes_crypt_ctr(&aes, len, &nc_off, iv, stream_block, buf, buf);

    printf("buf: ");
    for (int i=0; i < sizeof(buf); i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
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


#if LIB_PICO_CYW43_ARCH
static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
            result->ssid, result->rssi, result->channel,
            result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
            result->auth_mode);
    }
    return 0;
}
#endif


int main()
{
    stdio_init_all();

#if LIB_PICO_CYW43_ARCH
    pico_load_overlay(pico_cyw43_arch_lwip_poll);
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();

    absolute_time_t scan_time = nil_time;
    bool scan_in_progress = false;
#endif

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (true) {
        printf("Hello, world!\n");
        printf("overlays_start: %p\n", __overlays_start);
        printf("overlays_end: %p\n", __overlays_end);
        sleep_ms(1000);
        // printf("blink_one\n");
        // blink_one(1000);
        // printf("blink_two\n");
        // blink_two(500);
        printf("blink_three\n");
        blink_three(250);
        // printf("blink_one slower\n");
        // blink_one(2000);
        printf("fade_slow\n");
        fade_slow(2000);

    #if LIB_PICO_CYW43_ARCH
        printf("pico_cyw43_arch_lwip_poll\n");
        if (pico_load_overlay(pico_cyw43_arch_lwip_poll)) {
            if (absolute_time_diff_us(get_absolute_time(), scan_time) < 0) {
                if (!scan_in_progress) {
                    cyw43_wifi_scan_options_t scan_options = {0};
                    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
                    if (err == 0) {
                        printf("\nPerforming wifi scan\n");
                        scan_in_progress = true;
                    } else {
                        printf("Failed to start scan: %d\n", err);
                        scan_time = make_timeout_time_ms(10000); // wait 10s and scan again
                    }
                } else if (!cyw43_wifi_scan_active(&cyw43_state)) {
                    scan_time = make_timeout_time_ms(10000); // wait 10s and scan again
                    scan_in_progress = false; 
                }
            }

            cyw43_arch_poll();

            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(1000);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(1000);
        } else {
            printf("Failed to load pico_cyw43_arch_lwip_poll overlay\n");
        }
    #endif

        printf("pico_mbedtls\n");
        pico_load_overlay(pico_mbedtls);
        aes_tst();
    }
}
