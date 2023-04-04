#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <type_traits>
#include <iostream>

#if 0
// Class for bfloat16 emulation
class BFLOAT16 {
    int16_t value;

public:
    BFLOAT16() : value(0) {}

    inline BFLOAT16(float f32) {
        union {float f32; int16_t bf16[2]; int32_t d;} u;
        u.f32 = f32;

        if ( ( ( u.d & 0x7fffffff ) == 0x7f800000 ) || ( ( u.d & 0x7fC00000 ) == 0x7fC00000 ) ) {
            // inf or qnan
        } else if ( ( u.d & 0x7f800000 ) == 0x7f800000 ) {
            // snan
            u.d |= 0x00400000;
        } else if ( ( u.d & 0x7f800000 ) == 0 ) {
            // zero, denormal
            u.d &= 0x80000000;
        } else {
            // negative and all other numbers
            int lsb = (u.d >> 16) & 1;
            int rounding_bias = 0x7fff + lsb;
            u.d += rounding_bias;
        }
        value = u.bf16[1];
    }

    inline operator float() const {
        union { float f32; int16_t bf16[2]; } u;
        u.bf16[1] = value;
        u.bf16[0] = 0;
        return u.f32;
    }
};
#endif
