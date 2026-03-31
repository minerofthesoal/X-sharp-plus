/*
 * X# Standard Library - Regex Module Implementation
 * ===================================================
 * Regular expressions using POSIX regex.h.
 */

#include "regex_lib.h"
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_GROUPS 32

XsValue xs_regex_compile(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

    int flags = REG_EXTENDED;
    if (argc >= 2 && args[1].type == VAL_SCROLL) {
        const char *opts = args[1].scroll;
        for (int i = 0; opts[i]; i++) {
            if (opts[i] == 'i') flags |= REG_ICASE;
            if (opts[i] == 'n') flags |= REG_NEWLINE;
        }
    }

    regex_t *re = (regex_t *)malloc(sizeof(regex_t));
    if (!re) return xs_abyss();

    int rc = regcomp(re, args[0].scroll, flags);
    if (rc != 0) {
        free(re);
        return xs_abyss();
    }

    XsValue val;
    val.type = VAL_ENTITY;
    val.object = re;
    return val;
}

XsValue xs_regex_match(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    if (args[0].type != VAL_ENTITY || args[1].type != VAL_SCROLL) return xs_abyss();

    regex_t *re = (regex_t *)args[0].object;
    const char *str = args[1].scroll;

    regmatch_t matches[MAX_GROUPS];
    int rc = regexec(re, str, MAX_GROUPS, matches, 0);
    if (rc != 0) return xs_abyss();

    /* Return an arsenal of matched group strings */
    XsArsenal *arr = xs_arsenal_new(MAX_GROUPS);

    for (int i = 0; i < MAX_GROUPS; i++) {
        if (matches[i].rm_so == -1) break;
        int len = matches[i].rm_eo - matches[i].rm_so;
        char *group = (char *)malloc(len + 1);
        if (group) {
            memcpy(group, str + matches[i].rm_so, len);
            group[len] = '\0';
            xs_arsenal_push(arr, xs_scroll(group));
        }
    }

    return xs_arsenal(arr);
}

XsValue xs_regex_matchAll(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    if (args[0].type != VAL_ENTITY || args[1].type != VAL_SCROLL) return xs_abyss();

    regex_t *re = (regex_t *)args[0].object;
    const char *str = args[1].scroll;

    XsArsenal *all = xs_arsenal_new(16);
    regmatch_t match[1];
    const char *cursor = str;

    while (regexec(re, cursor, 1, match, (cursor != str) ? REG_NOTBOL : 0) == 0) {
        int len = match[0].rm_eo - match[0].rm_so;
        if (len == 0) {
            /* Avoid infinite loop on zero-length match */
            cursor++;
            if (*cursor == '\0') break;
            continue;
        }
        char *m = (char *)malloc(len + 1);
        if (m) {
            memcpy(m, cursor + match[0].rm_so, len);
            m[len] = '\0';
            xs_arsenal_push(all, xs_scroll(m));
        }
        cursor += match[0].rm_eo;
        if (*cursor == '\0') break;
    }

    return xs_arsenal(all);
}

