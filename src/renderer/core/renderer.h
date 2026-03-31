#ifndef XSHARP_RENDERER_H
#define XSHARP_RENDERER_H

#include "math3d.h"
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Color                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t r, g, b, a;
} Color;

static inline Color color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (Color){r, g, b, a};
}
static inline Color color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (Color){r, g, b, 255};
}
static inline Color color_from_vec4(Vec4 v) {
    return (Color){
        (uint8_t)(clampf(v.x, 0, 1) * 255.0f),
        (uint8_t)(clampf(v.y, 0, 1) * 255.0f),
        (uint8_t)(clampf(v.z, 0, 1) * 255.0f),
        (uint8_t)(clampf(v.w, 0, 1) * 255.0f)
    };
}
static inline Vec4 color_to_vec4(Color c) {
    return (Vec4){c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
}
static inline uint32_t color_pack(Color c) {
    return ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) |
           ((uint32_t)c.g << 8)  | (uint32_t)c.r;
}
static inline Color color_unpack(uint32_t p) {
    return (Color){p & 0xFF, (p >> 8) & 0xFF, (p >> 16) & 0xFF, (p >> 24) & 0xFF};
}

#define COLOR_WHITE   ((Color){255,255,255,255})
#define COLOR_BLACK   ((Color){0,0,0,255})
#define COLOR_RED     ((Color){255,0,0,255})
#define COLOR_GREEN   ((Color){0,255,0,255})
#define COLOR_BLUE    ((Color){0,0,255,255})
#define COLOR_CLEAR   ((Color){0,0,0,0})

/* ------------------------------------------------------------------ */
/*  Blend mode                                                         */
/* ------------------------------------------------------------------ */

typedef enum {
    BLEND_NONE,
    BLEND_ALPHA,
    BLEND_ADDITIVE,
    BLEND_MULTIPLY
} BlendMode;

/* ------------------------------------------------------------------ */
/*  Texture filter                                                     */
/* ------------------------------------------------------------------ */

typedef enum {
    TEX_FILTER_NEAREST,
    TEX_FILTER_BILINEAR
} TexFilter;

/* ------------------------------------------------------------------ */
/*  Framebuffer                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    int       width, height;
    uint32_t *color;       /* packed RGBA pixels */
    float    *depth;       /* z-buffer, one float per pixel */
} Framebuffer;

Framebuffer *framebuffer_create(int width, int height);
void         framebuffer_destroy(Framebuffer *fb);
bool         framebuffer_resize(Framebuffer *fb, int width, int height);
void         framebuffer_clear(Framebuffer *fb, Color color, float depth);
void         framebuffer_clear_color(Framebuffer *fb, Color color);
void         framebuffer_clear_depth(Framebuffer *fb, float depth);
void         framebuffer_set_pixel(Framebuffer *fb, int x, int y, Color c);
Color        framebuffer_get_pixel(const Framebuffer *fb, int x, int y);
void         framebuffer_set_depth(Framebuffer *fb, int x, int y, float d);
float        framebuffer_get_depth(const Framebuffer *fb, int x, int y);

/* Write framebuffer to a TGA file */
bool         framebuffer_save_tga(const Framebuffer *fb, const char *path);

/* ------------------------------------------------------------------ */
/*  Viewport / Scissor                                                 */
/* ------------------------------------------------------------------ */

typedef struct {
    int x, y, width, height;
} Rect;

/* ------------------------------------------------------------------ */
/*  Render state                                                       */
/* ------------------------------------------------------------------ */

typedef struct {
    bool       depth_test;
    bool       depth_write;
    bool       backface_cull;
    BlendMode  blend_mode;
    TexFilter  tex_filter;
    bool       wireframe;
    Rect       viewport;
    Rect       scissor;
    bool       scissor_enable;
} RenderState;

RenderState render_state_default(int fb_width, int fb_height);

/* ------------------------------------------------------------------ */
/*  Renderer context                                                   */
/* ------------------------------------------------------------------ */

typedef struct XSharpRenderer {
    Framebuffer *framebuffer;
    RenderState  state;
} XSharpRenderer;

XSharpRenderer *renderer_create(int width, int height);
void            renderer_destroy(XSharpRenderer *r);
void            renderer_resize(XSharpRenderer *r, int width, int height);
void            renderer_clear(XSharpRenderer *r, Color color, float depth);
void            renderer_set_state(XSharpRenderer *r, RenderState state);

/* ------------------------------------------------------------------ */
/*  Software rasterization primitives                                  */
/* ------------------------------------------------------------------ */

/* A vertex as passed into the rasterizer (post-transform, screen space) */
typedef struct {
    Vec4  position;    /* screen-space (x,y = pixel, z = depth, w = 1/w) */
    Vec3  world_pos;   /* world-space position for lighting */
    Vec3  normal;      /* interpolated normal */
    Vec2  uv;          /* texture coordinates */
    Vec4  color;       /* vertex color (0..1 RGBA) */
} RasterVertex;

/* Draw a single triangle. Handles clipping, depth, blending. */
void renderer_draw_triangle(XSharpRenderer *r, const RasterVertex tri[3]);

/* Draw a line (Bresenham) */
void renderer_draw_line(XSharpRenderer *r, int x0, int y0, int x1, int y1, Color c);

/* ------------------------------------------------------------------ */
/*  Blending                                                           */
/* ------------------------------------------------------------------ */

Color blend_colors(Color dst, Color src, BlendMode mode);

/* ------------------------------------------------------------------ */
/*  Texture sampling (forward decl; full API in texture.h)             */
/* ------------------------------------------------------------------ */

struct XSharpTexture;
Color texture_sample(const struct XSharpTexture *tex, float u, float v, TexFilter filter);

#endif /* XSHARP_RENDERER_H */
