/*
 * X# Standard Library - IO Module Implementation
 * =================================================
 */

#include "io_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

/* ===== Helpers ===== */

static void print_value(XsValue v) {
    switch (v.type) {
        case VAL_BLADE:  printf("%lld", (long long)v.blade); break;
        case VAL_SPARK:  printf("%g", v.spark); break;
        case VAL_SCROLL: printf("%s", v.scroll ? v.scroll : "null"); break;
        case VAL_FATE:   printf("%s", v.fate ? "true" : "false"); break;
        case VAL_ABYSS:  printf("null"); break;
        case VAL_ARSENAL: {
            XsArsenal* arr = (XsArsenal*)v.object;
            printf("[");
            if (arr) {
                for (int i = 0; i < arr->count; i++) {
                    if (i > 0) printf(", ");
                    print_value(arr->items[i]);
                }
            }
            printf("]");
            break;
        }
        case VAL_ENTITY:    printf("<Entity>"); break;
        case VAL_SPELL:     printf("<Spell>"); break;
        case VAL_NATIVE_FN: printf("<NativeFn>"); break;
    }
}

/* ===== engrave (print without newline) ===== */

XsValue xs_io_engrave(int argc, XsValue* args) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        print_value(args[i]);
    }
    fflush(stdout);
    return xs_abyss();
}

/* ===== engraveLn (print with newline) ===== */

XsValue xs_io_engraveLn(int argc, XsValue* args) {
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        print_value(args[i]);
    }
    printf("\n");
    fflush(stdout);
    return xs_abyss();
}

/* ===== readLine ===== */

XsValue xs_io_readLine(int argc, XsValue* args) {
    /* Optional prompt */
    if (argc >= 1 && args[0].type == VAL_SCROLL) {
        printf("%s", args[0].scroll);
        fflush(stdout);
    }
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) return xs_abyss();
    /* Strip trailing newline */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
    if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
    return xs_scroll(xs_strdup(buf));
}

/* ===== readChar ===== */

XsValue xs_io_readChar(int argc, XsValue* args) {
    (void)argc; (void)args;
    int c = fgetc(stdin);
    if (c == EOF) return xs_abyss();
    char* s = (char*)malloc(2);
    s[0] = (char)c;
    s[1] = '\0';
    return xs_scroll(s);
}

/* ===== readFile ===== */

XsValue xs_io_readFile(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    FILE* f = fopen(args[0].scroll, "rb");
    if (!f) return xs_abyss();
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(size + 1);
    if (!buf) { fclose(f); return xs_abyss(); }
    size_t read = fread(buf, 1, size, f);
    buf[read] = '\0';
    fclose(f);
    return xs_scroll(buf);
}

/* ===== writeFile ===== */

XsValue xs_io_writeFile(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL)
        return xs_fate(false);
    FILE* f = fopen(args[0].scroll, "wb");
    if (!f) return xs_fate(false);
    size_t len = strlen(args[1].scroll);
    size_t written = fwrite(args[1].scroll, 1, len, f);
    fclose(f);
    return xs_fate(written == len);
}

/* ===== appendFile ===== */

XsValue xs_io_appendFile(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL)
        return xs_fate(false);
    FILE* f = fopen(args[0].scroll, "ab");
    if (!f) return xs_fate(false);
    size_t len = strlen(args[1].scroll);
    size_t written = fwrite(args[1].scroll, 1, len, f);
    fclose(f);
    return xs_fate(written == len);
}

/* ===== fileExists ===== */

XsValue xs_io_fileExists(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_fate(false);
    struct stat st;
    return xs_fate(stat(args[0].scroll, &st) == 0);
}

/* ===== deleteFile ===== */

XsValue xs_io_deleteFile(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_fate(false);
    return xs_fate(remove(args[0].scroll) == 0);
}

/* ===== copyFile ===== */

