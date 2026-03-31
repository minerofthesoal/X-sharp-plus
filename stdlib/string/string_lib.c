/*
 * X# Standard Library - String Module Implementation
 * ====================================================
 */

#include "string_lib.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

/* ===== Helpers ===== */

static char* safe_str(XsValue v) {
    if (v.type == VAL_SCROLL && v.scroll) return v.scroll;
    return "";
}

static int safe_len(XsValue v) {
    if (v.type == VAL_SCROLL && v.scroll) return (int)strlen(v.scroll);
    return 0;
}

/* ===== length ===== */

XsValue xs_string_length(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_blade(0);
    return xs_blade((int64_t)strlen(safe_str(args[0])));
}

/* ===== charAt ===== */

XsValue xs_string_charAt(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* s = safe_str(args[0]);
    int idx = (int)xs_as_spark(args[1]);
    int len = (int)strlen(s);
    if (idx < 0 || idx >= len) return xs_abyss();
    char* result = (char*)malloc(2);
    result[0] = s[idx];
    result[1] = '\0';
    return xs_scroll(result);
}

/* ===== substring ===== */

XsValue xs_string_substring(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* s = safe_str(args[0]);
    int len = (int)strlen(s);
    int start = (int)xs_as_spark(args[1]);
    int end = (argc >= 3) ? (int)xs_as_spark(args[2]) : len;
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start > end) { int t = start; start = end; end = t; }
    int rlen = end - start;
    char* result = (char*)malloc(rlen + 1);
    memcpy(result, s + start, rlen);
    result[rlen] = '\0';
    return xs_scroll(result);
}

/* ===== indexOf ===== */

XsValue xs_string_indexOf(int argc, XsValue* args) {
    if (argc < 2) return xs_blade(-1);
    char* haystack = safe_str(args[0]);
    char* needle = safe_str(args[1]);
    int from = (argc >= 3) ? (int)xs_as_spark(args[2]) : 0;
    int hlen = (int)strlen(haystack);
    if (from < 0) from = 0;
    if (from >= hlen) return xs_blade(-1);
    char* found = strstr(haystack + from, needle);
    if (!found) return xs_blade(-1);
    return xs_blade((int64_t)(found - haystack));
}

/* ===== lastIndexOf ===== */

XsValue xs_string_lastIndexOf(int argc, XsValue* args) {
    if (argc < 2) return xs_blade(-1);
    char* haystack = safe_str(args[0]);
    char* needle = safe_str(args[1]);
    int hlen = (int)strlen(haystack);
    int nlen = (int)strlen(needle);
    if (nlen == 0) return xs_blade((int64_t)hlen);
    if (nlen > hlen) return xs_blade(-1);
    for (int i = hlen - nlen; i >= 0; i--) {
        if (strncmp(haystack + i, needle, nlen) == 0) {
            return xs_blade((int64_t)i);
        }
    }
    return xs_blade(-1);
}

/* ===== contains ===== */

XsValue xs_string_contains(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    char* haystack = safe_str(args[0]);
    char* needle = safe_str(args[1]);
    return xs_fate(strstr(haystack, needle) != NULL);
}

/* ===== startsWith ===== */

XsValue xs_string_startsWith(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    char* s = safe_str(args[0]);
    char* prefix = safe_str(args[1]);
    size_t plen = strlen(prefix);
    return xs_fate(strncmp(s, prefix, plen) == 0);
}

/* ===== endsWith ===== */

XsValue xs_string_endsWith(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    char* s = safe_str(args[0]);
    char* suffix = safe_str(args[1]);
    size_t slen = strlen(s);
    size_t xlen = strlen(suffix);
    if (xlen > slen) return xs_fate(false);
    return xs_fate(strcmp(s + slen - xlen, suffix) == 0);
}

/* ===== toUpper ===== */

XsValue xs_string_toUpper(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    char* s = safe_str(args[0]);
    size_t len = strlen(s);
    char* result = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) result[i] = (char)toupper((unsigned char)s[i]);
    result[len] = '\0';
    return xs_scroll(result);
}

