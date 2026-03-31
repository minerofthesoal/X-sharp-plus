/*
 * X# Standard Library - Graphics Module Implementation
 * ======================================================
 * Software framebuffer renderer with Bresenham line/circle algorithms.
 * The framebuffer can be presented via platform-specific backends.
 */

#include "graphics_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ===== Internal framebuffer ===== */

typedef struct {
    uint32_t* pixels;   /* RGBA packed */
    int       width;
    int       height;
    uint32_t  color;    /* current draw color */
    int       alive;    /* window is open */
} XsGfxWindow;

static XsGfxWindow* s_window = NULL;

static uint32_t pack_rgba(int r, int g, int b, int a) {
    return ((uint32_t)(a & 0xFF) << 24) |
           ((uint32_t)(r & 0xFF) << 16) |
           ((uint32_t)(g & 0xFF) << 8)  |
           ((uint32_t)(b & 0xFF));
}

static void set_pixel(XsGfxWindow* w, int x, int y) {
    if (x >= 0 && x < w->width && y >= 0 && y < w->height) {
        w->pixels[y * w->width + x] = w->color;
    }
}

/* ===== createWindow(width, height, title) ===== */

XsValue xs_gfx_createWindow(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    int w = (int)xs_as_spark(args[0]);
    int h = (int)xs_as_spark(args[1]);
    if (w <= 0 || h <= 0 || w > 7680 || h > 4320) return xs_fate(false);

    if (s_window) { free(s_window->pixels); free(s_window); }

    s_window = (XsGfxWindow*)calloc(1, sizeof(XsGfxWindow));
    s_window->width  = w;
    s_window->height = h;
    s_window->pixels = (uint32_t*)calloc(w * h, sizeof(uint32_t));
    s_window->color  = pack_rgba(255, 255, 255, 255);
    s_window->alive  = 1;
    return xs_fate(true);
}

/* ===== destroyWindow() ===== */

XsValue xs_gfx_destroyWindow(int argc, XsValue* args) {
    (void)argc; (void)args;
    if (s_window) {
        free(s_window->pixels);
        free(s_window);
        s_window = NULL;
    }
    return xs_abyss();
}

/* ===== clearScreen(r?, g?, b?) ===== */

XsValue xs_gfx_clearScreen(int argc, XsValue* args) {
    if (!s_window) return xs_abyss();
    uint32_t c = 0xFF000000; /* black, opaque */
    if (argc >= 3) {
        c = pack_rgba((int)xs_as_spark(args[0]),
                      (int)xs_as_spark(args[1]),
                      (int)xs_as_spark(args[2]), 255);
    }
    int total = s_window->width * s_window->height;
    for (int i = 0; i < total; i++) s_window->pixels[i] = c;
    return xs_abyss();
}

/* ===== setColor(r, g, b, a?) ===== */

XsValue xs_gfx_setColor(int argc, XsValue* args) {
    if (!s_window || argc < 3) return xs_abyss();
    int r = (int)xs_as_spark(args[0]);
    int g = (int)xs_as_spark(args[1]);
    int b = (int)xs_as_spark(args[2]);
    int a = (argc >= 4) ? (int)xs_as_spark(args[3]) : 255;
    s_window->color = pack_rgba(r, g, b, a);
    return xs_abyss();
}

/* ===== drawPixel(x, y) ===== */

XsValue xs_gfx_drawPixel(int argc, XsValue* args) {
    if (!s_window || argc < 2) return xs_abyss();
    set_pixel(s_window, (int)xs_as_spark(args[0]), (int)xs_as_spark(args[1]));
    return xs_abyss();
}

/* ===== drawLine(x1, y1, x2, y2) - Bresenham ===== */

