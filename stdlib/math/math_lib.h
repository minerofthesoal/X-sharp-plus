/*
 * X# Standard Library - Math Module
 * ===================================
 * 35+ math functions: trigonometry, rounding, interpolation, random, etc.
 */

#ifndef XS_MATH_LIB_H
#define XS_MATH_LIB_H

#include "../../src/runtime/runtime.h"

/* Constants exposed as native functions */
XsValue xs_math_PI(int argc, XsValue* args);
XsValue xs_math_E(int argc, XsValue* args);
XsValue xs_math_TAU(int argc, XsValue* args);
XsValue xs_math_INF(int argc, XsValue* args);
XsValue xs_math_NAN_VAL(int argc, XsValue* args);

/* Basic operations */
XsValue xs_math_abs(int argc, XsValue* args);
XsValue xs_math_ceil(int argc, XsValue* args);
XsValue xs_math_floor(int argc, XsValue* args);
XsValue xs_math_round(int argc, XsValue* args);
XsValue xs_math_sign(int argc, XsValue* args);
XsValue xs_math_fract(int argc, XsValue* args);
XsValue xs_math_sqrt(int argc, XsValue* args);
XsValue xs_math_cbrt(int argc, XsValue* args);
XsValue xs_math_pow(int argc, XsValue* args);
XsValue xs_math_exp(int argc, XsValue* args);
XsValue xs_math_log(int argc, XsValue* args);
XsValue xs_math_log2(int argc, XsValue* args);
XsValue xs_math_log10(int argc, XsValue* args);

/* Trigonometry */
XsValue xs_math_sin(int argc, XsValue* args);
XsValue xs_math_cos(int argc, XsValue* args);
XsValue xs_math_tan(int argc, XsValue* args);
XsValue xs_math_asin(int argc, XsValue* args);
XsValue xs_math_acos(int argc, XsValue* args);
XsValue xs_math_atan(int argc, XsValue* args);
XsValue xs_math_atan2(int argc, XsValue* args);
XsValue xs_math_sinh(int argc, XsValue* args);
XsValue xs_math_cosh(int argc, XsValue* args);
XsValue xs_math_tanh(int argc, XsValue* args);

/* Min / Max / Clamp */
XsValue xs_math_min(int argc, XsValue* args);
XsValue xs_math_max(int argc, XsValue* args);
XsValue xs_math_clamp(int argc, XsValue* args);

/* Interpolation */
XsValue xs_math_lerp(int argc, XsValue* args);
XsValue xs_math_inverseLerp(int argc, XsValue* args);
XsValue xs_math_remap(int argc, XsValue* args);
XsValue xs_math_step(int argc, XsValue* args);
XsValue xs_math_smoothstep(int argc, XsValue* args);

/* Random */
XsValue xs_math_random(int argc, XsValue* args);
XsValue xs_math_randomRange(int argc, XsValue* args);
XsValue xs_math_randomInt(int argc, XsValue* args);
XsValue xs_math_seedRandom(int argc, XsValue* args);

/* Angle conversions */
XsValue xs_math_degToRad(int argc, XsValue* args);
XsValue xs_math_radToDeg(int argc, XsValue* args);

/* Register all math functions */
void xs_math_register(VM* vm);

#endif /* XS_MATH_LIB_H */
