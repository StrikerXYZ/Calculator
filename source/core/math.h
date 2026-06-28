#pragma once

#include "types.h"

// Constants
#define MATH_PI         3.14159265358979323846
#define MATH_TAU        6.28318530717958647692   // 2*PI
#define MATH_PI_OVER_2  1.57079632679489661923   // PI/2
#define MATH_PI_OVER_4  0.78539816339744830962   // PI/4
#define MATH_E          2.71828182845904523536   // Euler's number
#define MATH_SQRT2      1.41421356237309504880
#define MATH_INV_SQRT2  0.70710678118654752440   // 1/sqrt(2)

#define MATH_PI_OVER_2_HI  1.5707963267948966e+00
#define MATH_PI_OVER_2_LO  6.123233995736766e-17
#define MATH_2_OVER_PI     0.6366197723675814

#define MATH_LN2        0.6931471805599453094172321
#define MATH_LOG2E      1.4426950408889634073599247 // 1/ln(2)
#define MATH_LN10       2.3025850929940456840179915
#define MATH_INV_LN10   0.4342944819032518276511289  // 1/ln(10)
#define MATH_SQRT3      1.7320508075688772935274463
#define MATH_TAN_PI8    0.4142135623730950488016887 // tan(Pi/8) = sqrt(2) - 1

// -----------------------------------------------------------------------
// Limit below which F8 has a fractional part.
// A F8 has 52 mantissa bits. At 2^52, the ULP (unit in the last place)
// is exactly 1.0 - meaning there is no representable value between
// adjacent integers. Values at or beyond this magnitude ARE integers.
// Floor/Ceil/Trunc must return x unchanged for these values.
// -----------------------------------------------------------------------
#define MATH_F8_MANTISSA_LIMIT 4503599627370496.0   // 2^52 Mantissa bits

typedef struct
{
	F8 half_turn; // 2 = 360 degrees
} Turn;

U8 math_u8_min(U8 a, U8 b);
U8 math_u8_max(U8 a, U8 b);
F8 math_f8_min(F8 a, F8 b);
F8 math_f8_max(F8 a, F8 b);

U1 math_is_finite(F8 x);
U1 math_is_equal(F8 x, F8 y, F8 epsilon);

F8 math_abs(F8 x);
F4 math_F4_abs(F4 x);

F8 math_trunc(F8 x);
F8 math_floor(F8 x);
F8 math_ceil(F8 x);
F8 math_round(F8 x);
F8 math_fmod(F8 x, F8 y);

F8 math_sqrt(F8 x);
F4 math_F4_sqrt(F4 x);
F8 math_rsqrt(F8 x);
F4 math_F4_rsqrt(F4 x);

// Trig functions - input in radians
// Accurate to ~13 decimal places for |x| < 3.3e6 (Cody-Waite reduction)
F8 math_sin_radian(F8 x);
F8 math_cos_radian(F8 x);
F8 math_tan_radian(F8 x);
F8 math_sin(Turn turns);
F8 math_cos(Turn turns);
F8 math_tan(Turn turns);

F8 math_ln(F8 x); // loge(x), x > 0
F8 math_exp(F8 x); // e ^ x, 
F8 math_log(F8 x); // log10(x), x > 0
F8 math_pow(F8 x, F8 y); // F8 math_exp2(F8 x); // 2 ^ x

F8 math_asin(F8 x);
F8 math_acos(F8 x);
F8 math_atan(F8 x);