/* ===== toLower ===== */

XsValue xs_string_toLower(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    char* s = safe_str(args[0]);
    size_t len = strlen(s);
    char* result = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) result[i] = (char)tolower((unsigned char)s[i]);
    result[len] = '\0';
    return xs_scroll(result);
}

/* ===== trim ===== */

XsValue xs_string_trim(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    char* s = safe_str(args[0]);
    while (*s && isspace((unsigned char)*s)) s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    char* result = (char*)malloc(len + 1);
    memcpy(result, s, len);
    result[len] = '\0';
    return xs_scroll(result);
}

/* ===== trimStart ===== */

XsValue xs_string_trimStart(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    char* s = safe_str(args[0]);
    while (*s && isspace((unsigned char)*s)) s++;
    char* result = xs_strdup(s);
    return xs_scroll(result);
}

/* ===== trimEnd ===== */

XsValue xs_string_trimEnd(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    char* s = safe_str(args[0]);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    char* result = (char*)malloc(len + 1);
    memcpy(result, s, len);
    result[len] = '\0';
    return xs_scroll(result);
}

/* ===== split ===== */

XsValue xs_string_split(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* s = safe_str(args[0]);
    char* delim = safe_str(args[1]);
    XsArsenal* arr = xs_arsenal_new(8);
    size_t dlen = strlen(delim);

    if (dlen == 0) {
        /* Split into individual characters */
        size_t slen = strlen(s);
        for (size_t i = 0; i < slen; i++) {
            char* ch = (char*)malloc(2);
            ch[0] = s[i];
            ch[1] = '\0';
            xs_arsenal_push(arr, xs_scroll(ch));
        }
    } else {
        char* copy = xs_strdup(s);
        char* cur = copy;
        char* found;
        while ((found = strstr(cur, delim)) != NULL) {
            size_t seg = found - cur;
            char* piece = (char*)malloc(seg + 1);
            memcpy(piece, cur, seg);
            piece[seg] = '\0';
            xs_arsenal_push(arr, xs_scroll(piece));
            cur = found + dlen;
        }
        /* Remainder */
        xs_arsenal_push(arr, xs_scroll(xs_strdup(cur)));
        free(copy);
    }
    return xs_arsenal(arr);
}

/* ===== join ===== */

XsValue xs_string_join(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_ARSENAL) return xs_abyss();
    XsArsenal* arr = (XsArsenal*)args[0].object;
    char* sep = safe_str(args[1]);
    size_t seplen = strlen(sep);

    if (arr->count == 0) return xs_scroll(xs_strdup(""));

    /* Calculate total length */
    size_t total = 0;
    for (int i = 0; i < arr->count; i++) {
        XsValue item = xs_arsenal_get(arr, i);
        if (item.type == VAL_SCROLL) total += strlen(item.scroll);
        if (i > 0) total += seplen;
    }

    char* result = (char*)malloc(total + 1);
    char* p = result;
    for (int i = 0; i < arr->count; i++) {
        if (i > 0) { memcpy(p, sep, seplen); p += seplen; }
        XsValue item = xs_arsenal_get(arr, i);
        if (item.type == VAL_SCROLL) {
            size_t l = strlen(item.scroll);
            memcpy(p, item.scroll, l);
            p += l;
        }
    }
    *p = '\0';
    return xs_scroll(result);
}

/* ===== replace (first occurrence) ===== */

XsValue xs_string_replace(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    char* s = safe_str(args[0]);
    char* old = safe_str(args[1]);
    char* rep = safe_str(args[2]);
    size_t oldlen = strlen(old);
    size_t replen = strlen(rep);

    char* found = strstr(s, old);
    if (!found) return xs_scroll(xs_strdup(s));

    size_t slen = strlen(s);
    size_t newlen = slen - oldlen + replen;
    char* result = (char*)malloc(newlen + 1);
    size_t before = found - s;
    memcpy(result, s, before);
    memcpy(result + before, rep, replen);
    memcpy(result + before + replen, found + oldlen, slen - before - oldlen);
    result[newlen] = '\0';
    return xs_scroll(result);
}

