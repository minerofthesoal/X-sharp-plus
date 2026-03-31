/*
 * X# Standard Library - Math Module Implementation
 * ==================================================
 */

#include "math_lib.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif
#define XS_TAU (2.0 * M_PI)

static int s_random_seeded = 0;

static void ensure_seeded(void) {
    if (!s_random_seeded) {
        srand((unsigned)time(NULL));
        s_random_seeded = 1;
    }
}

static double to_double(XsValue v) {
    if (v.type == VAL_SPARK) return v.spark;
    if (v.type == VAL_BLADE) return (double)v.blade;
    return 0.0;
}

/* ===== Constants ===== */

XsValue xs_math_PI(int argc, XsValue* args) {
    (void)argc; (void)args;
    return xs_spark(M_PI);
}

XsValue xs_math_E(int argc, XsValue* args) {
    (void)argc; (void)args;
    return xs_spark(M_E);
}

XsValue xs_math_TAU(int argc, XsValue* args) {
    (void)argc; (void)args;
    return xs_spark(XS_TAU);
}

XsValue xs_math_INF(int argc, XsValue* args) {
    (void)argc; (void)args;
    return xs_spark(INFINITY);
}

XsValue xs_math_NAN_VAL(int argc, XsValue* args) {
    (void)argc; (void)args;
    return xs_spark(NAN);
}

/* ===== Basic operations ===== */

XsValue xs_math_abs(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_BLADE) {
        int64_t v = args[0].blade;
        return xs_blade(v < 0 ? -v : v);
    }
    return xs_spark(fabs(to_double(args[0])));
}

XsValue xs_math_ceil(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_BLADE) return args[0];
    return xs_spark(ceil(to_double(args[0])));
}

XsValue xs_math_floor(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_BLADE) return args[0];
    return xs_spark(floor(to_double(args[0])));
}

XsValue xs_math_round(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_BLADE) return args[0];
    return xs_spark(round(to_double(args[0])));
}

XsValue xs_math_sign(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    double v = to_double(args[0]);
    if (v > 0.0) return xs_spark(1.0);
    if (v < 0.0) return xs_spark(-1.0);
    return xs_spark(0.0);
}

XsValue xs_math_fract(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    double v = to_double(args[0]);
    return xs_spark(v - floor(v));
}

XsValue xs_math_sqrt(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(sqrt(to_double(args[0])));
}

XsValue xs_math_cbrt(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(cbrt(to_double(args[0])));
}

XsValue xs_math_pow(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    /* If both are integers and exponent is non-negative, return integer */
    if (args[0].type == VAL_BLADE && args[1].type == VAL_BLADE && args[1].blade >= 0) {
        int64_t base = args[0].blade;
        int64_t exp = args[1].blade;
        int64_t result = 1;
        while (exp > 0) {
            if (exp & 1) result *= base;
            base *= base;
            exp >>= 1;
        }
        return xs_blade(result);
    }
    return xs_spark(pow(to_double(args[0]), to_double(args[1])));
}

XsValue xs_math_exp(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(exp(to_double(args[0])));
}

XsValue xs_math_log(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(log(to_double(args[0])));
}

XsValue xs_math_log2(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(log2(to_double(args[0])));
}

XsValue xs_math_log10(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(log10(to_double(args[0])));
}

/* ===== Trigonometry ===== */

XsValue xs_math_sin(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(sin(to_double(args[0])));
}

XsValue xs_math_cos(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(cos(to_double(args[0])));
}

XsValue xs_math_tan(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(tan(to_double(args[0])));
}

XsValue xs_math_asin(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(asin(to_double(args[0])));
}

XsValue xs_math_acos(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(acos(to_double(args[0])));
}

XsValue xs_math_atan(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(atan(to_double(args[0])));
}

XsValue xs_math_atan2(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    return xs_spark(atan2(to_double(args[0]), to_double(args[1])));
}

XsValue xs_math_sinh(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(sinh(to_double(args[0])));
}

XsValue xs_math_cosh(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(cosh(to_double(args[0])));
}

XsValue xs_math_tanh(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(tanh(to_double(args[0])));
}

/* ===== Min / Max / Clamp ===== */

XsValue xs_math_min(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    double a = to_double(args[0]);
    double b = to_double(args[1]);
    if (args[0].type == VAL_BLADE && args[1].type == VAL_BLADE) {
        return xs_blade(args[0].blade < args[1].blade ? args[0].blade : args[1].blade);
    }
    return xs_spark(a < b ? a : b);
}

XsValue xs_math_max(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    double a = to_double(args[0]);
    double b = to_double(args[1]);
    if (args[0].type == VAL_BLADE && args[1].type == VAL_BLADE) {
        return xs_blade(args[0].blade > args[1].blade ? args[0].blade : args[1].blade);
    }
    return xs_spark(a > b ? a : b);
}

XsValue xs_math_clamp(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    double v = to_double(args[0]);
    double lo = to_double(args[1]);
    double hi = to_double(args[2]);
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    if (args[0].type == VAL_BLADE && args[1].type == VAL_BLADE && args[2].type == VAL_BLADE) {
        int64_t iv = args[0].blade;
        if (iv < args[1].blade) iv = args[1].blade;
        if (iv > args[2].blade) iv = args[2].blade;
        return xs_blade(iv);
    }
    return xs_spark(v);
}

/* ===== Interpolation ===== */

XsValue xs_math_lerp(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    double a = to_double(args[0]);
    double b = to_double(args[1]);
    double t = to_double(args[2]);
    return xs_spark(a + (b - a) * t);
}

