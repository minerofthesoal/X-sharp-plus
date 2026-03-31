/*
 * X# (Xsharp) Compressed Source Container Format (.Xscsc) - Implementation
 * ==========================================================================
 */

#include "xscsc.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* ===== Lifecycle ===== */

XscscArchive* xscsc_create(void) {
    XscscArchive* ar = (XscscArchive*)calloc(1, sizeof(XscscArchive));
    if (!ar)
        return NULL;
    ar->version = XSCSC_VERSION;
    ar->compression_method = XSCSC_COMPRESS_LZMA2;
    return ar;
}

void xscsc_free(XscscArchive* ar) {
    if (!ar)
        return;
    free(ar->compressed_data);
    free(ar);
}

/* ===== Compression ===== */

XscscArchive* xscsc_compress(XsscArchive* xssc, int compression_level) {
    if (!xssc)
        return NULL;

    /* Serialize the Xssc archive to a buffer */
    uint8_t* raw = NULL;
    size_t raw_size = 0;
    if (!xssc_serialize(xssc, &raw, &raw_size))
        return NULL;

    /* Determine dictionary size */
    if (compression_level < LZMA2_MIN_LEVEL)
        compression_level = LZMA2_DEFAULT_LEVEL;
    if (compression_level > LZMA2_MAX_LEVEL)
        compression_level = LZMA2_MAX_LEVEL;
    uint32_t dict_size = lzma2_dict_size_for_level(compression_level);

    /* Compress with LZMA2 */
    uint8_t* comp = NULL;
    size_t comp_size = 0;
    if (!lzma2_compress(raw, raw_size, &comp, &comp_size, compression_level, dict_size)) {
        /* raw is owned by xssc->raw_data, don't free it here */
        return NULL;
    }

    XscscArchive* ar = xscsc_create();
    if (!ar) {
        free(comp);
        return NULL;
    }

    ar->uncompressed_size = raw_size;
    ar->compressed_size = comp_size;
    ar->dictionary_size = dict_size;
    ar->compressed_data = comp;
    ar->data_checksum = xssc_crc32(comp, comp_size);

    return ar;
}

XsscArchive* xscsc_decompress(const XscscArchive* xscsc) {
    if (!xscsc || !xscsc->compressed_data)
        return NULL;
    if (xscsc->compression_method != XSCSC_COMPRESS_LZMA2)
        return NULL;

    /* Verify checksum */
    uint32_t crc = xssc_crc32(xscsc->compressed_data, (size_t)xscsc->compressed_size);
    if (crc != xscsc->data_checksum)
        return NULL;

    /* Decompress */
    uint8_t* raw = (uint8_t*)malloc((size_t)xscsc->uncompressed_size);
    if (!raw)
        return NULL;

    if (!lzma2_decompress(xscsc->compressed_data, (size_t)xscsc->compressed_size, raw,
                          (size_t)xscsc->uncompressed_size, xscsc->dictionary_size)) {
        free(raw);
        return NULL;
    }

    /* Parse the Xssc archive from decompressed data */
    XsscArchive* xssc_ar = xssc_read_buffer(raw, (size_t)xscsc->uncompressed_size);
    free(raw);
    return xssc_ar;
}

/* ===== I/O ===== */

bool xscsc_write(const XscscArchive* ar, const char* path) {
    if (!ar || !path || !ar->compressed_data)
        return false;

    FILE* f = fopen(path, "wb");
    if (!f)
        return false;

    /* Header */
    fwrite(XSCSC_MAGIC, 1, XSCSC_MAGIC_SIZE, f);
    fwrite(&ar->version, 2, 1, f);
    fwrite(&ar->compression_method, 1, 1, f);
    fwrite(&ar->uncompressed_size, 8, 1, f);
    fwrite(&ar->compressed_size, 8, 1, f);
    fwrite(&ar->dictionary_size, 4, 1, f);

    /* Compressed data */
    fwrite(ar->compressed_data, 1, (size_t)ar->compressed_size, f);

    /* Footer */
    fwrite(&ar->data_checksum, 4, 1, f);
    fwrite(XSCSC_FOOTER_MAGIC, 1, XSCSC_MAGIC_SIZE, f);

    fclose(f);
    return true;
}

