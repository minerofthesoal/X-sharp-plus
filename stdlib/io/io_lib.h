/*
 * X# Standard Library - IO Module
 * =================================
 * 18 I/O functions: print, file ops, directory ops.
 */

#ifndef XS_IO_LIB_H
#define XS_IO_LIB_H

#include "../../src/runtime/runtime.h"

/* Print */
XsValue xs_io_engrave(int argc, XsValue* args);
XsValue xs_io_engraveLn(int argc, XsValue* args);

/* Input */
XsValue xs_io_readLine(int argc, XsValue* args);
XsValue xs_io_readChar(int argc, XsValue* args);

/* File I/O */
XsValue xs_io_readFile(int argc, XsValue* args);
XsValue xs_io_writeFile(int argc, XsValue* args);
XsValue xs_io_appendFile(int argc, XsValue* args);
XsValue xs_io_fileExists(int argc, XsValue* args);
XsValue xs_io_deleteFile(int argc, XsValue* args);
XsValue xs_io_copyFile(int argc, XsValue* args);
XsValue xs_io_moveFile(int argc, XsValue* args);
XsValue xs_io_fileSize(int argc, XsValue* args);

/* Directory */
XsValue xs_io_listDir(int argc, XsValue* args);
XsValue xs_io_mkdir(int argc, XsValue* args);
XsValue xs_io_rmdir(int argc, XsValue* args);
XsValue xs_io_cwd(int argc, XsValue* args);
XsValue xs_io_chdir(int argc, XsValue* args);
XsValue xs_io_isDir(int argc, XsValue* args);

void xs_io_register(VM* vm);

#endif /* XS_IO_LIB_H */
