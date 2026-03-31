#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <float.h>

/* ================================================================== */
/*  Framebuffer                                                        */
/* ================================================================== */

Framebuffer *framebuffer_create(int width, int height) {
    Framebuffer *fb = calloc(1, sizeof(Framebuffer));
    if (!fb) return NULL;
    fb->width  = width;
    fb->height = height;
    fb->color  = calloc((size_t)width * height, sizeof(uint32_t));
    fb->depth  = malloc((size_t)width * height * sizeof(float));
    if (!fb->color || !fb->depth) {
        free(fb->color);
        free(fb->depth);
        free(fb);
        return NULL;
    }
    /* Clear depth to 1.0 (far) */
    for (int i = 0; i < width * height; i++)
        fb->depth[i] = 1.0f;
    return fb;
}

void framebuffer_destroy(Framebuffer *fb) {
    if (!fb) return;
    free(fb->color);
    free(fb->depth);
    free(fb);
}

bool framebuffer_resize(Framebuffer *fb, int width, int height) {
    if (!fb || width <= 0 || height <= 0) return false;
    uint32_t *nc = calloc((size_t)width * height, sizeof(uint32_t));
    float    *nd = malloc((size_t)width * height * sizeof(float));
    if (!nc || !nd) { free(nc); free(nd); return false; }
    for (int i = 0; i < width * height; i++) nd[i] = 1.0f;
    free(fb->color);
    free(fb->depth);
    fb->color  = nc;
    fb->depth  = nd;
    fb->width  = width;
    fb->height = height;
    return true;
}

void framebuffer_clear(Framebuffer *fb, Color color, float depth) {
    framebuffer_clear_color(fb, color);
    framebuffer_clear_depth(fb, depth);
}

void framebuffer_clear_color(Framebuffer *fb, Color color) {
    if (!fb) return;
    uint32_t packed = color_pack(color);
    int n = fb->width * fb->height;
    for (int i = 0; i < n; i++)
        fb->color[i] = packed;
}

void framebuffer_clear_depth(Framebuffer *fb, float depth) {
    if (!fb) return;
    int n = fb->width * fb->height;
    for (int i = 0; i < n; i++)
        fb->depth[i] = depth;
}

void framebuffer_set_pixel(Framebuffer *fb, int x, int y, Color c) {
    if (!fb || x < 0 || y < 0 || x >= fb->width || y >= fb->height) return;
    fb->color[y * fb->width + x] = color_pack(c);
}

Color framebuffer_get_pixel(const Framebuffer *fb, int x, int y) {
    if (!fb || x < 0 || y < 0 || x >= fb->width || y >= fb->height)
        return COLOR_BLACK;
    return color_unpack(fb->color[y * fb->width + x]);
}

void framebuffer_set_depth(Framebuffer *fb, int x, int y, float d) {
    if (!fb || x < 0 || y < 0 || x >= fb->width || y >= fb->height) return;
    fb->depth[y * fb->width + x] = d;
}

float framebuffer_get_depth(const Framebuffer *fb, int x, int y) {
    if (!fb || x < 0 || y < 0 || x >= fb->width || y >= fb->height) return 1.0f;
    return fb->depth[y * fb->width + x];
}

bool framebuffer_save_tga(const Framebuffer *fb, const char *path) {
    if (!fb || !path) return false;
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    /* TGA header - uncompressed true color */
    uint8_t header[18];
    memset(header, 0, sizeof(header));
    header[2]  = 2;  /* uncompressed RGB */
    header[12] = (uint8_t)(fb->width & 0xFF);
    header[13] = (uint8_t)((fb->width >> 8) & 0xFF);
    header[14] = (uint8_t)(fb->height & 0xFF);
    header[15] = (uint8_t)((fb->height >> 8) & 0xFF);
    header[16] = 32;  /* bits per pixel */
    header[17] = 0x28; /* top-left origin + 8 alpha bits */
    fwrite(header, 1, 18, fp);

    /* Write pixels top-to-bottom, BGRA order */
    for (int y = 0; y < fb->height; y++) {
        for (int x = 0; x < fb->width; x++) {
            Color c = framebuffer_get_pixel(fb, x, y);
            uint8_t bgra[4] = {c.b, c.g, c.r, c.a};
            fwrite(bgra, 1, 4, fp);
        }
    }
    fclose(fp);
    return true;
}

/* ================================================================== */
/*  Render state                                                       */
/* ================================================================== */

RenderState render_state_default(int fb_width, int fb_height) {
    RenderState s;
    s.depth_test    = true;
    s.depth_write   = true;
    s.backface_cull = true;
    s.blend_mode    = BLEND_NONE;
    s.tex_filter    = TEX_FILTER_NEAREST;
    s.wireframe     = false;
    s.viewport      = (Rect){0, 0, fb_width, fb_height};
    s.scissor       = (Rect){0, 0, fb_width, fb_height};
    s.scissor_enable = false;
    return s;
}

