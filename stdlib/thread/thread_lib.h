/*
 * X# Standard Library - Thread Module
 * =====================================
 * Threading primitives: spawn, join, mutex, channels, atomics.
 * 11 functions.
 */

#ifndef XS_THREAD_LIB_H
#define XS_THREAD_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_thread_spawn(int argc, XsValue* args);
XsValue xs_thread_join(int argc, XsValue* args);
XsValue xs_thread_detach(int argc, XsValue* args);
XsValue xs_thread_mutex_new(int argc, XsValue* args);
XsValue xs_thread_mutex_lock(int argc, XsValue* args);
XsValue xs_thread_mutex_unlock(int argc, XsValue* args);
XsValue xs_thread_channel_new(int argc, XsValue* args);
XsValue xs_thread_channel_send(int argc, XsValue* args);
XsValue xs_thread_channel_recv(int argc, XsValue* args);
XsValue xs_thread_atomic_inc(int argc, XsValue* args);
XsValue xs_thread_atomic_dec(int argc, XsValue* args);

void xs_thread_register(VM* vm);

#endif /* XS_THREAD_LIB_H */
