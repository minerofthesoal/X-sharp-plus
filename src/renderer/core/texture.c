#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/*  Internal helpers                                                   */
/* ================================================================== */

/* Number of bytes per pixel for a source format */
static int format_bpp(TexFormat fmt) {
    switch (fmt) {
    case TEX_FMT_RGBA8:
        return 4;
    case TEX_FMT_RGB8:
        return 3;
    case TEX_FMT_GRAY8:
        return 1;
    }
    return 4;
}

/* Convert arbitrary format data to RGBA8.  Caller must free result. */
static uint8_t* convert_to_rgba8(int w, int h, TexFormat fmt, const uint8_t* src) {
    int n = w * h;
    uint8_t* dst = malloc((size_t)n * 4);
    if (!dst)
        return NULL;

    switch (fmt) {
    case TEX_FMT_RGBA8:
        memcpy(dst, src, (size_t)n * 4);
        break;
    case TEX_FMT_RGB8:
        for (int i = 0; i < n; i++) {
            dst[i * 4 + 0] = src[i * 3 + 0];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 255;
        }
        break;
    case TEX_FMT_GRAY8:
        for (int i = 0; i < n; i++) {
            dst[i * 4 + 0] = src[i];
            dst[i * 4 + 1] = src[i];
            dst[i * 4 + 2] = src[i];
            dst[i * 4 + 3] = 255;
        }
        break;
    }
    return dst;
}

/* Wrap a UV coordinate */
static float wrap_coord(float t, TexWrap mode) {
    switch (mode) {
    case TEX_WRAP_REPEAT:
        t = t - floorf(t);
        break;
    case TEX_WRAP_CLAMP:
        t = clampf(t, 0.0f, 1.0f);
        break;
    case TEX_WRAP_MIRROR: {
        float ft = floorf(t);
        int n = (int)ft;
        t = t - ft;
        if (n % 2 != 0)
            t = 1.0f - t;
        break;
    }
    }
    return t;
}

/* Fetch a texel from a mip level (clamped) */
static Color fetch_texel(const MipLevel* mip, int x, int y) {
    x = clampi(x, 0, mip->width - 1);
    y = clampi(y, 0, mip->height - 1);
    int idx = (y * mip->width + x) * 4;
    return (Color){mip->pixels[idx], mip->pixels[idx + 1], mip->pixels[idx + 2],
                   mip->pixels[idx + 3]};
}

/* ================================================================== */
/*  Texture create / destroy                                           */
/* ================================================================== */

XSharpTexture* texture_create(int width, int height, TexFormat fmt, const uint8_t* pixels) {
    if (width <= 0 || height <= 0)
        return NULL;

    XSharpTexture* tex = calloc(1, sizeof(XSharpTexture));
    if (!tex)
        return NULL;

    tex->width = width;
    tex->height = height;
    tex->src_format = fmt;
    tex->wrap_u = TEX_WRAP_REPEAT;
    tex->wrap_v = TEX_WRAP_REPEAT;
    tex->min_filter = TEX_FILTER_NEAREST;
    tex->mag_filter = TEX_FILTER_NEAREST;
    tex->mip_count = 1;

    if (pixels) {
        tex->mips[0].width = width;
        tex->mips[0].height = height;
        tex->mips[0].pixels = convert_to_rgba8(width, height, fmt, pixels);
    } else {
        tex->mips[0].width = width;
        tex->mips[0].height = height;
        tex->mips[0].pixels = calloc((size_t)width * height * 4, 1);
    }

    if (!tex->mips[0].pixels) {
        free(tex);
        return NULL;
    }
    return tex;
}

XSharpTexture* texture_create_empty(int width, int height) {
    return texture_create(width, height, TEX_FMT_RGBA8, NULL);
}

void texture_destroy(XSharpTexture* tex) {
    if (!tex)
        return;
    for (int i = 0; i < tex->mip_count; i++)
        free(tex->mips[i].pixels);
    free(tex);
}

void texture_update(XSharpTexture* tex, int x, int y, int w, int h, TexFormat fmt,
                    const uint8_t* pixels) {
    if (!tex || !pixels)
        return;
    MipLevel* mip = &tex->mips[0];
    int bpp = format_bpp(fmt);

    for (int row = 0; row < h; row++) {
        int dy = y + row;
        if (dy < 0 || dy >= mip->height)
            continue;
        for (int col = 0; col < w; col++) {
            int dx = x + col;
            if (dx < 0 || dx >= mip->width)
                continue;

            const uint8_t* src = &pixels[(row * w + col) * bpp];
            uint8_t* dst = &mip->pixels[(dy * mip->width + dx) * 4];

            switch (fmt) {
            case TEX_FMT_RGBA8:
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = src[3];
                break;
            case TEX_FMT_RGB8:
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = 255;
                break;
            case TEX_FMT_GRAY8:
                dst[0] = dst[1] = dst[2] = src[0];
                dst[3] = 255;
                break;
            }
        }
    }
}