XscscArchive* xscsc_read(const char* path) {
    if (!path)
        return NULL;

    FILE* f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < (long)(XSCSC_HEADER_SIZE + XSCSC_FOOTER_SIZE)) {
        fclose(f);
        return NULL;
    }

    /* Read header */
    char magic[XSCSC_MAGIC_SIZE];
    if (fread(magic, 1, XSCSC_MAGIC_SIZE, f) != XSCSC_MAGIC_SIZE ||
        memcmp(magic, XSCSC_MAGIC, XSCSC_MAGIC_SIZE) != 0) {
        fclose(f);
        return NULL;
    }

    XscscArchive* ar = xscsc_create();
    if (!ar) {
        fclose(f);
        return NULL;
    }

    if (fread(&ar->version, 2, 1, f) != 1)
        goto fail;
    if (fread(&ar->compression_method, 1, 1, f) != 1)
        goto fail;
    if (fread(&ar->uncompressed_size, 8, 1, f) != 1)
        goto fail;
    if (fread(&ar->compressed_size, 8, 1, f) != 1)
        goto fail;
    if (fread(&ar->dictionary_size, 4, 1, f) != 1)
        goto fail;

    /* Read compressed data */
    ar->compressed_data = (uint8_t*)malloc((size_t)ar->compressed_size);
    if (!ar->compressed_data)
        goto fail;
    if (fread(ar->compressed_data, 1, (size_t)ar->compressed_size, f) !=
        (size_t)ar->compressed_size)
        goto fail;

    /* Read footer */
    if (fread(&ar->data_checksum, 4, 1, f) != 1)
        goto fail;

    char footer[XSCSC_MAGIC_SIZE];
    if (fread(footer, 1, XSCSC_MAGIC_SIZE, f) != XSCSC_MAGIC_SIZE ||
        memcmp(footer, XSCSC_FOOTER_MAGIC, XSCSC_MAGIC_SIZE) != 0)
        goto fail;

    fclose(f);

    /* Verify checksum */
    uint32_t crc = xssc_crc32(ar->compressed_data, (size_t)ar->compressed_size);
    if (crc != ar->data_checksum) {
        xscsc_free(ar);
        return NULL;
    }

    return ar;

fail:
    fclose(f);
    xscsc_free(ar);
    return NULL;
}

/* ===== Directory traversal helper ===== */

static bool add_dir_recursive(XsscArchive* xssc, const char* dir_path, const char* prefix) {
    DIR* d = opendir(dir_path);
    if (!d)
        return false;

    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char fullpath[4096];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, ent->d_name);

        char arcpath[4096];
        if (prefix && prefix[0])
            snprintf(arcpath, sizeof(arcpath), "%s/%s", prefix, ent->d_name);
        else
            snprintf(arcpath, sizeof(arcpath), "%s", ent->d_name);

        struct stat st;
        if (stat(fullpath, &st) != 0)
            continue;

        if (S_ISDIR(st.st_mode)) {
            if (!add_dir_recursive(xssc, fullpath, arcpath)) {
                closedir(d);
                return false;
            }
        } else if (S_ISREG(st.st_mode)) {
            /* Determine entry type from extension */
            XsscEntryType type = XSSC_ENTRY_ASSET;
            const char* ext = strrchr(ent->d_name, '.');
            if (ext) {
                if (strcmp(ext, ".xs") == 0)
                    type = XSSC_ENTRY_SOURCE;
                else if (strcmp(ext, ".xsb") == 0)
                    type = XSSC_ENTRY_BYTECODE;
                else if (strcmp(ext, ".cfg") == 0 || strcmp(ext, ".json") == 0 ||
                         strcmp(ext, ".toml") == 0)
                    type = XSSC_ENTRY_CONFIG;
            }

            if (xssc_add_file(xssc, fullpath, arcpath, type) < 0) {
                closedir(d);
                return false;
            }
        }
    }

    closedir(d);
    return true;
}

/* ===== High-level convenience ===== */

bool xscsc_create_from_dir(const char* dir_path, const char* output_path, int compression_level) {
    if (!dir_path || !output_path)
        return false;

    XsscArchive* xssc = xssc_create();
    if (!xssc)
        return false;

    xssc_set_metadata(xssc, "creator", "xsharp-compiler");
    xssc_set_metadata(xssc, "format", "xscsc");

    if (!add_dir_recursive(xssc, dir_path, "")) {
        xssc_free(xssc);
        return false;
    }

    XscscArchive* xscsc = xscsc_compress(xssc, compression_level);
    xssc_free(xssc);
    if (!xscsc)
        return false;

    bool ok = xscsc_write(xscsc, output_path);
    xscsc_free(xscsc);
    return ok;
}

int xscsc_extract_all(const char* archive_path, const char* output_dir) {
    if (!archive_path || !output_dir)
        return -1;

    XscscArchive* xscsc = xscsc_read(archive_path);
    if (!xscsc)
        return -1;

    XsscArchive* xssc = xscsc_decompress(xscsc);
    xscsc_free(xscsc);
    if (!xssc)
        return -1;

    int result = xssc_extract_all(xssc, output_dir);
    xssc_free(xssc);
    return result;
}

/* ===== Validation ===== */

bool xscsc_validate_file(const char* path) {
    XscscArchive* ar = xscsc_read(path);
    if (!ar)
        return false;

    /* xscsc_read already validates the CRC32 */
    /* Additionally try to decompress */
    XsscArchive* xssc = xscsc_decompress(ar);
    xscsc_free(ar);

    if (!xssc)
        return false;

    bool valid = xssc_validate(xssc);
    xssc_free(xssc);
    return valid;
}
