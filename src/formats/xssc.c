/*
 * X# (Xsharp) Source Container Format (.Xssc) - Implementation
 * ==============================================================
 */

#include "xssc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Utility ===== */

static char* safe_strdup(const char* s) {
    if (!s)
        return NULL;
    size_t len = strlen(s);
    char* dup = (char*)malloc(len + 1);
    if (dup)
        memcpy(dup, s, len + 1);
    return dup;
}

/* ===== CRC32 (standard polynomial 0xEDB88320) ===== */

static uint32_t crc32_table[256];
static int crc32_table_init = 0;

static void crc32_init_table(void) {
    if (crc32_table_init)
        return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320u;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_init = 1;
}

uint32_t xssc_crc32(const uint8_t* data, size_t len) {
    return xssc_crc32_update(0, data, len);
}

uint32_t xssc_crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
    crc32_init_table();
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

/* ===== Archive lifecycle ===== */

XsscArchive* xssc_create(void) {
    XsscArchive* ar = (XsscArchive*)calloc(1, sizeof(XsscArchive));
    if (!ar)
        return NULL;
    ar->version = XSSC_VERSION;
    ar->flags = XSSC_FLAG_NONE;
    ar->metadata_capacity = 16;
    ar->metadata = (XsscMetadata*)calloc(ar->metadata_capacity, sizeof(XsscMetadata));
    ar->entry_capacity = 16;
    ar->entries = (XsscEntry*)calloc(ar->entry_capacity, sizeof(XsscEntry));
    if (!ar->metadata || !ar->entries) {
        free(ar->metadata);
        free(ar->entries);
        free(ar);
        return NULL;
    }
    return ar;
}

void xssc_free(XsscArchive* archive) {
    if (!archive)
        return;

    for (int i = 0; i < archive->metadata_count; i++) {
        free(archive->metadata[i].key);
        free(archive->metadata[i].value);
    }
    free(archive->metadata);

    for (int i = 0; i < archive->entry_count; i++) {
        free(archive->entries[i].path);
        free(archive->entries[i].data);
    }
    free(archive->entries);
    free(archive->raw_data);
    free(archive);
}

/* ===== Metadata ===== */

bool xssc_set_metadata(XsscArchive* archive, const char* key, const char* value) {
    if (!archive || !key || !value)
        return false;
    if (strlen(key) > XSSC_MAX_KEY_LEN)
        return false;
    if (strlen(value) > XSSC_MAX_VAL_LEN)
        return false;

    /* Check if key already exists */
    for (int i = 0; i < archive->metadata_count; i++) {
        if (strcmp(archive->metadata[i].key, key) == 0) {
            free(archive->metadata[i].value);
            archive->metadata[i].value = safe_strdup(value);
            archive->metadata[i].value_len = (uint32_t)strlen(value);
            return true;
        }
    }

    /* New key */
    if (archive->metadata_count >= XSSC_MAX_METADATA)
        return false;
    if (archive->metadata_count >= archive->metadata_capacity) {
        int new_cap = archive->metadata_capacity * 2;
        XsscMetadata* tmp =
            (XsscMetadata*)realloc(archive->metadata, sizeof(XsscMetadata) * new_cap);
        if (!tmp)
            return false;
        archive->metadata = tmp;
        archive->metadata_capacity = new_cap;
    }

    XsscMetadata* m = &archive->metadata[archive->metadata_count++];
    m->key = safe_strdup(key);
    m->value = safe_strdup(value);
    m->value_len = (uint32_t)strlen(value);
    return true;
}

const char* xssc_get_metadata(const XsscArchive* archive, const char* key) {
    if (!archive || !key)
        return NULL;
    for (int i = 0; i < archive->metadata_count; i++) {
        if (strcmp(archive->metadata[i].key, key) == 0) {
            return archive->metadata[i].value;
        }
    }
    return NULL;
}

/* ===== Entry management ===== */