/* ================================================================== */
/*  Mipmap generation                                                  */
/* ================================================================== */

void texture_generate_mipmaps(XSharpTexture* tex) {
    if (!tex)
        return;

    /* Free existing mips (except base) */
    for (int i = 1; i < tex->mip_count; i++)
        free(tex->mips[i].pixels);

    tex->mip_count = 1;

    int w = tex->width, h = tex->height;
    while ((w > 1 || h > 1) && tex->mip_count < TEXTURE_MAX_MIPS) {
        int nw = maxi(w / 2, 1);
        int nh = maxi(h / 2, 1);
        int level = tex->mip_count;

        tex->mips[level].width = nw;
        tex->mips[level].height = nh;
        tex->mips[level].pixels = malloc((size_t)nw * nh * 4);
        if (!tex->mips[level].pixels)
            break;

        const MipLevel* prev = &tex->mips[level - 1];

        /* Box filter: average 2x2 block */
        for (int y = 0; y < nh; y++) {
            for (int x = 0; x < nw; x++) {
                int sx = x * 2, sy = y * 2;
                int sx1 = mini(sx + 1, prev->width - 1);
                int sy1 = mini(sy + 1, prev->height - 1);

                /* Gather 4 texels */
                const uint8_t* p00 = &prev->pixels[(sy * prev->width + sx) * 4];
                const uint8_t* p10 = &prev->pixels[(sy * prev->width + sx1) * 4];
                const uint8_t* p01 = &prev->pixels[(sy1 * prev->width + sx) * 4];
                const uint8_t* p11 = &prev->pixels[(sy1 * prev->width + sx1) * 4];

                uint8_t* dst = &tex->mips[level].pixels[(y * nw + x) * 4];
                for (int c = 0; c < 4; c++) {
                    dst[c] = (uint8_t)((p00[c] + p10[c] + p01[c] + p11[c] + 2) / 4);
                }
            }
        }

        tex->mip_count++;
        w = nw;
        h = nh;
    }
}

/* ================================================================== */
/*  Texture sampling                                                   */
/* ================================================================== */

Color texture_sample(const XSharpTexture* tex, float u, float v, TexFilter filter) {
    return texture_sample_mip(tex, 0, u, v, filter);
}

Color texture_sample_mip(const XSharpTexture* tex, int mip, float u, float v, TexFilter filter) {
    if (!tex || mip < 0 || mip >= tex->mip_count)
        return COLOR_BLACK;

    const MipLevel* lvl = &tex->mips[mip];
    if (!lvl->pixels)
        return COLOR_BLACK;

    u = wrap_coord(u, tex->wrap_u);
    v = wrap_coord(v, tex->wrap_v);

    switch (filter) {
    case TEX_FILTER_NEAREST: {
        int x = (int)(u * lvl->width) % lvl->width;
        int y = (int)(v * lvl->height) % lvl->height;
        if (x < 0)
            x += lvl->width;
        if (y < 0)
            y += lvl->height;
        return fetch_texel(lvl, x, y);
    }

    case TEX_FILTER_BILINEAR: {
        float fx = u * lvl->width - 0.5f;
        float fy = v * lvl->height - 0.5f;
        int x0 = (int)floorf(fx);
        int y0 = (int)floorf(fy);
        float tx = fx - x0;
        float ty = fy - y0;

        Color c00 = fetch_texel(lvl, x0, y0);
        Color c10 = fetch_texel(lvl, x0 + 1, y0);
        Color c01 = fetch_texel(lvl, x0, y0 + 1);
        Color c11 = fetch_texel(lvl, x0 + 1, y0 + 1);

        uint8_t r = (uint8_t)(c00.r * (1 - tx) * (1 - ty) + c10.r * tx * (1 - ty) +
                              c01.r * (1 - tx) * ty + c11.r * tx * ty);
        uint8_t g = (uint8_t)(c00.g * (1 - tx) * (1 - ty) + c10.g * tx * (1 - ty) +
                              c01.g * (1 - tx) * ty + c11.g * tx * ty);
        uint8_t b = (uint8_t)(c00.b * (1 - tx) * (1 - ty) + c10.b * tx * (1 - ty) +
                              c01.b * (1 - tx) * ty + c11.b * tx * ty);
        uint8_t a = (uint8_t)(c00.a * (1 - tx) * (1 - ty) + c10.a * tx * (1 - ty) +
                              c01.a * (1 - tx) * ty + c11.a * tx * ty);
        return (Color){r, g, b, a};
    }
    }
    return COLOR_BLACK;
}

