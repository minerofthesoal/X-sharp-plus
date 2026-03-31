/*
 * X# Standard Library - Time Module Implementation
 * ==================================================
 * Time operations using POSIX APIs.
 */

#include "time_lib.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define _POSIX_C_SOURCE 200809L

/* ===== Timer storage ===== */

#define MAX_TIMERS 64

static struct timespec s_timers[MAX_TIMERS];
static int s_timer_active[MAX_TIMERS] = {0};

static int alloc_timer(void) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!s_timer_active[i]) {
            s_timer_active[i] = 1;
            return i;
        }
    }
    return -1;
}

/* ===== Time functions ===== */

XsValue xs_time_now(int argc, XsValue *args) {
    (void)argc; (void)args;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    double now_ms = (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
    return xs_spark(now_ms);
}

XsValue xs_time_timestamp(int argc, XsValue *args) {
    (void)argc; (void)args;
    return xs_blade((int64_t)time(NULL));
}

XsValue xs_time_format(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    /* args[0] = timestamp (blade), args[1] = format string (scroll) */
    if (args[0].type != VAL_BLADE && args[0].type != VAL_SPARK) return xs_abyss();
    if (args[1].type != VAL_SCROLL) return xs_abyss();

    time_t t;
    if (args[0].type == VAL_BLADE) {
        t = (time_t)args[0].blade;
    } else {
        t = (time_t)args[0].spark;
    }

    struct tm *tm_info = localtime(&t);
    if (!tm_info) return xs_abyss();

    char buf[512];
    size_t len = strftime(buf, sizeof(buf), args[1].scroll, tm_info);
    if (len == 0) return xs_abyss();

    return xs_scroll(xs_strdup(buf));
}

XsValue xs_time_parse(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    if (args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL) return xs_abyss();

    struct tm tm_info;
    memset(&tm_info, 0, sizeof(tm_info));

    char *rest = strptime(args[0].scroll, args[1].scroll, &tm_info);
    if (!rest) return xs_abyss();

    time_t t = mktime(&tm_info);
    return xs_blade((int64_t)t);
}

XsValue xs_time_addDuration(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    /* args[0] = timestamp (blade), args[1] = seconds to add (blade or spark) */
    int64_t ts = 0;
    if (args[0].type == VAL_BLADE) ts = args[0].blade;
    else if (args[0].type == VAL_SPARK) ts = (int64_t)args[0].spark;
    else return xs_abyss();

    double dur = 0.0;
    if (args[1].type == VAL_BLADE) dur = (double)args[1].blade;
    else if (args[1].type == VAL_SPARK) dur = args[1].spark;
    else return xs_abyss();

    return xs_blade(ts + (int64_t)dur);
}

XsValue xs_time_diffTime(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    time_t t1 = 0, t2 = 0;
    if (args[0].type == VAL_BLADE) t1 = (time_t)args[0].blade;
    else if (args[0].type == VAL_SPARK) t1 = (time_t)args[0].spark;
    if (args[1].type == VAL_BLADE) t2 = (time_t)args[1].blade;
    else if (args[1].type == VAL_SPARK) t2 = (time_t)args[1].spark;

    return xs_spark(difftime(t1, t2));
}

XsValue xs_time_startTimer(int argc, XsValue *args) {
    (void)argc; (void)args;
    int id = alloc_timer();
    if (id < 0) return xs_blade(-1);
    clock_gettime(CLOCK_MONOTONIC, &s_timers[id]);
    return xs_blade(id);
}

XsValue xs_time_stopTimer(int argc, XsValue *args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type != VAL_BLADE) return xs_abyss();
    int id = (int)args[0].blade;
    if (id < 0 || id >= MAX_TIMERS || !s_timer_active[id]) return xs_abyss();

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (double)(end.tv_sec - s_timers[id].tv_sec) * 1000.0
                   + (double)(end.tv_nsec - s_timers[id].tv_nsec) / 1000000.0;
    s_timer_active[id] = 0;
    return xs_spark(elapsed);
}

XsValue xs_time_elapsed(int argc, XsValue *args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type != VAL_BLADE) return xs_abyss();
    int id = (int)args[0].blade;
    if (id < 0 || id >= MAX_TIMERS || !s_timer_active[id]) return xs_abyss();

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed = (double)(now.tv_sec - s_timers[id].tv_sec) * 1000.0
                   + (double)(now.tv_nsec - s_timers[id].tv_nsec) / 1000000.0;
    return xs_spark(elapsed);
}

/* ===== Registration ===== */

void xs_time_register(VM *vm) {
    vm_register_native(vm, "Time.now",         xs_time_now);
    vm_register_native(vm, "Time.timestamp",   xs_time_timestamp);
    vm_register_native(vm, "Time.format",      xs_time_format);
    vm_register_native(vm, "Time.parse",       xs_time_parse);
    vm_register_native(vm, "Time.addDuration", xs_time_addDuration);
    vm_register_native(vm, "Time.diffTime",    xs_time_diffTime);
    vm_register_native(vm, "Time.startTimer",  xs_time_startTimer);
    vm_register_native(vm, "Time.stopTimer",   xs_time_stopTimer);
    vm_register_native(vm, "Time.elapsed",     xs_time_elapsed);
}