static int add_entry(XsscArchive* archive, const char* archive_path, const uint8_t* data,
                     size_t size, XsscEntryType type) {
    if (!archive || !archive_path || !data)
        return -1;
    if (strlen(archive_path) > XSSC_MAX_PATH_LEN)
        return -1;
    if (archive->entry_count >= XSSC_MAX_ENTRIES)
        return -1;

    if (archive->entry_count >= archive->entry_capacity) {
        int new_cap = archive->entry_capacity * 2;
        XsscEntry* tmp = (XsscEntry*)realloc(archive->entries, sizeof(XsscEntry) * new_cap);
        if (!tmp)
            return -1;
        archive->entries = tmp;
        archive->entry_capacity = new_cap;
    }

    int idx = archive->entry_count++;
    XsscEntry* e = &archive->entries[idx];
    e->path = safe_strdup(archive_path);
    e->content_size = size;
    e->type = type;
    e->checksum = xssc_crc32(data, size);
    e->data = (uint8_t*)malloc(size);
    if (!e->data) {
        archive->entry_count--;
        free(e->path);
        return -1;
    }
    memcpy(e->data, data, size);
    return idx;
}

int xssc_add_file(XsscArchive* archive, const char* disk_path, const char* archive_path,
                  XsscEntryType type) {
    if (!archive || !disk_path || !archive_path)
        return -1;

    FILE* f = fopen(disk_path, "rb");
    if (!f)
        return -1;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        fclose(f);
        return -1;
    }

    uint8_t* buf = (uint8_t*)malloc((size_t)fsize);
    if (!buf) {
        fclose(f);
        return -1;
    }

    if (fsize > 0) {
        size_t rd = fread(buf, 1, (size_t)fsize, f);
        if ((long)rd != fsize) {
            free(buf);
            fclose(f);
            return -1;
        }
    }
    fclose(f);

    int idx = add_entry(archive, archive_path, buf, (size_t)fsize, type);
    free(buf);
    return idx;
}

int xssc_add_data(XsscArchive* archive, const char* archive_path, const uint8_t* data, size_t size,
                  XsscEntryType type) {
    return add_entry(archive, archive_path, data, size, type);
}

/* ===== Serialization helpers ===== */

typedef struct {
    uint8_t* buf;
    size_t size;
    size_t cap;
} WriteBuf;

static bool wb_init(WriteBuf* wb, size_t initial) {
    wb->buf = (uint8_t*)malloc(initial);
    wb->size = 0;
    wb->cap = initial;
    return wb->buf != NULL;
}

static bool wb_ensure(WriteBuf* wb, size_t need) {
    if (wb->size + need <= wb->cap)
        return true;
    size_t new_cap = wb->cap;
    while (wb->size + need > new_cap)
        new_cap *= 2;
    uint8_t* tmp = (uint8_t*)realloc(wb->buf, new_cap);
    if (!tmp)
        return false;
    wb->buf = tmp;
    wb->cap = new_cap;
    return true;
}

static bool wb_write(WriteBuf* wb, const void* data, size_t len) {
    if (!wb_ensure(wb, len))
        return false;
    memcpy(wb->buf + wb->size, data, len);
    wb->size += len;
    return true;
}

static bool wb_write_u8(WriteBuf* wb, uint8_t v) {
    return wb_write(wb, &v, 1);
}
static bool wb_write_u16(WriteBuf* wb, uint16_t v) {
    return wb_write(wb, &v, 2);
}
static bool wb_write_u32(WriteBuf* wb, uint32_t v) {
    return wb_write(wb, &v, 4);
}
static bool wb_write_u64(WriteBuf* wb, uint64_t v) {
    return wb_write(wb, &v, 8);
}

/* ===== Serialization ===== */