/* ================================================================== */
/*  TGA loading                                                        */
/* ================================================================== */

XSharpTexture* texture_load_tga(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    uint8_t header[18];
    if (fread(header, 1, 18, fp) != 18) {
        fclose(fp);
        return NULL;
    }

    uint8_t id_length = header[0];
    uint8_t image_type = header[2];
    int width = header[12] | (header[13] << 8);
    int height = header[14] | (header[15] << 8);
    uint8_t bpp = header[16];
    uint8_t descriptor = header[17];

    /* Only support uncompressed true-color (2) and grayscale (3) */
    if (image_type != 2 && image_type != 3) {
        fclose(fp);
        return NULL;
    }
    if (bpp != 24 && bpp != 32 && bpp != 8) {
        fclose(fp);
        return NULL;
    }

    /* Skip image ID */
    if (id_length > 0)
        fseek(fp, id_length, SEEK_CUR);

    int pixel_count = width * height;
    int src_bpp = bpp / 8;
    uint8_t* raw = malloc((size_t)pixel_count * src_bpp);
    if (!raw) {
        fclose(fp);
        return NULL;
    }

    if (fread(raw, 1, (size_t)pixel_count * src_bpp, fp) != (size_t)pixel_count * src_bpp) {
        free(raw);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    /* Convert to RGBA8 */
    uint8_t* rgba = malloc((size_t)pixel_count * 4);
    if (!rgba) {
        free(raw);
        return NULL;
    }

    bool top_origin = (descriptor & 0x20) != 0;

    for (int y = 0; y < height; y++) {
        int src_y = top_origin ? y : (height - 1 - y);
        for (int x = 0; x < width; x++) {
            int si = (src_y * width + x) * src_bpp;
            int di = (y * width + x) * 4;

            if (bpp == 32) {
                rgba[di + 0] = raw[si + 2]; /* TGA stores BGRA */
                rgba[di + 1] = raw[si + 1];
                rgba[di + 2] = raw[si + 0];
                rgba[di + 3] = raw[si + 3];
            } else if (bpp == 24) {
                rgba[di + 0] = raw[si + 2];
                rgba[di + 1] = raw[si + 1];
                rgba[di + 2] = raw[si + 0];
                rgba[di + 3] = 255;
            } else { /* 8 bpp grayscale */
                rgba[di + 0] = raw[si];
                rgba[di + 1] = raw[si];
                rgba[di + 2] = raw[si];
                rgba[di + 3] = 255;
            }
        }
    }
    free(raw);

    XSharpTexture* tex = texture_create(width, height, TEX_FMT_RGBA8, rgba);
    free(rgba);
    return tex;
}

/* ================================================================== */
/*  BMP loading                                                        */
/* ================================================================== */

XSharpTexture* texture_load_bmp(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    /* Read BMP file header (14 bytes) */
    uint8_t file_header[14];
    if (fread(file_header, 1, 14, fp) != 14) {
        fclose(fp);
        return NULL;
    }
    if (file_header[0] != 'B' || file_header[1] != 'M') {
        fclose(fp);
        return NULL;
    }

    uint32_t data_offset = file_header[10] | (file_header[11] << 8) | (file_header[12] << 16) |
                           (file_header[13] << 24);

    /* Read DIB header (at least 40 bytes for BITMAPINFOHEADER) */
    uint8_t dib[40];
    if (fread(dib, 1, 40, fp) != 40) {
        fclose(fp);
        return NULL;
    }

    int32_t width = (int32_t)(dib[4] | (dib[5] << 8) | (dib[6] << 16) | (dib[7] << 24));
    int32_t height = (int32_t)(dib[8] | (dib[9] << 8) | (dib[10] << 16) | (dib[11] << 24));
    uint16_t bpp = dib[14] | (dib[15] << 8);
    uint32_t compr = dib[16] | (dib[17] << 8) | (dib[18] << 16) | (dib[19] << 24);

    /* Only support uncompressed 24/32 bpp */
    if (compr != 0) {
        fclose(fp);
        return NULL;
    }
    if (bpp != 24 && bpp != 32) {
        fclose(fp);
        return NULL;
    }
    if (width <= 0) {
        fclose(fp);
        return NULL;
    }

    bool top_down = height < 0;
    if (height < 0)
        height = -height;

    /* Seek to pixel data */
    fseek(fp, data_offset, SEEK_SET);

    int src_bpp = bpp / 8;
    int row_size = ((width * src_bpp + 3) / 4) * 4; /* rows padded to 4 bytes */

    uint8_t* row_buf = malloc(row_size);
    uint8_t* rgba = malloc((size_t)width * height * 4);
    if (!row_buf || !rgba) {
        free(row_buf);
        free(rgba);
        fclose(fp);
        return NULL;
    }

    for (int y = 0; y < height; y++) {
        int dst_y = top_down ? y : (height - 1 - y);
        if (fread(row_buf, 1, row_size, fp) != (size_t)row_size) {
            free(row_buf);
            free(rgba);
            fclose(fp);
            return NULL;
        }
        for (int x = 0; x < width; x++) {
            int si = x * src_bpp;
            int di = (dst_y * width + x) * 4;
            rgba[di + 0] = row_buf[si + 2]; /* BMP stores BGR(A) */
            rgba[di + 1] = row_buf[si + 1];
            rgba[di + 2] = row_buf[si + 0];
            rgba[di + 3] = (bpp == 32) ? row_buf[si + 3] : 255;
        }
    }
    free(row_buf);
    fclose(fp);

    XSharpTexture* tex = texture_create(width, height, TEX_FMT_RGBA8, rgba);
    free(rgba);
    return tex;
}

/* ================================================================== */
/*  Texture Atlas                                                      */
/* ================================================================== */

TextureAtlas* atlas_create(int width, int height) {
    TextureAtlas* atlas = calloc(1, sizeof(TextureAtlas));
    if (!atlas)
        return NULL;

    atlas->texture = texture_create_empty(width, height);
    if (!atlas->texture) {
        free(atlas);
        return NULL;
    }

    atlas->region_capacity = 64;
    atlas->regions = malloc(sizeof(AtlasRegion) * atlas->region_capacity);
    if (!atlas->regions) {
        texture_destroy(atlas->texture);
        free(atlas);
        return NULL;
    }

    atlas->region_count = 0;
    atlas->cursor_x = 0;
    atlas->cursor_y = 0;
    atlas->row_height = 0;
    return atlas;
}

void atlas_destroy(TextureAtlas* atlas) {
    if (!atlas)
        return;
    texture_destroy(atlas->texture);
    free(atlas->regions);
    free(atlas);
}

int atlas_add(TextureAtlas* atlas, int w, int h, TexFormat fmt, const uint8_t* pixels) {
    if (!atlas || !pixels || w <= 0 || h <= 0)
        return -1;

    int aw = atlas->texture->width;
    int ah = atlas->texture->height;

    /* Simple shelf packing: if current row can't fit, move to next row */
    if (atlas->cursor_x + w > aw) {
        atlas->cursor_x = 0;
        atlas->cursor_y += atlas->row_height;
        atlas->row_height = 0;
    }
    if (atlas->cursor_y + h > ah)
        return -1; /* atlas full */

    /* Blit pixels into the atlas texture */
    texture_update(atlas->texture, atlas->cursor_x, atlas->cursor_y, w, h, fmt, pixels);

    /* Grow region array if needed */
    if (atlas->region_count >= atlas->region_capacity) {
        int new_cap = atlas->region_capacity * 2;
        AtlasRegion* nr = realloc(atlas->regions, sizeof(AtlasRegion) * new_cap);
        if (!nr)
            return -1;
        atlas->regions = nr;
        atlas->region_capacity = new_cap;
    }

    int idx = atlas->region_count++;
    atlas->regions[idx].pixel_x = atlas->cursor_x;
    atlas->regions[idx].pixel_y = atlas->cursor_y;
    atlas->regions[idx].pixel_w = w;
    atlas->regions[idx].pixel_h = h;
    atlas->regions[idx].u0 = (float)atlas->cursor_x / aw;
    atlas->regions[idx].v0 = (float)atlas->cursor_y / ah;
    atlas->regions[idx].u1 = (float)(atlas->cursor_x + w) / aw;
    atlas->regions[idx].v1 = (float)(atlas->cursor_y + h) / ah;

    atlas->cursor_x += w;
    if (h > atlas->row_height)
        atlas->row_height = h;

    return idx;
}

const AtlasRegion* atlas_get_region(const TextureAtlas* atlas, int index) {
    if (!atlas || index < 0 || index >= atlas->region_count)
        return NULL;
    return &atlas->regions[index];
}