/* ================================================================== */
/*  Renderer                                                           */
/* ================================================================== */

XSharpRenderer *renderer_create(int width, int height) {
    XSharpRenderer *r = calloc(1, sizeof(XSharpRenderer));
    if (!r) return NULL;
    r->framebuffer = framebuffer_create(width, height);
    if (!r->framebuffer) { free(r); return NULL; }
    r->state = render_state_default(width, height);
    return r;
}

void renderer_destroy(XSharpRenderer *r) {
    if (!r) return;
    framebuffer_destroy(r->framebuffer);
    free(r);
}

void renderer_resize(XSharpRenderer *r, int width, int height) {
    if (!r) return;
    framebuffer_resize(r->framebuffer, width, height);
    r->state.viewport = (Rect){0, 0, width, height};
    if (!r->state.scissor_enable)
        r->state.scissor = r->state.viewport;
}

void renderer_clear(XSharpRenderer *r, Color color, float depth) {
    if (!r) return;
    framebuffer_clear(r->framebuffer, color, depth);
}

void renderer_set_state(XSharpRenderer *r, RenderState state) {
    if (!r) return;
    r->state = state;
}

/* ================================================================== */
/*  Blending                                                           */
/* ================================================================== */

Color blend_colors(Color dst, Color src, BlendMode mode) {
    switch (mode) {
    case BLEND_NONE:
        return src;

    case BLEND_ALPHA: {
        float sa = src.a / 255.0f;
        float da = 1.0f - sa;
        return (Color){
            (uint8_t)(src.r * sa + dst.r * da),
            (uint8_t)(src.g * sa + dst.g * da),
            (uint8_t)(src.b * sa + dst.b * da),
            (uint8_t)clampf(src.a + dst.a * da, 0, 255)
        };
    }

    case BLEND_ADDITIVE: {
        return (Color){
            (uint8_t)clampi(dst.r + src.r, 0, 255),
            (uint8_t)clampi(dst.g + src.g, 0, 255),
            (uint8_t)clampi(dst.b + src.b, 0, 255),
            (uint8_t)clampi(dst.a + src.a, 0, 255)
        };
    }

    case BLEND_MULTIPLY: {
        return (Color){
            (uint8_t)((dst.r * src.r) / 255),
            (uint8_t)((dst.g * src.g) / 255),
            (uint8_t)((dst.b * src.b) / 255),
            (uint8_t)((dst.a * src.a) / 255)
        };
    }
    }
    return src;
}

/* ================================================================== */
/*  Line drawing (Bresenham)                                           */
/* ================================================================== */