bool xssc_serialize(XsscArchive* archive, uint8_t** out_buf, size_t* out_size) {
    if (!archive || !out_buf || !out_size)
        return false;

    WriteBuf wb;
    if (!wb_init(&wb, 4096))
        return false;

    /* ---- Header ---- */
    /* Magic */
    if (!wb_write(&wb, XSSC_MAGIC, XSSC_MAGIC_SIZE))
        goto fail;
    /* Version */
    if (!wb_write_u16(&wb, archive->version))
        goto fail;
    /* Flags */
    if (!wb_write_u16(&wb, archive->flags))
        goto fail;
    /* Header size placeholder - we'll patch it */
    size_t header_size_off = wb.size;
    if (!wb_write_u32(&wb, 0))
        goto fail;
    /* Entry count */
    if (!wb_write_u32(&wb, (uint32_t)archive->entry_count))
        goto fail;

    /* ---- Metadata section ---- */
    for (int i = 0; i < archive->metadata_count; i++) {
        XsscMetadata* m = &archive->metadata[i];
        uint16_t klen = (uint16_t)strlen(m->key);
        if (!wb_write_u16(&wb, klen))
            goto fail;
        if (!wb_write(&wb, m->key, klen))
            goto fail;
        uint32_t vlen = m->value_len;
        if (!wb_write_u32(&wb, vlen))
            goto fail;
        if (!wb_write(&wb, m->value, vlen))
            goto fail;
    }
    /* Terminator: key_len=0 */
    if (!wb_write_u16(&wb, 0))
        goto fail;

    /* Calculate content offsets */
    /* First pass: compute entry table size to determine content section start */
    size_t entry_table_start = wb.size;
    size_t entry_table_size = 0;
    for (int i = 0; i < archive->entry_count; i++) {
        entry_table_size += 2;                                /* path_len */
        entry_table_size += strlen(archive->entries[i].path); /* path */
        entry_table_size += 8;                                /* content_offset */
        entry_table_size += 8;                                /* content_size */
        entry_table_size += 4;                                /* checksum */
        entry_table_size += 1;                                /* entry_type */
    }

    size_t content_section_start = entry_table_start + entry_table_size;

    /* Patch header_size: everything from start to entry table end */
    uint32_t hdr_size = (uint32_t)content_section_start;
    memcpy(wb.buf + header_size_off, &hdr_size, 4);

    /* Set content offsets */
    uint64_t content_off = 0;
    for (int i = 0; i < archive->entry_count; i++) {
        archive->entries[i].content_offset = content_off;
        content_off += archive->entries[i].content_size;
    }

    /* ---- Entry table ---- */
    for (int i = 0; i < archive->entry_count; i++) {
        XsscEntry* e = &archive->entries[i];
        uint16_t plen = (uint16_t)strlen(e->path);
        if (!wb_write_u16(&wb, plen))
            goto fail;
        if (!wb_write(&wb, e->path, plen))
            goto fail;
        if (!wb_write_u64(&wb, e->content_offset))
            goto fail;
        if (!wb_write_u64(&wb, e->content_size))
            goto fail;
        if (!wb_write_u32(&wb, e->checksum))
            goto fail;
        if (!wb_write_u8(&wb, (uint8_t)e->type))
            goto fail;
    }

    /* ---- Content section ---- */
    for (int i = 0; i < archive->entry_count; i++) {
        XsscEntry* e = &archive->entries[i];
        if (e->data && e->content_size > 0) {
            if (!wb_write(&wb, e->data, (size_t)e->content_size))
                goto fail;
        }
    }

    /* ---- Footer ---- */
    /* CRC32 of everything so far */
    uint32_t total_crc = xssc_crc32(wb.buf, wb.size);
    if (!wb_write_u32(&wb, total_crc))
        goto fail;
    /* Reverse magic */
    if (!wb_write(&wb, XSSC_FOOTER_MAGIC, XSSC_MAGIC_SIZE))
        goto fail;

    /* Store raw data in archive */
    free(archive->raw_data);
    archive->raw_data = wb.buf;
    archive->raw_size = wb.size;

    *out_buf = wb.buf;
    *out_size = wb.size;
    return true;

fail:
    free(wb.buf);
    return false;
}

bool xssc_write(XsscArchive* archive, const char* path) {
    uint8_t* buf = NULL;
    size_t size = 0;

    if (!xssc_serialize(archive, &buf, &size))
        return false;

    FILE* f = fopen(path, "wb");
    if (!f)
        return false;

    size_t written = fwrite(buf, 1, size, f);
    fclose(f);

    return written == size;
}

