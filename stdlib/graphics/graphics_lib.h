/*
 * X# Standard Library - Graphics Module
 * =======================================
 * 19 graphics functions: window, drawing primitives, images.
 * Uses a framebuffer abstraction; actual rendering backend is separate.
 */

#ifndef XS_GRAPHICS_LIB_H
#define XS_GRAPHICS_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_gfx_createWindow(int argc, XsValue* args);
XsValue xs_gfx_destroyWindow(int argc, XsValue* args);
XsValue xs_gfx_clearScreen(int argc, XsValue* args);
XsValue xs_gfx_setColor(int argc, XsValue* args);
XsValue xs_gfx_drawPixel(int argc, XsValue* args);
XsValue xs_gfx_drawLine(int argc, XsValue* args);
XsValue xs_gfx_drawRect(int argc, XsValue* args);
XsValue xs_gfx_fillRect(int argc, XsValue* args);
XsValue xs_gfx_drawCircle(int argc, XsValue* args);
XsValue xs_gfx_fillCircle(int argc, XsValue* args);
XsValue xs_gfx_drawTriangle(int argc, XsValue* args);
XsValue xs_gfx_fillTriangle(int argc, XsValue* args);
XsValue xs_gfx_drawText(int argc, XsValue* args);
XsValue xs_gfx_loadImage(int argc, XsValue* args);
XsValue xs_gfx_drawImage(int argc, XsValue* args);
XsValue xs_gfx_getScreenWidth(int argc, XsValue* args);
XsValue xs_gfx_getScreenHeight(int argc, XsValue* args);
XsValue xs_gfx_pollEvents(int argc, XsValue* args);
XsValue xs_gfx_swapBuffers(int argc, XsValue* args);

void xs_graphics_register(VM* vm);

#endif /* XS_GRAPHICS_LIB_H */
