/*
 * X# (Xsharp) Debug Information - Implementation
 * =================================================
 */

#include "debug_info.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== Utility ===== */

static char *safe_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *dup = (char *)malloc(len + 1);
    if (dup) memcpy(dup, s, len + 1);
    return dup;
}

/* ===== Source Map ===== */

void xs_source_map_init(XsSourceMap *map) {
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

void xs_source_map_free(XsSourceMap *map) {
    free(map->entries);
    map->entries = NULL;
    map->count = 0;
    map->capacity = 0;
}

void xs_source_map_add(XsSourceMap *map, uint32_t offset,
                        const char *file, int line, int column) {
    if (map->count >= map->capacity) {
        int new_cap = map->capacity < 16 ? 16 : map->capacity * 2;
        XsSourceMapEntry *new_entries = (XsSourceMapEntry *)realloc(
            map->entries, sizeof(XsSourceMapEntry) * new_cap);
        if (!new_entries) return;
        map->entries = new_entries;
        map->capacity = new_cap;
    }

    XsSourceMapEntry *e = &map->entries[map->count++];
    e->bytecode_offset = offset;
    e->loc.file = file;  /* caller owns the string; typically stored in debug info */
    e->loc.line = line;
    e->loc.column = column;
}

/* Binary search the sorted source map for the closest entry <= offset */
XsSourceLoc *xs_source_map_lookup(XsSourceMap *map, uint32_t offset) {
    if (!map || map->count == 0) return NULL;

    int lo = 0, hi = map->count - 1;
    int best = -1;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (map->entries[mid].bytecode_offset <= offset) {
            best = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    if (best < 0) return NULL;
    return &map->entries[best].loc;
}

uint32_t xs_source_map_find_offset(XsSourceMap *map,
                                    const char *file, int line) {
    if (!map) return UINT32_MAX;

    /* Find the first entry matching file:line */
    for (int i = 0; i < map->count; i++) {
        XsSourceMapEntry *e = &map->entries[i];
        if (e->loc.line == line) {
            if (!file || !e->loc.file || strcmp(e->loc.file, file) == 0) {
                return e->bytecode_offset;
            }
        }
    }
    return UINT32_MAX;
}

/* ===== Debug Info ===== */

XsDebugInfo *xs_debug_info_new(const char *module_name, const char *source_file) {
    XsDebugInfo *info = (XsDebugInfo *)calloc(1, sizeof(XsDebugInfo));
    if (!info) return NULL;

    info->module_name = safe_strdup(module_name);
    info->source_file = safe_strdup(source_file);
    xs_source_map_init(&info->source_map);

    return info;
}

void xs_debug_info_free(XsDebugInfo *info) {
    if (!info) return;

    free(info->module_name);
    free(info->source_file);
    free(info->source_text);

    xs_source_map_free(&info->source_map);

    /* Free functions */
    for (int i = 0; i < info->func_count; i++) {
        XsFuncDebugInfo *f = &info->functions[i];
        free(f->name);
        free(f->source_file);
        for (int j = 0; j < f->param_count; j++) {
            free(f->param_names[j]);
            free(f->param_types[j]);
        }
        free(f->param_names);
        free(f->param_types);
        for (int j = 0; j < f->local_count; j++) {
            free(f->locals[j].name);
            free(f->locals[j].type_name);
        }
        free(f->locals);
    }
    free(info->functions);

    /* Free globals */
    for (int i = 0; i < info->global_count; i++) {
        free(info->globals[i].name);
        free(info->globals[i].type_name);
    }
    free(info->globals);

    free(info);
}

/* ===== Function Debug Info ===== */

XsFuncDebugInfo *xs_debug_info_add_func(XsDebugInfo *info, const char *name,
                                         const char *source_file,
                                         int line_start, int line_end,
                                         uint32_t bytecode_start) {
    if (!info) return NULL;

    if (info->func_count >= info->func_capacity) {
        int new_cap = info->func_capacity < 8 ? 8 : info->func_capacity * 2;
        XsFuncDebugInfo *new_funcs = (XsFuncDebugInfo *)realloc(
            info->functions, sizeof(XsFuncDebugInfo) * new_cap);
        if (!new_funcs) return NULL;
        info->functions = new_funcs;
        info->func_capacity = new_cap;
    }

    XsFuncDebugInfo *f = &info->functions[info->func_count++];
    memset(f, 0, sizeof(XsFuncDebugInfo));
    f->name = safe_strdup(name);
    f->source_file = safe_strdup(source_file);
    f->line_start = line_start;
    f->line_end = line_end;
    f->bytecode_start = bytecode_start;

    return f;
}

void xs_func_debug_add_param(XsFuncDebugInfo *func,
                              const char *name, const char *type) {
    if (!func) return;

    int idx = func->param_count;
    func->param_count++;

    func->param_names = (char **)realloc(func->param_names,
                                          sizeof(char *) * func->param_count);
    func->param_types = (char **)realloc(func->param_types,
                                          sizeof(char *) * func->param_count);
    func->param_names[idx] = safe_strdup(name);
    func->param_types[idx] = safe_strdup(type);
}

void xs_func_debug_add_local(XsFuncDebugInfo *func,
                              const char *name, const char *type,
                              int slot, int start_off, int end_off,
                              int line_defined) {
    if (!func) return;

    if (func->local_count >= func->locals_capacity) {
        int new_cap = func->locals_capacity < 8 ? 8 : func->locals_capacity * 2;
        XsVarDebugInfo *new_locals = (XsVarDebugInfo *)realloc(
            func->locals, sizeof(XsVarDebugInfo) * new_cap);
        if (!new_locals) return;
        func->locals = new_locals;
        func->locals_capacity = new_cap;
    }

    XsVarDebugInfo *v = &func->locals[func->local_count++];
    v->name = safe_strdup(name);
    v->type_name = safe_strdup(type);
    v->scope = VAR_SCOPE_LOCAL;
    v->slot = slot;
    v->start_offset = start_off;
    v->end_offset = end_off;
    v->line_defined = line_defined;
}

XsFuncDebugInfo *xs_debug_info_find_func(XsDebugInfo *info, uint32_t offset) {
    if (!info) return NULL;

    for (int i = 0; i < info->func_count; i++) {
        XsFuncDebugInfo *f = &info->functions[i];
        if (offset >= f->bytecode_start &&
            offset < f->bytecode_start + f->bytecode_length) {
            return f;
        }
    }
    return NULL;
}

XsFuncDebugInfo *xs_debug_info_find_func_by_name(XsDebugInfo *info,
                                                   const char *name) {
    if (!info || !name) return NULL;

    for (int i = 0; i < info->func_count; i++) {
        if (info->functions[i].name && strcmp(info->functions[i].name, name) == 0) {
            return &info->functions[i];
        }
    }
    return NULL;
}

/* ===== Global Variable Debug Info ===== */

void xs_debug_info_add_global(XsDebugInfo *info, const char *name,
                               const char *type, int slot) {
    if (!info) return;

    if (info->global_count >= info->global_capacity) {
        int new_cap = info->global_capacity < 8 ? 8 : info->global_capacity * 2;
        XsVarDebugInfo *new_globals = (XsVarDebugInfo *)realloc(
            info->globals, sizeof(XsVarDebugInfo) * new_cap);
        if (!new_globals) return;
        info->globals = new_globals;
        info->global_capacity = new_cap;
    }

    XsVarDebugInfo *v = &info->globals[info->global_count++];
    v->name = safe_strdup(name);
    v->type_name = safe_strdup(type);
    v->scope = VAR_SCOPE_GLOBAL;
    v->slot = slot;
    v->start_offset = 0;
    v->end_offset = INT32_MAX;
    v->line_defined = 0;
}

/* ===== Source Text ===== */

void xs_debug_info_set_source(XsDebugInfo *info, const char *src) {
    if (!info) return;
    free(info->source_text);
    info->source_text = safe_strdup(src);

    /* Count lines */
    info->source_line_count = 0;
    if (info->source_text) {
        info->source_line_count = 1;
        for (const char *p = info->source_text; *p; p++) {
            if (*p == '\n') info->source_line_count++;
        }
    }
}

const char *xs_debug_info_get_line(XsDebugInfo *info, int line, int *out_length) {
    if (!info || !info->source_text || line < 1) return NULL;

    const char *p = info->source_text;
    int current_line = 1;

    while (*p && current_line < line) {
        if (*p == '\n') current_line++;
        p++;
    }

    if (current_line != line) return NULL;

    /* Find end of line */
    const char *end = p;
    while (*end && *end != '\n') end++;

    if (out_length) *out_length = (int)(end - p);
    return p;
}

/* ===== Serialization ===== */

/* Simple binary format:
 *   Magic: "XSDI" (4 bytes)
 *   Version: uint32
 *   Module name: length-prefixed string
 *   Source file: length-prefixed string
 *   Source text: length-prefixed string
 *   Source map entry count: uint32
 *   Source map entries: [offset(u32), line(u32), col(u32)] ...
 *   Function count: uint32
 *   Functions: [name, file, line_start, line_end, bc_start, bc_len,
 *               param_count, params..., local_count, locals...]
 *   Global count: uint32
 *   Globals: [name, type, slot]
 */

/* Helper: write length-prefixed string */
static bool write_string(uint8_t **buf, size_t *size, size_t *cap,
                          const char *s) {
    uint32_t len = s ? (uint32_t)strlen(s) : 0;
    size_t need = *size + 4 + len;

    while (need > *cap) {
        *cap = *cap < 256 ? 256 : *cap * 2;
        uint8_t *new_buf = (uint8_t *)realloc(*buf, *cap);
        if (!new_buf) return false;
        *buf = new_buf;
    }

    memcpy(*buf + *size, &len, 4);
    *size += 4;
    if (len > 0) {
        memcpy(*buf + *size, s, len);
        *size += len;
    }
    return true;
}

static bool write_u32(uint8_t **buf, size_t *size, size_t *cap, uint32_t val) {
    size_t need = *size + 4;
    while (need > *cap) {
        *cap = *cap < 256 ? 256 : *cap * 2;
        uint8_t *new_buf = (uint8_t *)realloc(*buf, *cap);
        if (!new_buf) return false;
        *buf = new_buf;
    }
    memcpy(*buf + *size, &val, 4);
    *size += 4;
    return true;
}

static bool write_i32(uint8_t **buf, size_t *size, size_t *cap, int32_t val) {
    return write_u32(buf, size, cap, (uint32_t)val);
}

bool xs_debug_info_serialize(XsDebugInfo *info,
                              uint8_t **out_buf, size_t *out_size) {
    if (!info) return false;

    uint8_t *buf = NULL;
    size_t size = 0, cap = 0;

    /* Magic */
    if (!write_u32(&buf, &size, &cap, 0x49445358)) goto fail; /* "XSDI" */
    if (!write_u32(&buf, &size, &cap, 1)) goto fail;          /* version */

    if (!write_string(&buf, &size, &cap, info->module_name)) goto fail;
    if (!write_string(&buf, &size, &cap, info->source_file)) goto fail;
    if (!write_string(&buf, &size, &cap, info->source_text)) goto fail;

    /* Source map */
    if (!write_u32(&buf, &size, &cap, (uint32_t)info->source_map.count)) goto fail;
    for (int i = 0; i < info->source_map.count; i++) {
        XsSourceMapEntry *e = &info->source_map.entries[i];
        if (!write_u32(&buf, &size, &cap, e->bytecode_offset)) goto fail;
        if (!write_i32(&buf, &size, &cap, e->loc.line)) goto fail;
        if (!write_i32(&buf, &size, &cap, e->loc.column)) goto fail;
    }

    /* Functions */
    if (!write_u32(&buf, &size, &cap, (uint32_t)info->func_count)) goto fail;
    for (int i = 0; i < info->func_count; i++) {
        XsFuncDebugInfo *f = &info->functions[i];
        if (!write_string(&buf, &size, &cap, f->name)) goto fail;
        if (!write_string(&buf, &size, &cap, f->source_file)) goto fail;
        if (!write_i32(&buf, &size, &cap, f->line_start)) goto fail;
        if (!write_i32(&buf, &size, &cap, f->line_end)) goto fail;
        if (!write_u32(&buf, &size, &cap, f->bytecode_start)) goto fail;
        if (!write_u32(&buf, &size, &cap, f->bytecode_length)) goto fail;

        if (!write_u32(&buf, &size, &cap, (uint32_t)f->param_count)) goto fail;
        for (int j = 0; j < f->param_count; j++) {
            if (!write_string(&buf, &size, &cap, f->param_names[j])) goto fail;
            if (!write_string(&buf, &size, &cap, f->param_types[j])) goto fail;
        }

        if (!write_u32(&buf, &size, &cap, (uint32_t)f->local_count)) goto fail;
        for (int j = 0; j < f->local_count; j++) {
            XsVarDebugInfo *v = &f->locals[j];
            if (!write_string(&buf, &size, &cap, v->name)) goto fail;
            if (!write_string(&buf, &size, &cap, v->type_name)) goto fail;
            if (!write_i32(&buf, &size, &cap, v->slot)) goto fail;
            if (!write_i32(&buf, &size, &cap, v->start_offset)) goto fail;
            if (!write_i32(&buf, &size, &cap, v->end_offset)) goto fail;
            if (!write_i32(&buf, &size, &cap, v->line_defined)) goto fail;
        }
    }

    /* Globals */
    if (!write_u32(&buf, &size, &cap, (uint32_t)info->global_count)) goto fail;
    for (int i = 0; i < info->global_count; i++) {
        XsVarDebugInfo *v = &info->globals[i];
        if (!write_string(&buf, &size, &cap, v->name)) goto fail;
        if (!write_string(&buf, &size, &cap, v->type_name)) goto fail;
        if (!write_i32(&buf, &size, &cap, v->slot)) goto fail;
    }

    *out_buf = buf;
    *out_size = size;
    return true;

fail:
    free(buf);
    return false;
}

/* Helper: read length-prefixed string */
static char *read_string(const uint8_t **p, const uint8_t *end) {
    if (*p + 4 > end) return NULL;
    uint32_t len;
    memcpy(&len, *p, 4);
    *p += 4;

    if (*p + len > end) return NULL;
    if (len == 0) return safe_strdup("");

    char *s = (char *)malloc(len + 1);
    if (!s) return NULL;
    memcpy(s, *p, len);
    s[len] = '\0';
    *p += len;
    return s;
}

static bool read_u32(const uint8_t **p, const uint8_t *end, uint32_t *val) {
    if (*p + 4 > end) return false;
    memcpy(val, *p, 4);
    *p += 4;
    return true;
}

static bool read_i32(const uint8_t **p, const uint8_t *end, int32_t *val) {
    return read_u32(p, end, (uint32_t *)val);
}

XsDebugInfo *xs_debug_info_deserialize(const uint8_t *buf, size_t size) {
    if (!buf || size < 8) return NULL;

    const uint8_t *p = buf;
    const uint8_t *end = buf + size;

    uint32_t magic, version;
    if (!read_u32(&p, end, &magic)) return NULL;
    if (magic != 0x49445358) return NULL;  /* "XSDI" */
    if (!read_u32(&p, end, &version)) return NULL;
    if (version != 1) return NULL;

    char *module_name = read_string(&p, end);
    char *source_file = read_string(&p, end);
    char *source_text = read_string(&p, end);

    if (!module_name || !source_file) {
        free(module_name);
        free(source_file);
        free(source_text);
        return NULL;
    }

    XsDebugInfo *info = xs_debug_info_new(module_name, source_file);
    free(module_name);
    free(source_file);

    if (!info) {
        free(source_text);
        return NULL;
    }

    if (source_text) {
        xs_debug_info_set_source(info, source_text);
        free(source_text);
    }

    /* Source map */
    uint32_t sm_count;
    if (!read_u32(&p, end, &sm_count)) goto fail;
    for (uint32_t i = 0; i < sm_count; i++) {
        uint32_t offset;
        int32_t line, col;
        if (!read_u32(&p, end, &offset)) goto fail;
        if (!read_i32(&p, end, &line)) goto fail;
        if (!read_i32(&p, end, &col)) goto fail;
        xs_source_map_add(&info->source_map, offset, info->source_file, line, col);
    }

    /* Functions */
    uint32_t func_count;
    if (!read_u32(&p, end, &func_count)) goto fail;
    for (uint32_t i = 0; i < func_count; i++) {
        char *fname = read_string(&p, end);
        char *ffile = read_string(&p, end);
        int32_t ls, le;
        uint32_t bc_start, bc_len;
        if (!read_i32(&p, end, &ls)) { free(fname); free(ffile); goto fail; }
        if (!read_i32(&p, end, &le)) { free(fname); free(ffile); goto fail; }
        if (!read_u32(&p, end, &bc_start)) { free(fname); free(ffile); goto fail; }
        if (!read_u32(&p, end, &bc_len)) { free(fname); free(ffile); goto fail; }

        XsFuncDebugInfo *f = xs_debug_info_add_func(info, fname, ffile, ls, le, bc_start);
        free(fname);
        free(ffile);
        if (!f) goto fail;
        f->bytecode_length = bc_len;

        uint32_t pc;
        if (!read_u32(&p, end, &pc)) goto fail;
        for (uint32_t j = 0; j < pc; j++) {
            char *pn = read_string(&p, end);
            char *pt = read_string(&p, end);
            xs_func_debug_add_param(f, pn, pt);
            free(pn);
            free(pt);
        }

        uint32_t lc;
        if (!read_u32(&p, end, &lc)) goto fail;
        for (uint32_t j = 0; j < lc; j++) {
            char *ln = read_string(&p, end);
            char *lt = read_string(&p, end);
            int32_t slot, so, eo, ld;
            if (!read_i32(&p, end, &slot)) { free(ln); free(lt); goto fail; }
            if (!read_i32(&p, end, &so)) { free(ln); free(lt); goto fail; }
            if (!read_i32(&p, end, &eo)) { free(ln); free(lt); goto fail; }
            if (!read_i32(&p, end, &ld)) { free(ln); free(lt); goto fail; }
            xs_func_debug_add_local(f, ln, lt, slot, so, eo, ld);
            free(ln);
            free(lt);
        }
    }

    /* Globals */
    uint32_t gc;
    if (!read_u32(&p, end, &gc)) goto fail;
    for (uint32_t i = 0; i < gc; i++) {
        char *gn = read_string(&p, end);
        char *gt = read_string(&p, end);
        int32_t gs;
        if (!read_i32(&p, end, &gs)) { free(gn); free(gt); goto fail; }
        xs_debug_info_add_global(info, gn, gt, gs);
        free(gn);
        free(gt);
    }

    return info;

fail:
    xs_debug_info_free(info);
    return NULL;
}