/* ===== replaceAll ===== */

XsValue xs_string_replaceAll(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    char* s = safe_str(args[0]);
    char* old = safe_str(args[1]);
    char* rep = safe_str(args[2]);
    size_t oldlen = strlen(old);
    size_t replen = strlen(rep);

    if (oldlen == 0) return xs_scroll(xs_strdup(s));

    /* Count occurrences */
    int count = 0;
    char* p = s;
    while ((p = strstr(p, old)) != NULL) { count++; p += oldlen; }

    size_t slen = strlen(s);
    size_t newlen = slen + (replen - oldlen) * count;
    char* result = (char*)malloc(newlen + 1);
    char* dst = result;
    p = s;
    char* found;
    while ((found = strstr(p, old)) != NULL) {
        size_t seg = found - p;
        memcpy(dst, p, seg);
        dst += seg;
        memcpy(dst, rep, replen);
        dst += replen;
        p = found + oldlen;
    }
    strcpy(dst, p);
    return xs_scroll(result);
}

/* ===== repeat ===== */

XsValue xs_string_repeat(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* s = safe_str(args[0]);
    int n = (int)xs_as_spark(args[1]);
    if (n <= 0) return xs_scroll(xs_strdup(""));
    size_t slen = strlen(s);
    size_t total = slen * n;
    char* result = (char*)malloc(total + 1);
    for (int i = 0; i < n; i++) {
        memcpy(result + i * slen, s, slen);
    }
    result[total] = '\0';
    return xs_scroll(result);
}

/* ===== reverse ===== */

XsValue xs_string_reverse(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    char* s = safe_str(args[0]);
    size_t len = strlen(s);
    char* result = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) result[i] = s[len - 1 - i];
    result[len] = '\0';
    return xs_scroll(result);
}

/* ===== padStart ===== */

XsValue xs_string_padStart(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* s = safe_str(args[0]);
    int target = (int)xs_as_spark(args[1]);
    char* fill = (argc >= 3 && args[2].type == VAL_SCROLL) ? args[2].scroll : " ";
    int slen = (int)strlen(s);
    if (slen >= target) return xs_scroll(xs_strdup(s));
    int pad = target - slen;
    int flen = (int)strlen(fill);
    char* result = (char*)malloc(target + 1);
    for (int i = 0; i < pad; i++) result[i] = fill[i % flen];
    memcpy(result + pad, s, slen);
    result[target] = '\0';
    return xs_scroll(result);
}

/* ===== padEnd ===== */

XsValue xs_string_padEnd(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* s = safe_str(args[0]);
    int target = (int)xs_as_spark(args[1]);
    char* fill = (argc >= 3 && args[2].type == VAL_SCROLL) ? args[2].scroll : " ";
    int slen = (int)strlen(s);
    if (slen >= target) return xs_scroll(xs_strdup(s));
    int pad = target - slen;
    int flen = (int)strlen(fill);
    char* result = (char*)malloc(target + 1);
    memcpy(result, s, slen);
    for (int i = 0; i < pad; i++) result[slen + i] = fill[i % flen];
    result[target] = '\0';
    return xs_scroll(result);
}

/* ===== format (sprintf-style) ===== */

