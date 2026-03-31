/*
 * X# (Xsharp) Debug Information
 * ==============================
 * Source maps and debug metadata for mapping bytecode to source.
 */

#ifndef XSHARP_DEBUG_INFO_H
#define XSHARP_DEBUG_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ===== Source Location ===== */
typedef struct {
    const char* file; /* source file path */
    int line;         /* 1-based line number */
    int column;       /* 1-based column number */
} XsSourceLoc;

/* ===== Source Map Entry ===== */
/* Maps a bytecode offset to a source location */
typedef struct {
    uint32_t bytecode_offset;
    XsSourceLoc loc;
} XsSourceMapEntry;

/* ===== Source Map ===== */
typedef struct {
    XsSourceMapEntry* entries;
    int count;
    int capacity;
} XsSourceMap;

/* ===== Variable Debug Info ===== */
typedef enum {
    VAR_SCOPE_LOCAL,
    VAR_SCOPE_GLOBAL,
    VAR_SCOPE_PARAMETER,
    VAR_SCOPE_UPVALUE
} XsVarScope;

typedef struct {
    char* name;
    char* type_name; /* e.g. "blade", "scroll" */
    XsVarScope scope;
    int slot;         /* stack slot or global index */
    int start_offset; /* bytecode offset where var becomes live */
    int end_offset;   /* bytecode offset where var goes out of scope */
    int line_defined;
} XsVarDebugInfo;

/* ===== Function Debug Info ===== */
typedef struct {
    char* name;
    char* source_file;
    int line_start;
    int line_end;
    int param_count;
    char** param_names;
    char** param_types;
    int local_count;
    XsVarDebugInfo* locals;
    int locals_capacity;
    uint32_t bytecode_start; /* offset in bytecode chunk */
    uint32_t bytecode_length;
} XsFuncDebugInfo;

/* ===== Module Debug Info (top-level container) ===== */
typedef struct {
    char* module_name;
    char* source_file;
    char* source_text; /* full source for display */
    int source_line_count;

    XsSourceMap source_map;

    XsFuncDebugInfo* functions;
    int func_count;
    int func_capacity;

    XsVarDebugInfo* globals;
    int global_count;
    int global_capacity;
} XsDebugInfo;

/* ===== API ===== */

/* Create / destroy */
XsDebugInfo* xs_debug_info_new(const char* module_name, const char* source_file);
void xs_debug_info_free(XsDebugInfo* info);

/* Source map */
void xs_source_map_init(XsSourceMap* map);
void xs_source_map_free(XsSourceMap* map);
void xs_source_map_add(XsSourceMap* map, uint32_t offset, const char* file, int line, int column);
XsSourceLoc* xs_source_map_lookup(XsSourceMap* map, uint32_t offset);
uint32_t xs_source_map_find_offset(XsSourceMap* map, const char* file, int line);

/* Function debug info */
XsFuncDebugInfo* xs_debug_info_add_func(XsDebugInfo* info, const char* name,
                                        const char* source_file, int line_start, int line_end,
                                        uint32_t bytecode_start);
void xs_func_debug_add_param(XsFuncDebugInfo* func, const char* name, const char* type);
void xs_func_debug_add_local(XsFuncDebugInfo* func, const char* name, const char* type, int slot,
                             int start_off, int end_off, int line_defined);
XsFuncDebugInfo* xs_debug_info_find_func(XsDebugInfo* info, uint32_t offset);
XsFuncDebugInfo* xs_debug_info_find_func_by_name(XsDebugInfo* info, const char* name);

/* Global variable debug info */
void xs_debug_info_add_global(XsDebugInfo* info, const char* name, const char* type, int slot);

/* Source text */
void xs_debug_info_set_source(XsDebugInfo* info, const char* src);
const char* xs_debug_info_get_line(XsDebugInfo* info, int line, int* out_length);

/* Serialization (binary format) */
bool xs_debug_info_serialize(XsDebugInfo* info, uint8_t** out_buf, size_t* out_size);
XsDebugInfo* xs_debug_info_deserialize(const uint8_t* buf, size_t size);

#endif /* XSHARP_DEBUG_INFO_H */