/* ===== Deserialization helpers ===== */

typedef struct {
    const uint8_t* buf;
    size_t size;
    size_t pos;
} ReadBuf;

static bool rb_read(ReadBuf* rb, void* out, size_t len) {
    if (rb->pos + len > rb->size)
        return false;
    memcpy(out, rb->buf + rb->pos, len);
    rb->pos += len;
    return true;
}

static bool rb_read_u8(ReadBuf* rb, uint8_t* v) {
    return rb_read(rb, v, 1);
}
static bool rb_read_u16(ReadBuf* rb, uint16_t* v) {
    return rb_read(rb, v, 2);
}
static bool rb_read_u32(ReadBuf* rb, uint32_t* v) {
    return rb_read(rb, v, 4);
}
static bool rb_read_u64(ReadBuf* rb, uint64_t* v) {
    return rb_read(rb, v, 8);
}

/* ===== Deserialization ===== */

XsscArchive* xssc_read_buffer(const uint8_t* buf, size_t size) {
    if (!buf || size < XSSC_MAGIC_SIZE + 2 + 2 + 4 + 4 + 4 + XSSC_MAGIC_SIZE)
        return NULL;

    /* Check footer magic */
    if (memcmp(buf + size - XSSC_MAGIC_SIZE, XSSC_FOOTER_MAGIC, XSSC_MAGIC_SIZE) != 0)
        return NULL;

    /* Verify CRC32: everything before the footer (last 8 bytes = crc32 + magic) */
    size_t data_len = size - 4 - XSSC_MAGIC_SIZE;
    uint32_t stored_crc;
    memcpy(&stored_crc, buf + data_len, 4);
    uint32_t computed_crc = xssc_crc32(buf, data_len);
    if (stored_crc != computed_crc)
        return NULL;

    ReadBuf rb = {buf, size, 0};

    /* Magic */
    char magic[XSSC_MAGIC_SIZE];
    if (!rb_read(&rb, magic, XSSC_MAGIC_SIZE))
        return NULL;
    if (memcmp(magic, XSSC_MAGIC, XSSC_MAGIC_SIZE) != 0)
        return NULL;

    /* Version */
    uint16_t version;
    if (!rb_read_u16(&rb, &version))
        return NULL;

    /* Flags */
    uint16_t flags;
    if (!rb_read_u16(&rb, &flags))
        return NULL;

    /* Header size */
    uint32_t header_size;
    if (!rb_read_u32(&rb, &header_size))
        return NULL;

    /* Entry count */
    uint32_t entry_count;
    if (!rb_read_u32(&rb, &entry_count))
        return NULL;
    if (entry_count > XSSC_MAX_ENTRIES)
        return NULL;

    XsscArchive* ar = xssc_create();
    if (!ar)
        return NULL;
    ar->version = version;
    ar->flags = flags;
    ar->header_size = header_size;

    /* ---- Metadata section ---- */
    for (;;) {
        uint16_t klen;
        if (!rb_read_u16(&rb, &klen))
            goto fail;
        if (klen == 0)
            break; /* terminator */
        if (klen > XSSC_MAX_KEY_LEN)
            goto fail;

        char* key = (char*)malloc(klen + 1);
        if (!key)
            goto fail;
        if (!rb_read(&rb, key, klen)) {
            free(key);
            goto fail;
        }
        key[klen] = '\0';

        uint32_t vlen;
        if (!rb_read_u32(&rb, &vlen)) {
            free(key);
            goto fail;
        }
        if (vlen > XSSC_MAX_VAL_LEN) {
            free(key);
            goto fail;
        }

        char* val = (char*)malloc(vlen + 1);
        if (!val) {
            free(key);
            goto fail;
        }
        if (!rb_read(&rb, val, vlen)) {
            free(key);
            free(val);
            goto fail;
        }
        val[vlen] = '\0';

        xssc_set_metadata(ar, key, val);
        free(key);
        free(val);
    }

    /* ---- Entry table ---- */
    for (uint32_t i = 0; i < entry_count; i++) {
        uint16_t plen;
        if (!rb_read_u16(&rb, &plen))
            goto fail;
        if (plen > XSSC_MAX_PATH_LEN)
            goto fail;

        char* path = (char*)malloc(plen + 1);
        if (!path)
            goto fail;
        if (!rb_read(&rb, path, plen)) {
            free(path);
            goto fail;
        }
        path[plen] = '\0';

        uint64_t c_offset, c_size;
        uint32_t checksum;
        uint8_t etype;
        if (!rb_read_u64(&rb, &c_offset)) {
            free(path);
            goto fail;
        }
        if (!rb_read_u64(&rb, &c_size)) {
            free(path);
            goto fail;
        }
        if (!rb_read_u32(&rb, &checksum)) {
            free(path);
            goto fail;
        }
        if (!rb_read_u8(&rb, &etype)) {
            free(path);
            goto fail;
        }

        /* Grow entries if needed */
        if (ar->entry_count >= ar->entry_capacity) {
            int new_cap = ar->entry_capacity * 2;
            XsscEntry* tmp = (XsscEntry*)realloc(ar->entries, sizeof(XsscEntry) * new_cap);
            if (!tmp) {
                free(path);
                goto fail;
            }
            ar->entries = tmp;
            ar->entry_capacity = new_cap;
        }

        XsscEntry* e = &ar->entries[ar->entry_count++];
        memset(e, 0, sizeof(XsscEntry));
        e->path = path;
        e->content_offset = c_offset;
        e->content_size = c_size;
        e->checksum = checksum;
        e->type = (XsscEntryType)etype;
        e->data = NULL; /* loaded below */
    }

    /* ---- Content section ---- */
    /* rb.pos should now be at content_section_start = header_size */
    size_t content_start = rb.pos;
    for (int i = 0; i < ar->entry_count; i++) {
        XsscEntry* e = &ar->entries[i];
        if (e->content_size > 0) {
            size_t abs_off = content_start + (size_t)e->content_offset;
            if (abs_off + e->content_size > data_len)
                goto fail;
            e->data = (uint8_t*)malloc((size_t)e->content_size);
            if (!e->data)
                goto fail;
            memcpy(e->data, buf + abs_off, (size_t)e->content_size);
        }
    }

    /* Store raw data */
    ar->raw_data = (uint8_t*)malloc(size);
    if (ar->raw_data) {
        memcpy(ar->raw_data, buf, size);
        ar->raw_size = size;
    }

    return ar;

fail:
    xssc_free(ar);
    return NULL;
}