XsValue xs_math_inverseLerp(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    double a = to_double(args[0]);
    double b = to_double(args[1]);
    double v = to_double(args[2]);
    if (fabs(b - a) < DBL_EPSILON) return xs_spark(0.0);
    return xs_spark((v - a) / (b - a));
}

XsValue xs_math_remap(int argc, XsValue* args) {
    if (argc < 5) return xs_abyss();
    double v      = to_double(args[0]);
    double in_lo  = to_double(args[1]);
    double in_hi  = to_double(args[2]);
    double out_lo = to_double(args[3]);
    double out_hi = to_double(args[4]);
    if (fabs(in_hi - in_lo) < DBL_EPSILON) return xs_spark(out_lo);
    double t = (v - in_lo) / (in_hi - in_lo);
    return xs_spark(out_lo + (out_hi - out_lo) * t);
}

XsValue xs_math_step(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    double edge = to_double(args[0]);
    double x    = to_double(args[1]);
    return xs_spark(x < edge ? 0.0 : 1.0);
}

XsValue xs_math_smoothstep(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    double edge0 = to_double(args[0]);
    double edge1 = to_double(args[1]);
    double x     = to_double(args[2]);
    if (fabs(edge1 - edge0) < DBL_EPSILON) return xs_spark(0.0);
    double t = (x - edge0) / (edge1 - edge0);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return xs_spark(t * t * (3.0 - 2.0 * t));
}

/* ===== Random ===== */

XsValue xs_math_random(int argc, XsValue* args) {
    (void)argc; (void)args;
    ensure_seeded();
    return xs_spark((double)rand() / (double)RAND_MAX);
}

XsValue xs_math_randomRange(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    ensure_seeded();
    double lo = to_double(args[0]);
    double hi = to_double(args[1]);
    double t = (double)rand() / (double)RAND_MAX;
    return xs_spark(lo + (hi - lo) * t);
}

XsValue xs_math_randomInt(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    ensure_seeded();
    int64_t lo = (int64_t)to_double(args[0]);
    int64_t hi = (int64_t)to_double(args[1]);
    if (hi <= lo) return xs_blade(lo);
    int64_t range = hi - lo;
    return xs_blade(lo + (int64_t)(rand() % range));
}

XsValue xs_math_seedRandom(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    srand((unsigned)(int64_t)to_double(args[0]));
    s_random_seeded = 1;
    return xs_abyss();
}

/* ===== Angle conversions ===== */

XsValue xs_math_degToRad(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(to_double(args[0]) * M_PI / 180.0);
}

XsValue xs_math_radToDeg(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    return xs_spark(to_double(args[0]) * 180.0 / M_PI);
}

/* ===== Registration ===== */

void xs_math_register(VM* vm) {
    vm_register_native(vm, "Math.PI",          xs_math_PI);
    vm_register_native(vm, "Math.E",           xs_math_E);
    vm_register_native(vm, "Math.TAU",         xs_math_TAU);
    vm_register_native(vm, "Math.INF",         xs_math_INF);
    vm_register_native(vm, "Math.NAN",         xs_math_NAN_VAL);
    vm_register_native(vm, "Math.abs",         xs_math_abs);
    vm_register_native(vm, "Math.ceil",        xs_math_ceil);
    vm_register_native(vm, "Math.floor",       xs_math_floor);
    vm_register_native(vm, "Math.round",       xs_math_round);
    vm_register_native(vm, "Math.sign",        xs_math_sign);
    vm_register_native(vm, "Math.fract",       xs_math_fract);
    vm_register_native(vm, "Math.sqrt",        xs_math_sqrt);
    vm_register_native(vm, "Math.cbrt",        xs_math_cbrt);
    vm_register_native(vm, "Math.pow",         xs_math_pow);
    vm_register_native(vm, "Math.exp",         xs_math_exp);
    vm_register_native(vm, "Math.log",         xs_math_log);
    vm_register_native(vm, "Math.log2",        xs_math_log2);
    vm_register_native(vm, "Math.log10",       xs_math_log10);
    vm_register_native(vm, "Math.sin",         xs_math_sin);
    vm_register_native(vm, "Math.cos",         xs_math_cos);
    vm_register_native(vm, "Math.tan",         xs_math_tan);
    vm_register_native(vm, "Math.asin",        xs_math_asin);
    vm_register_native(vm, "Math.acos",        xs_math_acos);
    vm_register_native(vm, "Math.atan",        xs_math_atan);
    vm_register_native(vm, "Math.atan2",       xs_math_atan2);
    vm_register_native(vm, "Math.sinh",        xs_math_sinh);
    vm_register_native(vm, "Math.cosh",        xs_math_cosh);
    vm_register_native(vm, "Math.tanh",        xs_math_tanh);
    vm_register_native(vm, "Math.min",         xs_math_min);
    vm_register_native(vm, "Math.max",         xs_math_max);
    vm_register_native(vm, "Math.clamp",       xs_math_clamp);
    vm_register_native(vm, "Math.lerp",        xs_math_lerp);
    vm_register_native(vm, "Math.inverseLerp", xs_math_inverseLerp);
    vm_register_native(vm, "Math.remap",       xs_math_remap);
    vm_register_native(vm, "Math.step",        xs_math_step);
    vm_register_native(vm, "Math.smoothstep",  xs_math_smoothstep);
    vm_register_native(vm, "Math.random",      xs_math_random);
    vm_register_native(vm, "Math.randomRange", xs_math_randomRange);
    vm_register_native(vm, "Math.randomInt",   xs_math_randomInt);
    vm_register_native(vm, "Math.seedRandom",  xs_math_seedRandom);
    vm_register_native(vm, "Math.degToRad",    xs_math_degToRad);
    vm_register_native(vm, "Math.radToDeg",    xs_math_radToDeg);
}