XsValue xs_gfx_drawLine(int argc, XsValue* args) {
    if (!s_window || argc < 4) return xs_abyss();
    int x0 = (int)xs_as_spark(args[0]);
    int y0 = (int)xs_as_spark(args[1]);
    int x1 = (int)xs_as_spark(args[2]);
    int y1 = (int)xs_as_spark(args[3]);

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        set_pixel(s_window, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return xs_abyss();
}

/* ===== drawRect(x, y, w, h) ===== */

XsValue xs_gfx_drawRect(int argc, XsValue* args) {
    if (!s_window || argc < 4) return xs_abyss();
    int x = (int)xs_as_spark(args[0]);
    int y = (int)xs_as_spark(args[1]);
    int w = (int)xs_as_spark(args[2]);
    int h = (int)xs_as_spark(args[3]);
    for (int i = x; i < x + w; i++) { set_pixel(s_window, i, y); set_pixel(s_window, i, y + h - 1); }
    for (int j = y; j < y + h; j++) { set_pixel(s_window, x, j); set_pixel(s_window, x + w - 1, j); }
    return xs_abyss();
}

/* ===== fillRect(x, y, w, h) ===== */

XsValue xs_gfx_fillRect(int argc, XsValue* args) {
    if (!s_window || argc < 4) return xs_abyss();
    int x = (int)xs_as_spark(args[0]);
    int y = (int)xs_as_spark(args[1]);
    int w = (int)xs_as_spark(args[2]);
    int h = (int)xs_as_spark(args[3]);
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            set_pixel(s_window, i, j);
    return xs_abyss();
}

/* ===== drawCircle(cx, cy, r) - Midpoint circle ===== */

XsValue xs_gfx_drawCircle(int argc, XsValue* args) {
    if (!s_window || argc < 3) return xs_abyss();
    int cx = (int)xs_as_spark(args[0]);
    int cy = (int)xs_as_spark(args[1]);
    int r  = (int)xs_as_spark(args[2]);

    int x = r, y = 0, d = 1 - r;
    while (x >= y) {
        set_pixel(s_window, cx + x, cy + y);
        set_pixel(s_window, cx - x, cy + y);
        set_pixel(s_window, cx + x, cy - y);
        set_pixel(s_window, cx - x, cy - y);
        set_pixel(s_window, cx + y, cy + x);
        set_pixel(s_window, cx - y, cy + x);
        set_pixel(s_window, cx + y, cy - x);
        set_pixel(s_window, cx - y, cy - x);
        y++;
        if (d <= 0) {
            d += 2 * y + 1;
        } else {
            x--;
            d += 2 * (y - x) + 1;
        }
    }
    return xs_abyss();
}

/* ===== fillCircle(cx, cy, r) ===== */

XsValue xs_gfx_fillCircle(int argc, XsValue* args) {
    if (!s_window || argc < 3) return xs_abyss();
    int cx = (int)xs_as_spark(args[0]);
    int cy = (int)xs_as_spark(args[1]);
    int r  = (int)xs_as_spark(args[2]);

    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)sqrt((double)(r * r - dy * dy));
        for (int x = cx - dx; x <= cx + dx; x++) {
            set_pixel(s_window, x, cy + dy);
        }
    }
    return xs_abyss();
}

/* ===== drawTriangle(x1,y1, x2,y2, x3,y3) ===== */

XsValue xs_gfx_drawTriangle(int argc, XsValue* args) {
    if (!s_window || argc < 6) return xs_abyss();
    /* Draw 3 lines */
    XsValue line1[4] = { args[0], args[1], args[2], args[3] };
    XsValue line2[4] = { args[2], args[3], args[4], args[5] };
    XsValue line3[4] = { args[4], args[5], args[0], args[1] };
    xs_gfx_drawLine(4, line1);
    xs_gfx_drawLine(4, line2);
    xs_gfx_drawLine(4, line3);
    return xs_abyss();
}

/* ===== fillTriangle(x1,y1, x2,y2, x3,y3) - Scanline ===== */