XsValue xs_regex_replace(int argc, XsValue *args) {
    if (argc < 3) return xs_abyss();
    if (args[0].type != VAL_ENTITY || args[1].type != VAL_SCROLL || args[2].type != VAL_SCROLL)
        return xs_abyss();

    regex_t *re = (regex_t *)args[0].object;
    const char *str = args[1].scroll;
    const char *replacement = args[2].scroll;
    int replace_all = (argc >= 4 && args[3].type == VAL_FATE && args[3].fate);

    size_t str_len = strlen(str);
    size_t rep_len = strlen(replacement);
    /* Allocate generous buffer */
    size_t buf_cap = str_len * 2 + rep_len * 10 + 256;
    char *result = (char *)malloc(buf_cap);
    if (!result) return xs_abyss();

    size_t out_pos = 0;
    const char *cursor = str;
    regmatch_t match[1];
    int flags = 0;

    do {
        int rc = regexec(re, cursor, 1, match, flags);
        if (rc != 0) {
            /* Copy remaining string */
            size_t remaining = strlen(cursor);
            if (out_pos + remaining >= buf_cap) {
                buf_cap = out_pos + remaining + 1;
                result = (char *)realloc(result, buf_cap);
            }
            memcpy(result + out_pos, cursor, remaining);
            out_pos += remaining;
            break;
        }

        /* Copy prefix before match */
        if (match[0].rm_so > 0) {
            if (out_pos + (size_t)match[0].rm_so >= buf_cap) {
                buf_cap = (out_pos + match[0].rm_so) * 2;
                result = (char *)realloc(result, buf_cap);
            }
            memcpy(result + out_pos, cursor, match[0].rm_so);
            out_pos += match[0].rm_so;
        }

        /* Copy replacement */
        if (out_pos + rep_len >= buf_cap) {
            buf_cap = (out_pos + rep_len) * 2;
            result = (char *)realloc(result, buf_cap);
        }
        memcpy(result + out_pos, replacement, rep_len);
        out_pos += rep_len;

        cursor += match[0].rm_eo;
        if (match[0].rm_eo == match[0].rm_so) {
            /* Zero-length match, advance one char */
            if (*cursor) {
                result[out_pos++] = *cursor++;
            } else {
                break;
            }
        }
        flags = REG_NOTBOL;
    } while (replace_all && *cursor);

    result[out_pos] = '\0';
    return xs_scroll(result);
}

XsValue xs_regex_split(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    if (args[0].type != VAL_ENTITY || args[1].type != VAL_SCROLL) return xs_abyss();

    regex_t *re = (regex_t *)args[0].object;
    const char *str = args[1].scroll;

    XsArsenal *arr = xs_arsenal_new(16);
    regmatch_t match[1];
    const char *cursor = str;
    int flags = 0;

    while (*cursor) {
        int rc = regexec(re, cursor, 1, match, flags);
        if (rc != 0) {
            xs_arsenal_push(arr, xs_scroll(xs_strdup(cursor)));
            cursor += strlen(cursor);
            break;
        }
        if (match[0].rm_so == 0 && match[0].rm_eo == 0) {
            /* Zero-length match at start, push one char */
            char *c = (char *)malloc(2);
            c[0] = *cursor;
            c[1] = '\0';
            xs_arsenal_push(arr, xs_scroll(c));
            cursor++;
            continue;
        }

        /* Push the substring before the match */
        int len = match[0].rm_so;
        char *part = (char *)malloc(len + 1);
        if (part) {
            memcpy(part, cursor, len);
            part[len] = '\0';
            xs_arsenal_push(arr, xs_scroll(part));
        }
        cursor += match[0].rm_eo;
        flags = REG_NOTBOL;
    }

    /* If the string ended exactly at a delimiter, push empty */
    if (cursor == str + strlen(str) && strlen(str) > 0) {
        /* Already handled above */
    }

    return xs_arsenal(arr);
}

XsValue xs_regex_test(int argc, XsValue *args) {
    if (argc < 2) return xs_fate(false);
    if (args[0].type != VAL_ENTITY || args[1].type != VAL_SCROLL)
        return xs_fate(false);

    regex_t *re = (regex_t *)args[0].object;
    int rc = regexec(re, args[1].scroll, 0, NULL, 0);
    return xs_fate(rc == 0);
}

/* ===== Registration ===== */

void xs_regex_register(VM *vm) {
    vm_register_native(vm, "Regex.compile",  xs_regex_compile);
    vm_register_native(vm, "Regex.match",    xs_regex_match);
    vm_register_native(vm, "Regex.matchAll", xs_regex_matchAll);
    vm_register_native(vm, "Regex.replace",  xs_regex_replace);
    vm_register_native(vm, "Regex.split",    xs_regex_split);
    vm_register_native(vm, "Regex.test",     xs_regex_test);
}
