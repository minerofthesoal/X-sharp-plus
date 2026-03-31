/*
 * X# Standard Library - Regex Module
 * ====================================
 * Regular expressions using POSIX regex.h.
 * 6 functions.
 */

#ifndef XS_REGEX_LIB_H
#define XS_REGEX_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_regex_compile(int argc, XsValue* args);
XsValue xs_regex_match(int argc, XsValue* args);
XsValue xs_regex_matchAll(int argc, XsValue* args);
XsValue xs_regex_replace(int argc, XsValue* args);
XsValue xs_regex_split(int argc, XsValue* args);
XsValue xs_regex_test(int argc, XsValue* args);

void xs_regex_register(VM* vm);

#endif /* XS_REGEX_LIB_H */