XsValue xs_string_format(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    char* fmt = args[0].scroll;
    size_t flen = strlen(fmt);

    /* Build output by scanning format string for {} placeholders */
    size_t cap = flen * 2 + 256;
    char* result = (char*)malloc(cap);
    size_t rpos = 0;
    int argIdx = 1;

    for (size_t i = 0; i < flen; i++) {
        if (fmt[i] == '{' && i + 1 < flen && fmt[i + 1] == '}') {
            /* Replace {} with next arg */
            char buf[128];
            if (argIdx < argc) {
                XsValue v = args[argIdx++];
                switch (v.type) {
                    case VAL_BLADE:  snprintf(buf, sizeof(buf), "%lld", (long long)v.blade); break;
                    case VAL_SPARK:  snprintf(buf, sizeof(buf), "%g", v.spark); break;
                    case VAL_SCROLL: snprintf(buf, sizeof(buf), "%s", v.scroll ? v.scroll : "null"); break;
                    case VAL_FATE:   snprintf(buf, sizeof(buf), "%s", v.fate ? "true" : "false"); break;
                    case VAL_ABYSS:  snprintf(buf, sizeof(buf), "null"); break;
                    default:         snprintf(buf, sizeof(buf), "<object>"); break;
                }
            } else {
                snprintf(buf, sizeof(buf), "{}");
            }
            size_t blen = strlen(buf);
            while (rpos + blen + 1 >= cap) { cap *= 2; result = (char*)realloc(result, cap); }
            memcpy(result + rpos, buf, blen);
            rpos += blen;
            i++; /* skip } */
        } else {
            if (rpos + 2 >= cap) { cap *= 2; result = (char*)realloc(result, cap); }
            result[rpos++] = fmt[i];
        }
    }
    result[rpos] = '\0';
    return xs_scroll(result);
}

/* ===== concat ===== */

XsValue xs_string_concat(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* a = safe_str(args[0]);
    char* b = safe_str(args[1]);
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    char* result = (char*)malloc(alen + blen + 1);
    memcpy(result, a, alen);
    memcpy(result + alen, b, blen);
    result[alen + blen] = '\0';
    return xs_scroll(result);
}

/* ===== codePointAt ===== */

XsValue xs_string_codePointAt(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    char* s = safe_str(args[0]);
    int idx = (int)xs_as_spark(args[1]);
    int len = (int)strlen(s);
    if (idx < 0 || idx >= len) return xs_abyss();
    return xs_blade((int64_t)(unsigned char)s[idx]);
}

/* ===== fromCodePoint ===== */

XsValue xs_string_fromCodePoint(int argc, XsValue* args) {
    if (argc < 1) return xs_abyss();
    int cp = (int)xs_as_spark(args[0]);
    if (cp < 0 || cp > 127) return xs_abyss();
    char* result = (char*)malloc(2);
    result[0] = (char)cp;
    result[1] = '\0';
    return xs_scroll(result);
}

/* ===== Registration ===== */

void xs_string_register(VM* vm) {
    vm_register_native(vm, "String.length",        xs_string_length);
    vm_register_native(vm, "String.charAt",        xs_string_charAt);
    vm_register_native(vm, "String.substring",     xs_string_substring);
    vm_register_native(vm, "String.indexOf",       xs_string_indexOf);
    vm_register_native(vm, "String.lastIndexOf",   xs_string_lastIndexOf);
    vm_register_native(vm, "String.contains",      xs_string_contains);
    vm_register_native(vm, "String.startsWith",    xs_string_startsWith);
    vm_register_native(vm, "String.endsWith",      xs_string_endsWith);
    vm_register_native(vm, "String.toUpper",       xs_string_toUpper);
    vm_register_native(vm, "String.toLower",       xs_string_toLower);
    vm_register_native(vm, "String.trim",          xs_string_trim);
    vm_register_native(vm, "String.trimStart",     xs_string_trimStart);
    vm_register_native(vm, "String.trimEnd",       xs_string_trimEnd);
    vm_register_native(vm, "String.split",         xs_string_split);
    vm_register_native(vm, "String.join",          xs_string_join);
    vm_register_native(vm, "String.replace",       xs_string_replace);
    vm_register_native(vm, "String.replaceAll",    xs_string_replaceAll);
    vm_register_native(vm, "String.repeat",        xs_string_repeat);
    vm_register_native(vm, "String.reverse",       xs_string_reverse);
    vm_register_native(vm, "String.padStart",      xs_string_padStart);
    vm_register_native(vm, "String.padEnd",        xs_string_padEnd);
    vm_register_native(vm, "String.format",        xs_string_format);
    vm_register_native(vm, "String.concat",        xs_string_concat);
    vm_register_native(vm, "String.codePointAt",   xs_string_codePointAt);
    vm_register_native(vm, "String.fromCodePoint", xs_string_fromCodePoint);
}
