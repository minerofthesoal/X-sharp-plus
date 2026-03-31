/*
 * X# Standard Library - Filesystem Module Implementation
 * ========================================================
 * Extended filesystem operations using POSIX APIs.
 */

#include "fs_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <libgen.h>
#include <glob.h>

#ifdef __linux__
#include <sys/inotify.h>
#include <unistd.h>
#endif

XsValue xs_fs_watch(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

#ifdef __linux__
    int fd = inotify_init();
    if (fd < 0) return xs_blade(-1);

    int wd = inotify_add_watch(fd, args[0].scroll,
        IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd < 0) {
        close(fd);
        return xs_blade(-1);
    }

    /* Return the inotify file descriptor as a blade for later reading */
    /* Store both fd and wd packed: fd in upper 32 bits, wd in lower */
    int64_t handle = ((int64_t)fd << 32) | (int64_t)(unsigned int)wd;
    return xs_blade(handle);
#else
    /* Stub on non-Linux platforms */
    (void)args;
    return xs_blade(-1);
#endif
}

XsValue xs_fs_glob(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

    glob_t results;
    int rc = glob(args[0].scroll, GLOB_NOSORT | GLOB_TILDE, NULL, &results);
    if (rc != 0) {
        globfree(&results);
        return xs_arsenal(xs_arsenal_new(0));
    }

    XsArsenal *arr = xs_arsenal_new((int)results.gl_pathc);
    for (size_t i = 0; i < results.gl_pathc; i++) {
        xs_arsenal_push(arr, xs_scroll(xs_strdup(results.gl_pathv[i])));
    }

    globfree(&results);
    return xs_arsenal(arr);
}

XsValue xs_fs_realpath(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

    char resolved[PATH_MAX];
    char *result = realpath(args[0].scroll, resolved);
    if (!result) return xs_abyss();

    return xs_scroll(xs_strdup(resolved));
}

XsValue xs_fs_basename(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

    /* basename may modify its argument, so duplicate first */
    char *dup = xs_strdup(args[0].scroll);
    char *base = basename(dup);
    char *result = xs_strdup(base);
    free(dup);
    return xs_scroll(result);
}

XsValue xs_fs_dirname(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

    char *dup = xs_strdup(args[0].scroll);
    char *dir = dirname(dup);
    char *result = xs_strdup(dir);
    free(dup);
    return xs_scroll(result);
}

XsValue xs_fs_extname(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();

    const char *path = args[0].scroll;
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return xs_scroll(xs_strdup(""));

    /* Make sure dot is after last slash */
    const char *slash = strrchr(path, '/');
    if (slash && dot < slash) return xs_scroll(xs_strdup(""));

    return xs_scroll(xs_strdup(dot));
}

XsValue xs_fs_joinPath(int argc, XsValue *args) {
    if (argc < 2) return xs_abyss();
    if (args[0].type != VAL_SCROLL || args[1].type != VAL_SCROLL) return xs_abyss();

    const char *a = args[0].scroll;
    const char *b = args[1].scroll;

    size_t alen = strlen(a);
    size_t blen = strlen(b);
    size_t need = alen + blen + 2; /* '/' + '\0' */

    char *result = (char *)malloc(need);
    if (!result) return xs_abyss();

    if (alen > 0 && a[alen - 1] == '/') {
        snprintf(result, need, "%s%s", a, b);
    } else {
        snprintf(result, need, "%s/%s", a, b);
    }

    return xs_scroll(result);
}

XsValue xs_fs_isAbsolute(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_fate(false);
    return xs_fate(args[0].scroll[0] == '/');
}

XsValue xs_fs_tempDir(int argc, XsValue *args) {
    (void)argc; (void)args;
    const char *tmp = getenv("TMPDIR");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = getenv("TEMP");
    if (!tmp) tmp = "/tmp";
    return xs_scroll(xs_strdup(tmp));
}

XsValue xs_fs_tempFile(int argc, XsValue *args) {
    (void)argc; (void)args;
    const char *tmp = getenv("TMPDIR");
    if (!tmp) tmp = "/tmp";

    /* Optional prefix from args[0] */
    const char *prefix = "xs_tmp_";
    if (argc >= 1 && args[0].type == VAL_SCROLL) {
        prefix = args[0].scroll;
    }

    char template[PATH_MAX];
    snprintf(template, sizeof(template), "%s/%sXXXXXX", tmp, prefix);

    int fd = mkstemp(template);
    if (fd < 0) return xs_abyss();
    close(fd);

    return xs_scroll(xs_strdup(template));
}

/* ===== Registration ===== */

void xs_fs_register(VM *vm) {
    vm_register_native(vm, "FS.watch",      xs_fs_watch);
    vm_register_native(vm, "FS.glob",       xs_fs_glob);
    vm_register_native(vm, "FS.realpath",   xs_fs_realpath);
    vm_register_native(vm, "FS.basename",   xs_fs_basename);
    vm_register_native(vm, "FS.dirname",    xs_fs_dirname);
    vm_register_native(vm, "FS.extname",    xs_fs_extname);
    vm_register_native(vm, "FS.joinPath",   xs_fs_joinPath);
    vm_register_native(vm, "FS.isAbsolute", xs_fs_isAbsolute);
    vm_register_native(vm, "FS.tempDir",    xs_fs_tempDir);
    vm_register_native(vm, "FS.tempFile",   xs_fs_tempFile);
}