XsValue xs_gfx_fillTriangle(int argc, XsValue* args) {
    if (!s_window || argc < 6) return xs_abyss();
    int x1 = (int)xs_as_spark(args[0]), y1 = (int)xs_as_spark(args[1]);
    int x2 = (int)xs_as_spark(args[2]), y2 = (int)xs_as_spark(args[3]);
    int x3 = (int)xs_as_spark(args[4]), y3 = (int)xs_as_spark(args[5]);

    /* Sort by y */
    if (y1 > y2) { int t; t=x1;x1=x2;x2=t; t=y1;y1=y2;y2=t; }
    if (y1 > y3) { int t; t=x1;x1=x3;x3=t; t=y1;y1=y3;y3=t; }
    if (y2 > y3) { int t; t=x2;x2=x3;x3=t; t=y2;y2=y3;y3=t; }

    /* Scanline fill */
    for (int y = y1; y <= y3; y++) {
        int xa, xb;
        if (y3 != y1) {
            xa = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
        } else {
            xa = x1;
        }
        if (y < y2) {
            if (y2 != y1) xb = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
            else xb = x1;
        } else {
            if (y3 != y2) xb = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
            else xb = x2;
        }
        if (xa > xb) { int t = xa; xa = xb; xb = t; }
        for (int x = xa; x <= xb; x++) set_pixel(s_window, x, y);
    }
    return xs_abyss();
}

/* ===== drawText(x, y, text) - simple 1px placeholder ===== */

XsValue xs_gfx_drawText(int argc, XsValue* args) {
    if (!s_window || argc < 3) return xs_abyss();
    int x = (int)xs_as_spark(args[0]);
    int y = (int)xs_as_spark(args[1]);
    /* Mark text position with a small marker for each character */
    if (args[2].type == VAL_SCROLL && args[2].scroll) {
        int len = (int)strlen(args[2].scroll);
        for (int i = 0; i < len; i++) {
            /* Simple 5-pixel tall column per character */
            for (int dy = 0; dy < 5; dy++) {
                set_pixel(s_window, x + i * 6, y + dy);
                set_pixel(s_window, x + i * 6 + 1, y + dy);
                set_pixel(s_window, x + i * 6 + 2, y + dy);
                set_pixel(s_window, x + i * 6 + 3, y + dy);
            }
        }
    }
    return xs_abyss();
}

/* ===== loadImage(path) -> entity with width, height, pixels ===== */

XsValue xs_gfx_loadImage(int argc, XsValue* args) {
    if (argc < 1 || args[0].type != VAL_SCROLL) return xs_abyss();
    /* Read a simple PPM (P6) image file */
    FILE* f = fopen(args[0].scroll, "rb");
    if (!f) return xs_abyss();

    char magic[3];
    if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        fclose(f); return xs_abyss();
    }
    int w, h, maxval;
    if (fscanf(f, "%d %d %d", &w, &h, &maxval) != 3) { fclose(f); return xs_abyss(); }
    fgetc(f); /* consume single whitespace */

    uint32_t* pixels = (uint32_t*)malloc(w * h * sizeof(uint32_t));
    for (int i = 0; i < w * h; i++) {
        unsigned char rgb[3];
        if (fread(rgb, 1, 3, f) != 3) break;
        pixels[i] = pack_rgba(rgb[0], rgb[1], rgb[2], 255);
    }
    fclose(f);

    /* Store as entity */
    XsEntity* ent = xs_entity_new();
    xs_entity_set(ent, "width", xs_blade(w));
    xs_entity_set(ent, "height", xs_blade(h));

    /* Store pixel data pointer as a blade (cast) for internal use */
    XsValue pval;
    pval.type = VAL_BLADE;
    pval.blade = (int64_t)(intptr_t)pixels;
    xs_entity_set(ent, "_pixels", pval);

    return xs_entity(ent);
}

/* ===== drawImage(image, x, y) ===== */