void renderer_draw_line(XSharpRenderer *r, int x0, int y0, int x1, int y1, Color c) {
    if (!r) return;
    Framebuffer *fb = r->framebuffer;

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        if (x0 >= 0 && x0 < fb->width && y0 >= 0 && y0 < fb->height) {
            if (r->state.blend_mode != BLEND_NONE) {
                Color dst = framebuffer_get_pixel(fb, x0, y0);
                c = blend_colors(dst, c, r->state.blend_mode);
            }
            framebuffer_set_pixel(fb, x0, y0, c);
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* ================================================================== */
/*  Triangle rasterization (scanline)                                  */
/* ================================================================== */

/* Helper: put a single fragment with depth test + blend */
static void put_fragment(XSharpRenderer *r, int x, int y, float depth, Color c) {
    Framebuffer *fb = r->framebuffer;
    const RenderState *st = &r->state;

    /* Bounds check */
    if (x < 0 || x >= fb->width || y < 0 || y >= fb->height) return;

    /* Scissor test */
    if (st->scissor_enable) {
        if (x < st->scissor.x || x >= st->scissor.x + st->scissor.width ||
            y < st->scissor.y || y >= st->scissor.y + st->scissor.height)
            return;
    }

    /* Depth test */
    if (st->depth_test) {
        float cur = fb->depth[y * fb->width + x];
        if (depth >= cur) return;
    }

    /* Blend */
    if (st->blend_mode != BLEND_NONE) {
        Color dst = color_unpack(fb->color[y * fb->width + x]);
        c = blend_colors(dst, c, st->blend_mode);
    }

    fb->color[y * fb->width + x] = color_pack(c);
    if (st->depth_write)
        fb->depth[y * fb->width + x] = depth;
}

/*
 * Scanline triangle rasterizer using edge-walking.
 * Interpolates depth, UV, color, and normal per-fragment using barycentric coords.
 */
void renderer_draw_triangle(XSharpRenderer *r, const RasterVertex tri[3]) {
    if (!r) return;
    Framebuffer *fb = r->framebuffer;
    const RenderState *st = &r->state;

    /* Wireframe mode */
    if (st->wireframe) {
        Color c0 = color_from_vec4(tri[0].color);
        renderer_draw_line(r, (int)tri[0].position.x, (int)tri[0].position.y,
                              (int)tri[1].position.x, (int)tri[1].position.y, c0);
        renderer_draw_line(r, (int)tri[1].position.x, (int)tri[1].position.y,
                              (int)tri[2].position.x, (int)tri[2].position.y, c0);
        renderer_draw_line(r, (int)tri[2].position.x, (int)tri[2].position.y,
                              (int)tri[0].position.x, (int)tri[0].position.y, c0);
        return;
    }

    /* Backface culling (screen-space winding order) */
    if (st->backface_cull) {
        float ax = tri[1].position.x - tri[0].position.x;
        float ay = tri[1].position.y - tri[0].position.y;
        float bx = tri[2].position.x - tri[0].position.x;
        float by = tri[2].position.y - tri[0].position.y;
        float cross = ax * by - ay * bx;
        if (cross <= 0.0f) return;  /* CW = back-facing */
    }

    /* Compute bounding box */
    float fminx = fminf(fminf(tri[0].position.x, tri[1].position.x), tri[2].position.x);
    float fmaxx = fmaxf(fmaxf(tri[0].position.x, tri[1].position.x), tri[2].position.x);
    float fminy = fminf(fminf(tri[0].position.y, tri[1].position.y), tri[2].position.y);
    float fmaxy = fmaxf(fmaxf(tri[0].position.y, tri[1].position.y), tri[2].position.y);

    int minx = maxi((int)floorf(fminx), 0);
    int maxx = mini((int)ceilf(fmaxx), fb->width - 1);
    int miny = maxi((int)floorf(fminy), 0);
    int maxy = mini((int)ceilf(fmaxy), fb->height - 1);

    /* Clip to viewport */
    minx = maxi(minx, st->viewport.x);
    maxx = mini(maxx, st->viewport.x + st->viewport.width - 1);
    miny = maxi(miny, st->viewport.y);
    maxy = mini(maxy, st->viewport.y + st->viewport.height - 1);

    if (minx > maxx || miny > maxy) return;

    /* Precompute edge function coefficients for barycentric coordinates */
    float x0 = tri[0].position.x, y0 = tri[0].position.y;
    float x1 = tri[1].position.x, y1 = tri[1].position.y;
    float x2 = tri[2].position.x, y2 = tri[2].position.y;

    float denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
    if (fabsf(denom) < XSHARP_EPSILON) return; /* degenerate */
    float inv_denom = 1.0f / denom;

    /* Perspective-correct interpolation weights (1/w per vertex) */
    float w0 = tri[0].position.w;
    float w1 = tri[1].position.w;
    float w2 = tri[2].position.w;

    /* Rasterize using barycentric coordinates (half-space method) */
    for (int py = miny; py <= maxy; py++) {
        for (int px = minx; px <= maxx; px++) {
            float pxf = px + 0.5f;
            float pyf = py + 0.5f;

            /* Barycentric coordinates */
            float l0 = ((y1 - y2) * (pxf - x2) + (x2 - x1) * (pyf - y2)) * inv_denom;
            float l1 = ((y2 - y0) * (pxf - x2) + (x0 - x2) * (pyf - y2)) * inv_denom;
            float l2 = 1.0f - l0 - l1;

            /* Inside test */
            if (l0 < 0.0f || l1 < 0.0f || l2 < 0.0f) continue;

            /* Perspective-correct interpolation */
            float inv_w = l0 * w0 + l1 * w1 + l2 * w2;
            if (fabsf(inv_w) < XSHARP_EPSILON) continue;
            float w = 1.0f / inv_w;

            /* Depth (linear interpolation in screen space is correct for z) */
            float depth = l0 * tri[0].position.z + l1 * tri[1].position.z + l2 * tri[2].position.z;

            /* Clip depth to [0,1] */
            if (depth < 0.0f || depth > 1.0f) continue;

            /* Interpolate attributes with perspective correction */
            float pc0 = l0 * w0 * w;
            float pc1 = l1 * w1 * w;
            float pc2 = l2 * w2 * w;

            /* UV */
            Vec2 uv;
            uv.x = pc0 * tri[0].uv.x + pc1 * tri[1].uv.x + pc2 * tri[2].uv.x;
            uv.y = pc0 * tri[0].uv.y + pc1 * tri[1].uv.y + pc2 * tri[2].uv.y;
            (void)uv; /* used by pipeline for texturing */

            /* Vertex color */
            Vec4 frag_color;
            frag_color.x = pc0 * tri[0].color.x + pc1 * tri[1].color.x + pc2 * tri[2].color.x;
            frag_color.y = pc0 * tri[0].color.y + pc1 * tri[1].color.y + pc2 * tri[2].color.y;
            frag_color.z = pc0 * tri[0].color.z + pc1 * tri[1].color.z + pc2 * tri[2].color.z;
            frag_color.w = pc0 * tri[0].color.w + pc1 * tri[1].color.w + pc2 * tri[2].color.w;

            Color c = color_from_vec4(frag_color);
            put_fragment(r, px, py, depth, c);
        }
    }
}