XsValue xs_io_copyFile(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL)
        return xs_fate(false);
    FILE* src = fopen(args[0].scroll, "rb");
    if (!src) return xs_fate(false);
    FILE* dst = fopen(args[1].scroll, "wb");
    if (!dst) { fclose(src); return xs_fate(false); }
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            fclose(src); fclose(dst);
            return xs_fate(false);
        }
    }
    fclose(src);
    fclose(dst);
    return xs_fate(true);
}

/* ===== moveFile ===== */

XsValue xs_io_moveFile(int argc, XsValue* args) {
    if (argc < 2 || args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL)
        return xs_fate(false);
    return xs_fate(rename(args[0].scroll, args[1].scroll) == 0);
}

/* ===== fileSize ===== */

XsValue xs_io_fileSize(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_blade(-1);
    struct stat st;
    if (stat(args[0].scroll, &st) != 0) return xs_blade(-1);
    return xs_blade((int64_t)st.st_size);
}

/* ===== listDir ===== */

XsValue xs_io_listDir(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    DIR* d = opendir(args[0].scroll);
    if (!d) return xs_abyss();
    XsArsenal* arr = xs_arsenal_new(16);
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        xs_arsenal_push(arr, xs_scroll(xs_strdup(entry->d_name)));
    }
    closedir(d);
    return xs_arsenal(arr);
}

/* ===== mkdir ===== */

XsValue xs_io_mkdir(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_fate(false);
    return xs_fate(mkdir(args[0].scroll, 0755) == 0);
}

/* ===== rmdir ===== */

XsValue xs_io_rmdir(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_fate(false);
    return xs_fate(rmdir(args[0].scroll) == 0);
}

/* ===== cwd ===== */

XsValue xs_io_cwd(int argc, XsValue* args) {
    (void)argc; (void)args;
    char buf[4096];
    if (!getcwd(buf, sizeof(buf))) return xs_abyss();
    return xs_scroll(xs_strdup(buf));
}

/* ===== chdir ===== */

XsValue xs_io_chdir(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_fate(false);
    return xs_fate(chdir(args[0].scroll) == 0);
}

/* ===== isDir ===== */

XsValue xs_io_isDir(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_fate(false);
    struct stat st;
    if (stat(args[0].scroll, &st) != 0) return xs_fate(false);
    return xs_fate(S_ISDIR(st.st_mode));
}

/* ===== Registration ===== */

void xs_io_register(VM* vm) {
    vm_register_native(vm, "engrave",       xs_io_engrave);
    vm_register_native(vm, "engraveLn",     xs_io_engraveLn);
    vm_register_native(vm, "IO.engrave",    xs_io_engrave);
    vm_register_native(vm, "IO.engraveLn",  xs_io_engraveLn);
    vm_register_native(vm, "IO.readLine",   xs_io_readLine);
    vm_register_native(vm, "IO.readChar",   xs_io_readChar);
    vm_register_native(vm, "IO.readFile",   xs_io_readFile);
    vm_register_native(vm, "IO.writeFile",  xs_io_writeFile);
    vm_register_native(vm, "IO.appendFile", xs_io_appendFile);
    vm_register_native(vm, "IO.fileExists", xs_io_fileExists);
    vm_register_native(vm, "IO.deleteFile", xs_io_deleteFile);
    vm_register_native(vm, "IO.copyFile",   xs_io_copyFile);
    vm_register_native(vm, "IO.moveFile",   xs_io_moveFile);
    vm_register_native(vm, "IO.fileSize",   xs_io_fileSize);
    vm_register_native(vm, "IO.listDir",    xs_io_listDir);
    vm_register_native(vm, "IO.mkdir",      xs_io_mkdir);
    vm_register_native(vm, "IO.rmdir",      xs_io_rmdir);
    vm_register_native(vm, "IO.cwd",        xs_io_cwd);
    vm_register_native(vm, "IO.chdir",      xs_io_chdir);
    vm_register_native(vm, "IO.isDir",      xs_io_isDir);
}
