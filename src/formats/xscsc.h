/*
 * X# (Xsharp) Compressed Source Container Format (.Xscsc)
 * =========================================================
 * LZMA2 compressed version of .Xssc archives.
 *
 * File Structure:
 *   Magic: "XSCSC" (5 bytes)
 *   Version: uint16 (2 bytes)
 *   Compression method: uint8 (1 = LZMA2)
 *   Uncompressed size: uint64 (8 bytes)
 *   Compressed size: uint64 (8 bytes)
 *   Dictionary size: uint32 (4 bytes)
 *   LZMA2 compressed data
 *   Checksum of compressed data: uint32 (CRC32)
 *   Footer magic: "CSCSX" (5 bytes)
 */

#ifndef XSHARP_XSCSC_H
#define XSHARP_XSCSC_H

#include "lzma2.h"
#include "xssc.h"

/* Magic bytes */
#define XSCSC_MAGIC "XSCSC"
#define XSCSC_MAGIC_SIZE 5
#define XSCSC_FOOTER_MAGIC "CSCSX"
#define XSCSC_VERSION 1

/* Compression methods */
#define XSCSC_COMPRESS_LZMA2 1

/* Header size (magic + version + method + uncomp_size + comp_size + dict_size) */
#define XSCSC_HEADER_SIZE (5 + 2 + 1 + 8 + 8 + 4)

/* Footer size (crc32 + footer_magic) */
#define XSCSC_FOOTER_SIZE (4 + 5)

/* ===== Compressed archive container ===== */
typedef struct {
    uint16_t version;
    uint8_t compression_method;
    uint64_t uncompressed_size;
    uint64_t compressed_size;
    uint32_t dictionary_size;
    uint8_t* compressed_data; /* LZMA2 compressed payload */
    uint32_t data_checksum;   /* CRC32 of compressed data */
} XscscArchive;

/* ===== Lifecycle ===== */
XscscArchive* xscsc_create(void);
void xscsc_free(XscscArchive* ar);

/* ===== Compression ===== */

/*
 * Compress an in-memory .Xssc archive to .Xscsc.
 * Returns a new XscscArchive or NULL on error.
 * compression_level: 1-9 (default 5)
 */
XscscArchive* xscsc_compress(XsscArchive* xssc, int compression_level);

/*
 * Decompress a .Xscsc archive back to an in-memory .Xssc archive.
 * Returns a new XsscArchive or NULL on error.
 */
XsscArchive* xscsc_decompress(const XscscArchive* xscsc);

/* ===== I/O ===== */

/*
 * Write a .Xscsc archive to disk.
 */
bool xscsc_write(const XscscArchive* ar, const char* path);

/*
 * Read a .Xscsc archive from disk.
 */
XscscArchive* xscsc_read(const char* path);

/* ===== High-level convenience ===== */

/*
 * Create a compressed archive from a directory.
 * Recursively adds all files. Writes to output_path.
 * Returns true on success.
 */
bool xscsc_create_from_dir(const char* dir_path, const char* output_path, int compression_level);

/*
 * Decompress and extract all files from a .Xscsc archive.
 * Returns number of files extracted, or -1 on error.
 */
int xscsc_extract_all(const char* archive_path, const char* output_dir);

/* ===== Validation ===== */

/*
 * Validate a .Xscsc file on disk.
 */
bool xscsc_validate_file(const char* path);

#endif /* XSHARP_XSCSC_H */
