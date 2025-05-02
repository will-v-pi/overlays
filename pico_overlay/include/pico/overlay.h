
#ifndef _OVERLAYS_H
#define _OVERLAYS_H

#include "pico/binary_info.h"
#include "pico/sha256.h"

#include "all_overlays.h"

bool overlay_load(uint8_t* start, uint8_t* stop, char* hash);

// Load an overlay - returns true if the overlay was loaded successfully
#define pico_load_overlay(overlay) overlay_load(__load_start_overlay_##overlay, __load_stop_overlay_##overlay, overlay##_hash)

// Usual usage:
// #define my_function(...) __call_overlay_func(my_function, my_overlay, __VA_ARGS__)
// void __overlay_func(my_function, my_overlay)(uint arg1, ...) {
//     ...
// }
#define __call_overlay_func(name, overlay, ...) \
    if (pico_load_overlay(overlay)) { \
        name##_internal(__VA_ARGS__); \
    } else { \
        printf("overlay failed to load\n"); \
    }
#define __overlay_func(name, overlay) __noinline __attribute__((section("." __STRING(overlay) "." __STRING(name)))) name##_internal

// Alternative usage:
// __define_overlay_func(my_function, my_overlay, (uint arg1, ...), (arg1, ...), void) {
//     ...
// }
#define __define_overlay_func(name, overlay, args, args_notype, ret) \
ret __overlay_func(name, overlay) args; \
static inline ret name args { \
    if (pico_load_overlay(overlay)) { \
        name##_internal args_notype; \
    } else { \
        printf("overlay failed to load\n"); \
    } \
} \
ret name##_internal args 

extern uint8_t __overlays_start[];
extern uint8_t __overlays_end[];

#endif
