/*
 * X# Standard Library - Filesystem Module
 * =========================================
 * Extended filesystem operations: watch, glob, path manipulation.
 * 10 functions.
 */

#ifndef XS_FS_LIB_H
#define XS_FS_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_fs_watch(int argc, XsValue* args);
XsValue xs_fs_glob(int argc, XsValue* args);
XsValue xs_fs_realpath(int argc, XsValue* args);
XsValue xs_fs_basename(int argc, XsValue* args);
XsValue xs_fs_dirname(int argc, XsValue* args);
XsValue xs_fs_extname(int argc, XsValue* args);
XsValue xs_fs_joinPath(int argc, XsValue* args);
XsValue xs_fs_isAbsolute(int argc, XsValue* args);
XsValue xs_fs_tempDir(int argc, XsValue* args);
XsValue xs_fs_tempFile(int argc, XsValue* args);

void xs_fs_register(VM* vm);

#endif /* XS_FS_LIB_H */
