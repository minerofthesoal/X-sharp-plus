/*
 * X# (Xsharp) Source Container Format (.Xssc)
 * =============================================
 * Custom binary+text hybrid archive format for X# source files.
 *
 * File Structure:
 *   Magic: "XSSC" (4 bytes)
 *   Version: uint16 (2 bytes)
 *   Flags: uint16 (2 bytes)
 *   Header size: uint32 (4 bytes)
 *   Entry count: uint32 (4 bytes)
 *   Metadata section:
 *     Key-value pairs: [uint16 key_len][key bytes][uint32 val_len][value bytes]
 *     Terminated by key_len=0
 *   Entry table:
 *     For each entry:
 *       Path length: uint16
 *       Path: UTF-8 bytes
 *       Content offset: uint64
 *       Content size: uint64
 *       Checksum: uint32 (CRC32)
 *       Entry type: uint8 (0=source, 1=asset, 2=config, 3=bytecode)
 *   Content section:
 *     Raw content bytes for each entry
 *   Footer:
 *     Total checksum: uint32 (CRC32 of entire file minus footer)
 *     Magic: "CSSX" (reverse magic, 4 bytes)
 */

#ifndef XSHARP_XSSC_H
#define XSHARP_XSSC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Magic bytes */
#define XSSC_MAGIC "XSSC"
#define XSSC_MAGIC_SIZE 4
#define XSSC_FOOTER_MAGIC "CSSX"
#define XSSC_VERSION 1

/* Entry types */
typedef enum {
    XSSC_ENTRY_SOURCE = 0,
    XSSC_ENTRY_ASSET = 1,
    XSSC_ENTRY_CONFIG = 2,
    XSSC_ENTRY_BYTECODE = 3
} XsscEntryType;

/* Flags */
#define XSSC_FLAG_NONE 0x0000
#define XSSC_FLAG_SIGNED 0x0001
#define XSSC_FLAG_ENCRYPTED 0x0002

/* Maximum limits */
#define XSSC_MAX_PATH_LEN 4096
#define XSSC_MAX_ENTRIES 65535
#define XSSC_MAX_METADATA 256
#define XSSC_MAX_KEY_LEN 255
#define XSSC_MAX_VAL_LEN 65535

/* ===== Metadata key-value pair ===== */
typedef struct {
    char* key;
    char* value;
    uint32_t value_len;
} XsscMetadata;

/* ===== Archive entry ===== */
typedef struct {
    char* path;
    uint64_t content_offset; /* offset within content section */
    uint64_t content_size;
    uint32_t checksum; /* CRC32 of content */
    XsscEntryType type;
    uint8_t* data; /* in-memory content (NULL if not loaded) */
} XsscEntry;

/* ===== Archive container ===== */
typedef struct {
    uint16_t version;
    uint16_t flags;
    uint32_t header_size;

    /* Metadata */
    XsscMetadata* metadata;
    int metadata_count;
    int metadata_capacity;

    /* Entries */
    XsscEntry* entries;
    int entry_count;
    int entry_capacity;

    /* Raw serialized data (set after xssc_read or xssc_write) */
    uint8_t* raw_data;
    size_t raw_size;
} XsscArchive;

/* ===== CRC32 ===== */
uint32_t xssc_crc32(const uint8_t* data, size_t len);
uint32_t xssc_crc32_update(uint32_t crc, const uint8_t* data, size_t len);

/* ===== Archive lifecycle ===== */
XsscArchive* xssc_create(void);
void xssc_free(XsscArchive* archive);

/* ===== Metadata ===== */
bool xssc_set_metadata(XsscArchive* archive, const char* key, const char* value);
const char* xssc_get_metadata(const XsscArchive* archive, const char* key);

/* ===== Entry management ===== */

/* Add a file from disk to the archive. Returns entry index or -1 on error. */
int xssc_add_file(XsscArchive* archive, const char* disk_path, const char* archive_path,
                  XsscEntryType type);

/* Add raw data to the archive. Returns entry index or -1 on error. */
int xssc_add_data(XsscArchive* archive, const char* archive_path, const uint8_t* data, size_t size,
                  XsscEntryType type);

/* ===== Extraction ===== */

/* Extract a single entry to disk. Returns true on success. */
bool xssc_extract(const XsscArchive* archive, int entry_index, const char* output_dir);

/* Extract all entries to output_dir. Returns number of files extracted, -1 on error. */
int xssc_extract_all(const XsscArchive* archive, const char* output_dir);

/* ===== Listing ===== */

/* Print a listing of archive contents to stdout. */
void xssc_list(const XsscArchive* archive);

/* ===== I/O ===== */

/* Read an archive from disk. Returns NULL on error. */
XsscArchive* xssc_read(const char* path);

/* Read an archive from a memory buffer. Returns NULL on error. */
XsscArchive* xssc_read_buffer(const uint8_t* buf, size_t size);

/* Write the archive to disk. Returns true on success. */
bool xssc_write(XsscArchive* archive, const char* path);

/* Serialize the archive to a memory buffer. Caller must free *out_buf.
 * Returns true on success. */
bool xssc_serialize(XsscArchive* archive, uint8_t** out_buf, size_t* out_size);

/* ===== Validation ===== */

/* Validate archive integrity (checks all CRC32 checksums). */
bool xssc_validate(const XsscArchive* archive);

/* Validate a file on disk without fully loading it. */
bool xssc_validate_file(const char* path);

#endif /* XSHARP_XSSC_H */