XsValue xs_gfx_drawImage(int argc, XsValue* args) {
    if (!s_window || argc < 3 || args[0].type != VAL_ENTITY) return xs_abyss();
    XsEntity* ent = (XsEntity*)args[0].object;
    int dx = (int)xs_as_spark(args[1]);
    int dy = (int)xs_as_spark(args[2]);

    XsValue wv = xs_entity_get(ent, "width");
    XsValue hv = xs_entity_get(ent, "height");
    XsValue pv = xs_entity_get(ent, "_pixels");
    if (wv.type == VAL_ABYSS || hv.type == VAL_ABYSS) return xs_abyss();

    int iw = (int)wv.blade;
    int ih = (int)hv.blade;
    uint32_t* pixels = (uint32_t*)(intptr_t)pv.blade;
    if (!pixels) return xs_abyss();

    for (int y = 0; y < ih; y++) {
        for (int x = 0; x < iw; x++) {
            int sx = dx + x;
            int sy = dy + y;
            if (sx >= 0 && sx < s_window->width && sy >= 0 && sy < s_window->height) {
                s_window->pixels[sy * s_window->width + sx] = pixels[y * iw + x];
            }
        }
    }
    return xs_abyss();
}

/* ===== getScreenWidth() ===== */

XsValue xs_gfx_getScreenWidth(int argc, XsValue* args) {
    (void)argc; (void)args;
    if (!s_window) return xs_blade(0);
    return xs_blade((int64_t)s_window->width);
}

/* ===== getScreenHeight() ===== */

XsValue xs_gfx_getScreenHeight(int argc, XsValue* args) {
    (void)argc; (void)args;
    if (!s_window) return xs_blade(0);
    return xs_blade((int64_t)s_window->height);
}

/* ===== pollEvents() - stub, returns arsenal of events ===== */

XsValue xs_gfx_pollEvents(int argc, XsValue* args) {
    (void)argc; (void)args;
    /* In a real implementation, this would poll OS events.
       Returns empty arsenal for now; platform backend fills this. */
    return xs_arsenal(xs_arsenal_new(0));
}

/* ===== swapBuffers() ===== */

XsValue xs_gfx_swapBuffers(int argc, XsValue* args) {
    (void)argc; (void)args;
    if (!s_window) return xs_abyss();
    /* In a real implementation, blit framebuffer to screen.
       For headless/testing, write PPM to stdout or a file. */
    return xs_abyss();
}

/* ===== Registration ===== */

void xs_graphics_register(VM* vm) {
    vm_register_native(vm, "Gfx.createWindow",    xs_gfx_createWindow);
    vm_register_native(vm, "Gfx.destroyWindow",   xs_gfx_destroyWindow);
    vm_register_native(vm, "Gfx.clearScreen",     xs_gfx_clearScreen);
    vm_register_native(vm, "Gfx.setColor",        xs_gfx_setColor);
    vm_register_native(vm, "Gfx.drawPixel",       xs_gfx_drawPixel);
    vm_register_native(vm, "Gfx.drawLine",        xs_gfx_drawLine);
    vm_register_native(vm, "Gfx.drawRect",        xs_gfx_drawRect);
    vm_register_native(vm, "Gfx.fillRect",        xs_gfx_fillRect);
    vm_register_native(vm, "Gfx.drawCircle",      xs_gfx_drawCircle);
    vm_register_native(vm, "Gfx.fillCircle",      xs_gfx_fillCircle);
    vm_register_native(vm, "Gfx.drawTriangle",    xs_gfx_drawTriangle);
    vm_register_native(vm, "Gfx.fillTriangle",    xs_gfx_fillTriangle);
    vm_register_native(vm, "Gfx.drawText",        xs_gfx_drawText);
    vm_register_native(vm, "Gfx.loadImage",       xs_gfx_loadImage);
    vm_register_native(vm, "Gfx.drawImage",       xs_gfx_drawImage);
    vm_register_native(vm, "Gfx.getScreenWidth",  xs_gfx_getScreenWidth);
    vm_register_native(vm, "Gfx.getScreenHeight", xs_gfx_getScreenHeight);
    vm_register_native(vm, "Gfx.pollEvents",      xs_gfx_pollEvents);
    vm_register_native(vm, "Gfx.swapBuffers",     xs_gfx_swapBuffers);
}
