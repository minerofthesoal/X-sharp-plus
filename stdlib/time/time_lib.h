/*
 * X# Standard Library - Time Module
 * ===================================
 * Time operations: now, format, parse, duration math, timers.
 * 8 functions.
 */

#ifndef XS_TIME_LIB_H
#define XS_TIME_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_time_now(int argc, XsValue* args);
XsValue xs_time_timestamp(int argc, XsValue* args);
XsValue xs_time_format(int argc, XsValue* args);
XsValue xs_time_parse(int argc, XsValue* args);
XsValue xs_time_addDuration(int argc, XsValue* args);
XsValue xs_time_diffTime(int argc, XsValue* args);
XsValue xs_time_startTimer(int argc, XsValue* args);
XsValue xs_time_stopTimer(int argc, XsValue* args);
XsValue xs_time_elapsed(int argc, XsValue* args);

void xs_time_register(VM* vm);

#endif /* XS_TIME_LIB_H */