XsscArchive* xssc_read(const char* path) {
    if (!path)
        return NULL;

    FILE* f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(f);
        return NULL;
    }

    uint8_t* buf = (uint8_t*)malloc((size_t)fsize);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t rd = fread(buf, 1, (size_t)fsize, f);
    fclose(f);

    if ((long)rd != fsize) {
        free(buf);
        return NULL;
    }

    XsscArchive* ar = xssc_read_buffer(buf, (size_t)fsize);
    free(buf);
    return ar;
}

/* ===== Extraction ===== */

/* Helper: create directories for a path */
static void ensure_dir(const char* filepath) {
    char* tmp = safe_strdup(filepath);
    if (!tmp)
        return;
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';
#ifdef _WIN32
            _mkdir(tmp);
#else
            /* Use a simple system call approach */
            char cmd[XSSC_MAX_PATH_LEN + 16];
            snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", tmp);
            (void)system(cmd);
#endif
            *p = '/';
        }
    }
    free(tmp);
}

bool xssc_extract(const XsscArchive* archive, int entry_index, const char* output_dir) {
    if (!archive || !output_dir)
        return false;
    if (entry_index < 0 || entry_index >= archive->entry_count)
        return false;

    const XsscEntry* e = &archive->entries[entry_index];
    if (!e->data && e->content_size > 0)
        return false;

    char fullpath[XSSC_MAX_PATH_LEN * 2];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", output_dir, e->path);

    ensure_dir(fullpath);

    FILE* f = fopen(fullpath, "wb");
    if (!f)
        return false;

    if (e->content_size > 0 && e->data) {
        size_t written = fwrite(e->data, 1, (size_t)e->content_size, f);
        fclose(f);
        return written == (size_t)e->content_size;
    }

    fclose(f);
    return true;
}

