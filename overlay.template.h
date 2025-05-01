#include "overlays.h"

#include "pico/sha256.h"
#include "pico/binary_info.h"

extern uint8_t __load_start_overlay_@NAME@[];
extern uint8_t __load_stop_overlay_@NAME@[];

bi_decl(bi_ptr_string(0, 0, @NAME@_hash,"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", SHA256_RESULT_BYTES * 2 + 1));
