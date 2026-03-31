/*
 * X# Standard Library - String Module
 * =====================================
 * 22+ string manipulation functions.
 */

#ifndef XS_STRING_LIB_H
#define XS_STRING_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_string_length(int argc, XsValue* args);
XsValue xs_string_charAt(int argc, XsValue* args);
XsValue xs_string_substring(int argc, XsValue* args);
XsValue xs_string_indexOf(int argc, XsValue* args);
XsValue xs_string_lastIndexOf(int argc, XsValue* args);
XsValue xs_string_contains(int argc, XsValue* args);
XsValue xs_string_startsWith(int argc, XsValue* args);
XsValue xs_string_endsWith(int argc, XsValue* args);
XsValue xs_string_toUpper(int argc, XsValue* args);
XsValue xs_string_toLower(int argc, XsValue* args);
XsValue xs_string_trim(int argc, XsValue* args);
XsValue xs_string_trimStart(int argc, XsValue* args);
XsValue xs_string_trimEnd(int argc, XsValue* args);
XsValue xs_string_split(int argc, XsValue* args);
XsValue xs_string_join(int argc, XsValue* args);
XsValue xs_string_replace(int argc, XsValue* args);
XsValue xs_string_replaceAll(int argc, XsValue* args);
XsValue xs_string_repeat(int argc, XsValue* args);
XsValue xs_string_reverse(int argc, XsValue* args);
XsValue xs_string_padStart(int argc, XsValue* args);
XsValue xs_string_padEnd(int argc, XsValue* args);
XsValue xs_string_format(int argc, XsValue* args);
XsValue xs_string_concat(int argc, XsValue* args);
XsValue xs_string_codePointAt(int argc, XsValue* args);
XsValue xs_string_fromCodePoint(int argc, XsValue* args);

void xs_string_register(VM* vm);

#endif /* XS_STRING_LIB_H */