int xssc_extract_all(const XsscArchive* archive, const char* output_dir) {
    if (!archive || !output_dir)
        return -1;

    int extracted = 0;
    for (int i = 0; i < archive->entry_count; i++) {
        if (xssc_extract(archive, i, output_dir))
            extracted++;
        else
            return -1;
    }
    return extracted;
}

/* ===== Listing ===== */

static const char* entry_type_str(XsscEntryType t) {
    switch (t) {
    case XSSC_ENTRY_SOURCE:
        return "source";
    case XSSC_ENTRY_ASSET:
        return "asset";
    case XSSC_ENTRY_CONFIG:
        return "config";
    case XSSC_ENTRY_BYTECODE:
        return "bytecode";
    default:
        return "unknown";
    }
}

void xssc_list(const XsscArchive* archive) {
    if (!archive)
        return;

    printf("XSSC Archive v%u (flags=0x%04X)\n", archive->version, archive->flags);
    printf("Entries: %d\n", archive->entry_count);

    if (archive->metadata_count > 0) {
        printf("Metadata:\n");
        for (int i = 0; i < archive->metadata_count; i++) {
            printf("  %s = %s\n", archive->metadata[i].key, archive->metadata[i].value);
        }
    }

    printf("\n%-8s %-10s %-10s %-8s  %s\n", "Type", "Size", "CRC32", "Offset", "Path");
    printf("-------- ---------- ---------- --------  ----\n");

    for (int i = 0; i < archive->entry_count; i++) {
        const XsscEntry* e = &archive->entries[i];
        printf("%-8s %10llu 0x%08X %8llu  %s\n", entry_type_str(e->type),
               (unsigned long long)e->content_size, e->checksum,
               (unsigned long long)e->content_offset, e->path);
    }
}

/* ===== Validation ===== */

bool xssc_validate(const XsscArchive* archive) {
    if (!archive)
        return false;

    for (int i = 0; i < archive->entry_count; i++) {
        const XsscEntry* e = &archive->entries[i];
        if (!e->data && e->content_size > 0)
            return false;
        if (e->data && e->content_size > 0) {
            uint32_t crc = xssc_crc32(e->data, (size_t)e->content_size);
            if (crc != e->checksum)
                return false;
        }
    }
    return true;
}

bool xssc_validate_file(const char* path) {
    if (!path)
        return false;

    FILE* f = fopen(path, "rb");
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < (long)(XSSC_MAGIC_SIZE + 4 + XSSC_MAGIC_SIZE)) {
        fclose(f);
        return false;
    }

    uint8_t* buf = (uint8_t*)malloc((size_t)fsize);
    if (!buf) {
        fclose(f);
        return false;
    }

    size_t rd = fread(buf, 1, (size_t)fsize, f);
    fclose(f);
    if ((long)rd != fsize) {
        free(buf);
        return false;
    }

    /* Check header magic */
    if (memcmp(buf, XSSC_MAGIC, XSSC_MAGIC_SIZE) != 0) {
        free(buf);
        return false;
    }

    /* Check footer magic */
    if (memcmp(buf + fsize - XSSC_MAGIC_SIZE, XSSC_FOOTER_MAGIC, XSSC_MAGIC_SIZE) != 0) {
        free(buf);
        return false;
    }

    /* Check CRC32 */
    size_t data_len = (size_t)fsize - 4 - XSSC_MAGIC_SIZE;
    uint32_t stored_crc;
    memcpy(&stored_crc, buf + data_len, 4);
    uint32_t computed_crc = xssc_crc32(buf, data_len);

    free(buf);
    return stored_crc == computed_crc;
}
