/*
 * X# Standard Library - System Module
 * =====================================
 * 12 system functions: exec, env, info, etc.
 */

#ifndef XS_SYSTEM_LIB_H
#define XS_SYSTEM_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_sys_exec(int argc, XsValue* args);
XsValue xs_sys_getEnv(int argc, XsValue* args);
XsValue xs_sys_setEnv(int argc, XsValue* args);
XsValue xs_sys_exit(int argc, XsValue* args);
XsValue xs_sys_sleep(int argc, XsValue* args);
XsValue xs_sys_time(int argc, XsValue* args);
XsValue xs_sys_clock(int argc, XsValue* args);
XsValue xs_sys_platform(int argc, XsValue* args);
XsValue xs_sys_arch(int argc, XsValue* args);
XsValue xs_sys_cpuCount(int argc, XsValue* args);
XsValue xs_sys_memoryUsage(int argc, XsValue* args);
XsValue xs_sys_pid(int argc, XsValue* args);

void xs_system_register(VM* vm);

#endif /* XS_SYSTEM_LIB_H */
