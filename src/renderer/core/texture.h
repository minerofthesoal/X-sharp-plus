#ifndef XSHARP_TEXTURE_H
#define XSHARP_TEXTURE_H

#include "renderer.h"
#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Texture wrap / filter                                              */
/* ------------------------------------------------------------------ */

typedef enum { TEX_WRAP_REPEAT, TEX_WRAP_CLAMP, TEX_WRAP_MIRROR } TexWrap;

typedef enum {
    TEX_FMT_RGBA8, /* 4 bytes per pixel */
    TEX_FMT_RGB8,  /* 3 bytes per pixel */
    TEX_FMT_GRAY8  /* 1 byte per pixel  */
} TexFormat;

/* ------------------------------------------------------------------ */
/*  Mipmap level                                                       */
/* ------------------------------------------------------------------ */

typedef struct {
    int width, height;
    uint8_t* pixels; /* always stored as RGBA8 internally */
} MipLevel;

/* ------------------------------------------------------------------ */
/*  Texture                                                            */
/* ------------------------------------------------------------------ */

#define TEXTURE_MAX_MIPS 16

typedef struct XSharpTexture {
    int width, height;
    TexFormat src_format;
    TexWrap wrap_u, wrap_v;
    TexFilter min_filter, mag_filter;
    int mip_count;
    MipLevel mips[TEXTURE_MAX_MIPS];
} XSharpTexture;

/* Create from raw pixel data (copies the data) */
XSharpTexture* texture_create(int width, int height, TexFormat fmt, const uint8_t* pixels);

/* Create an empty texture */
XSharpTexture* texture_create_empty(int width, int height);

void texture_destroy(XSharpTexture* tex);

/* Update a sub-region of the base mip level */
void texture_update(XSharpTexture* tex, int x, int y, int w, int h, TexFormat fmt,
                    const uint8_t* pixels);

/* Generate mipmaps from the base level (box filter) */
void texture_generate_mipmaps(XSharpTexture* tex);

/* Sample a texture at (u,v) with given filter */
Color texture_sample(const XSharpTexture* tex, float u, float v, TexFilter filter);

/* Sample a specific mip level */
Color texture_sample_mip(const XSharpTexture* tex, int mip, float u, float v, TexFilter filter);

/* ------------------------------------------------------------------ */
/*  File loading                                                       */
/* ------------------------------------------------------------------ */

/* Load a 24/32-bit uncompressed TGA file */
XSharpTexture* texture_load_tga(const char* path);

/* Load a 24/32-bit uncompressed BMP file */
XSharpTexture* texture_load_bmp(const char* path);

/* ------------------------------------------------------------------ */
/*  Texture atlas                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    float u0, v0, u1, v1; /* UV coordinates for this sub-image */
    int pixel_x, pixel_y;
    int pixel_w, pixel_h;
} AtlasRegion;

typedef struct {
    XSharpTexture* texture;
    int region_count;
    int region_capacity;
    AtlasRegion* regions;
    /* Simple packing state */
    int cursor_x, cursor_y;
    int row_height;
} TextureAtlas;

TextureAtlas* atlas_create(int width, int height);
void atlas_destroy(TextureAtlas* atlas);

/* Add a sub-image to the atlas. Returns region index or -1 on failure. */
int atlas_add(TextureAtlas* atlas, int w, int h, TexFormat fmt, const uint8_t* pixels);

/* Get a region by index */
const AtlasRegion* atlas_get_region(const TextureAtlas* atlas, int index);

#endif /* XSHARP_TEXTURE_H */
