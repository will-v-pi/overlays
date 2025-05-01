
#ifndef _OVERLAYS_H
#define _OVERLAYS_H

#include "pico/binary_info.h"
#include "pico/sha256.h"

#include "all_overlays.h"

#define __call_overlay_func(name, overlay, ...) if (overlay_load(__load_start_overlay_##overlay, __load_stop_overlay_##overlay, overlay##_hash)) { name##_internal(__VA_ARGS__); } else { printf("overlay failed to load\n"); }
#define __overlay_func(name, overlay) __noinline __attribute__((section("." __STRING(overlay) "." __STRING(name)))) name##_internal

extern uint8_t __overlays_start[];
extern uint8_t __overlays_end[];

bool overlay_load(uint8_t* start, uint8_t* stop, char* hash);

#endif
