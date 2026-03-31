/*
 * X# Standard Library - System Module Implementation
 * =====================================================
 */

#include "system_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>

/* ===== exec(command) -> scroll (stdout output) ===== */

XsValue xs_sys_exec(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    FILE* p = popen(args[0].scroll, "r");
    if (!p) return xs_abyss();

    size_t cap = 4096, len = 0;
    char* buf = (char*)malloc(cap);
    size_t n;
    while ((n = fread(buf + len, 1, cap - len - 1, p)) > 0) {
        len += n;
        if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
    }
    buf[len] = '\0';
    int status = pclose(p);
    (void)status;
    return xs_scroll(buf);
}

/* ===== getEnv(name) -> scroll or abyss ===== */

XsValue xs_sys_getEnv(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    const char* val = getenv(args[0].scroll);
    if (!val) return xs_abyss();
    return xs_scroll(xs_strdup(val));
}

/* ===== setEnv(name, value) ===== */

XsValue xs_sys_setEnv(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL)
        return xs_fate(false);
    return xs_fate(setenv(args[0].scroll, args[1].scroll, 1) == 0);
}

/* ===== exit(code?) ===== */

XsValue xs_sys_exit(int argc, XsValue* args) {
    int code = (argc >= 1) ? (int)xs_as_spark(args[0]) : 0;
    exit(code);
    return xs_abyss(); /* unreachable */
}

/* ===== sleep(ms) ===== */

XsValue xs_sys_sleep(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int ms = (int)xs_as_spark(args[0]);
    if (ms <= 0) return xs_abyss();
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
    return xs_abyss();
}

/* ===== time() -> spark (seconds since epoch as float) ===== */

XsValue xs_sys_time(int argc, XsValue* args) {
    (void)argc; (void)args;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return xs_spark((double)ts.tv_sec + (double)ts.tv_nsec / 1e9);
}

/* ===== clock() -> spark (monotonic high-res seconds) ===== */

XsValue xs_sys_clock(int argc, XsValue* args) {
    (void)argc; (void)args;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return xs_spark((double)ts.tv_sec + (double)ts.tv_nsec / 1e9);
}

/* ===== platform() -> scroll ===== */

XsValue xs_sys_platform(int argc, XsValue* args) {
    (void)argc; (void)args;
    struct utsname u;
    if (uname(&u) != 0) return xs_scroll(xs_strdup("unknown"));
    return xs_scroll(xs_strdup(u.sysname));
}

/* ===== arch() -> scroll ===== */

XsValue xs_sys_arch(int argc, XsValue* args) {
    (void)argc; (void)args;
    struct utsname u;
    if (uname(&u) != 0) return xs_scroll(xs_strdup("unknown"));
    return xs_scroll(xs_strdup(u.machine));
}

/* ===== cpuCount() -> blade ===== */

XsValue xs_sys_cpuCount(int argc, XsValue* args) {
    (void)argc; (void)args;
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return xs_blade(n > 0 ? (int64_t)n : 1);
}

/* ===== memoryUsage() -> blade (RSS in bytes) ===== */

XsValue xs_sys_memoryUsage(int argc, XsValue* args) {
    (void)argc; (void)args;
    FILE* f = fopen("/proc/self/statm", "r");
    if (!f) return xs_blade(0);
    long pages = 0, resident = 0;
    if (fscanf(f, "%ld %ld", &pages, &resident) < 2) { fclose(f); return xs_blade(0); }
    fclose(f);
    long page_size = sysconf(_SC_PAGESIZE);
    return xs_blade((int64_t)(resident * page_size));
}

/* ===== pid() -> blade ===== */

XsValue xs_sys_pid(int argc, XsValue* args) {
    (void)argc; (void)args;
    return xs_blade((int64_t)getpid());
}

/* ===== Registration ===== */

void xs_system_register(VM* vm) {
    vm_register_native(vm, "System.exec",        xs_sys_exec);
    vm_register_native(vm, "System.getEnv",      xs_sys_getEnv);
    vm_register_native(vm, "System.setEnv",      xs_sys_setEnv);
    vm_register_native(vm, "System.exit",        xs_sys_exit);
    vm_register_native(vm, "System.sleep",       xs_sys_sleep);
    vm_register_native(vm, "System.time",        xs_sys_time);
    vm_register_native(vm, "System.clock",       xs_sys_clock);
    vm_register_native(vm, "System.platform",    xs_sys_platform);
    vm_register_native(vm, "System.arch",        xs_sys_arch);
    vm_register_native(vm, "System.cpuCount",    xs_sys_cpuCount);
    vm_register_native(vm, "System.memoryUsage", xs_sys_memoryUsage);
    vm_register_native(vm, "System.pid",         xs_sys_pid);
}
