/*
 * X# (Xsharp) Breakpoint System - Implementation
 * =================================================
 * Hash table keyed by "file:line" with linked-list collision chains.
 */

#include "breakpoint.h"
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

/* ===== Hash Function ===== */

static uint32_t bp_hash(const char *file, int line) {
    /* FNV-1a hash of "file:line" */
    uint32_t h = 2166136261u;
    if (file) {
        for (const char *p = file; *p; p++) {
            h ^= (uint8_t)*p;
            h *= 16777619u;
        }
    }
    h ^= ':';
    h *= 16777619u;
    /* Hash the line number bytes */
    uint32_t ln = (uint32_t)line;
    for (int i = 0; i < 4; i++) {
        h ^= (ln >> (i * 8)) & 0xFF;
        h *= 16777619u;
    }
    return h % XS_BP_HASH_SIZE;
}

/* ===== Init / Free ===== */

void xs_bp_manager_init(XsBreakpointManager *mgr) {
    if (!mgr) return;
    memset(mgr->hash_table, 0, sizeof(mgr->hash_table));
    memset(mgr->all, 0, sizeof(mgr->all));
    mgr->count = 0;
    mgr->next_id = 1;
}

void xs_bp_manager_free(XsBreakpointManager *mgr) {
    if (!mgr) return;
    for (int i = 0; i < XS_MAX_BREAKPOINTS; i++) {
        if (mgr->all[i]) {
            free(mgr->all[i]);
            mgr->all[i] = NULL;
        }
    }
    memset(mgr->hash_table, 0, sizeof(mgr->hash_table));
    mgr->count = 0;
}

/* ===== Internal: allocate and register a breakpoint ===== */

static XsBreakpoint *bp_alloc(XsBreakpointManager *mgr) {
    if (mgr->count >= XS_MAX_BREAKPOINTS) return NULL;

    XsBreakpoint *bp = (XsBreakpoint *)calloc(1, sizeof(XsBreakpoint));
    if (!bp) return NULL;

    bp->id = mgr->next_id++;
    bp->state = BP_STATE_ENABLED;
    bp->bytecode_offset = UINT32_MAX;

    /* Find empty slot in all[] */
    for (int i = 0; i < XS_MAX_BREAKPOINTS; i++) {
        if (!mgr->all[i]) {
            mgr->all[i] = bp;
            break;
        }
    }
    mgr->count++;

    return bp;
}

static void bp_insert_hash(XsBreakpointManager *mgr, XsBreakpoint *bp) {
    uint32_t idx = bp_hash(bp->file, bp->line);
    bp->next = mgr->hash_table[idx];
    mgr->hash_table[idx] = bp;
}

/* ===== Add breakpoints ===== */

int xs_bp_add_line(XsBreakpointManager *mgr, const char *file, int line) {
    if (!mgr || !file) return -1;

    XsBreakpoint *bp = bp_alloc(mgr);
    if (!bp) return -1;

    bp->type = BP_LINE;
    strncpy(bp->file, file, XS_MAX_FILE_PATH - 1);
    bp->file[XS_MAX_FILE_PATH - 1] = '\0';
    bp->line = line;

    bp_insert_hash(mgr, bp);
    return bp->id;
}

int xs_bp_add_conditional(XsBreakpointManager *mgr, const char *file,
                           int line, const char *condition) {
    if (!mgr || !file || !condition) return -1;

    XsBreakpoint *bp = bp_alloc(mgr);
    if (!bp) return -1;

    bp->type = BP_CONDITIONAL;
    strncpy(bp->file, file, XS_MAX_FILE_PATH - 1);
    bp->file[XS_MAX_FILE_PATH - 1] = '\0';
    bp->line = line;
    strncpy(bp->condition, condition, XS_MAX_CONDITION_LEN - 1);
    bp->condition[XS_MAX_CONDITION_LEN - 1] = '\0';

    bp_insert_hash(mgr, bp);
    return bp->id;
}

int xs_bp_add_hit_count(XsBreakpointManager *mgr, const char *file,
                         int line, int target, XsHitCountMode mode) {
    if (!mgr || !file) return -1;

    XsBreakpoint *bp = bp_alloc(mgr);
    if (!bp) return -1;

    bp->type = BP_HIT_COUNT;
    strncpy(bp->file, file, XS_MAX_FILE_PATH - 1);
    bp->file[XS_MAX_FILE_PATH - 1] = '\0';
    bp->line = line;
    bp->hit_target = target;
    bp->hit_mode = mode;

    bp_insert_hash(mgr, bp);
    return bp->id;
}

int xs_bp_add_logpoint(XsBreakpointManager *mgr, const char *file,
                        int line, const char *message) {
    if (!mgr || !file || !message) return -1;

    XsBreakpoint *bp = bp_alloc(mgr);
    if (!bp) return -1;

    bp->type = BP_LOG;
    strncpy(bp->file, file, XS_MAX_FILE_PATH - 1);
    bp->file[XS_MAX_FILE_PATH - 1] = '\0';
    bp->line = line;
    strncpy(bp->log_message, message, XS_MAX_CONDITION_LEN - 1);
    bp->log_message[XS_MAX_CONDITION_LEN - 1] = '\0';

    bp_insert_hash(mgr, bp);
    return bp->id;
}

/* ===== Remove / Modify ===== */

static void bp_remove_from_hash(XsBreakpointManager *mgr, XsBreakpoint *bp) {
    uint32_t idx = bp_hash(bp->file, bp->line);
    XsBreakpoint **pp = &mgr->hash_table[idx];
    while (*pp) {
        if (*pp == bp) {
            *pp = bp->next;
            bp->next = NULL;
            return;
        }
        pp = &(*pp)->next;
    }
}

bool xs_bp_remove(XsBreakpointManager *mgr, int id) {
    if (!mgr) return false;

    XsBreakpoint *bp = xs_bp_find_by_id(mgr, id);
    if (!bp) return false;

    bp_remove_from_hash(mgr, bp);

    /* Remove from all[] */
    for (int i = 0; i < XS_MAX_BREAKPOINTS; i++) {
        if (mgr->all[i] == bp) {
            mgr->all[i] = NULL;
            break;
        }
    }
    mgr->count--;

    free(bp);
    return true;
}

bool xs_bp_enable(XsBreakpointManager *mgr, int id) {
    XsBreakpoint *bp = xs_bp_find_by_id(mgr, id);
    if (!bp) return false;
    bp->state = BP_STATE_ENABLED;
    return true;
}

bool xs_bp_disable(XsBreakpointManager *mgr, int id) {
    XsBreakpoint *bp = xs_bp_find_by_id(mgr, id);
    if (!bp) return false;
    bp->state = BP_STATE_DISABLED;
    return true;
}

bool xs_bp_set_condition(XsBreakpointManager *mgr, int id,
                          const char *condition) {
    XsBreakpoint *bp = xs_bp_find_by_id(mgr, id);
    if (!bp || !condition) return false;

    bp->type = BP_CONDITIONAL;
    strncpy(bp->condition, condition, XS_MAX_CONDITION_LEN - 1);
    bp->condition[XS_MAX_CONDITION_LEN - 1] = '\0';
    return true;
}

bool xs_bp_set_hit_count(XsBreakpointManager *mgr, int id,
                          int target, XsHitCountMode mode) {
    XsBreakpoint *bp = xs_bp_find_by_id(mgr, id);
    if (!bp) return false;

    bp->type = BP_HIT_COUNT;
    bp->hit_target = target;
    bp->hit_mode = mode;
    return true;
}

/* ===== Lookup ===== */

XsBreakpoint *xs_bp_find_by_id(XsBreakpointManager *mgr, int id) {
    if (!mgr || id <= 0) return NULL;

    for (int i = 0; i < XS_MAX_BREAKPOINTS; i++) {
        if (mgr->all[i] && mgr->all[i]->id == id) {
            return mgr->all[i];
        }
    }
    return NULL;
}

XsBreakpoint *xs_bp_find_by_location(XsBreakpointManager *mgr,
                                      const char *file, int line) {
    if (!mgr || !file) return NULL;

    uint32_t idx = bp_hash(file, line);
    XsBreakpoint *bp = mgr->hash_table[idx];
    while (bp) {
        if (bp->line == line && strcmp(bp->file, file) == 0 &&
            bp->state != BP_STATE_DELETED) {
            return bp;
        }
        bp = bp->next;
    }
    return NULL;
}

XsBreakpoint *xs_bp_find_by_offset(XsBreakpointManager *mgr, uint32_t offset) {
    if (!mgr) return NULL;

    for (int i = 0; i < XS_MAX_BREAKPOINTS; i++) {
        if (mgr->all[i] && mgr->all[i]->bytecode_offset == offset &&
            mgr->all[i]->state == BP_STATE_ENABLED) {
            return mgr->all[i];
        }
    }
    return NULL;
}

/* ===== Hit checking ===== */

bool xs_bp_should_break(XsBreakpoint *bp) {
    if (!bp) return false;
    if (bp->state != BP_STATE_ENABLED) return false;

    switch (bp->type) {
        case BP_LINE:
            return true;

        case BP_CONDITIONAL:
            /* Condition evaluation is done externally;
               if we get here, the condition was true */
            return true;

        case BP_HIT_COUNT:
            switch (bp->hit_mode) {
                case HC_EQUAL:
                    return bp->hit_count == bp->hit_target;
                case HC_GREATER_EQUAL:
                    return bp->hit_count >= bp->hit_target;
                case HC_MULTIPLE:
                    return bp->hit_target > 0 &&
                           (bp->hit_count % bp->hit_target) == 0;
            }
            return false;

        case BP_LOG:
            /* Logpoints never stop execution */
            return false;
    }

    return false;
}

void xs_bp_record_hit(XsBreakpoint *bp) {
    if (bp) {
        bp->hit_count++;
    }
}

/* ===== Clear all ===== */

void xs_bp_clear_all(XsBreakpointManager *mgr) {
    if (!mgr) return;

    for (int i = 0; i < XS_MAX_BREAKPOINTS; i++) {
        if (mgr->all[i]) {
            free(mgr->all[i]);
            mgr->all[i] = NULL;
        }
    }
    memset(mgr->hash_table, 0, sizeof(mgr->hash_table));
    mgr->count = 0;
}

/* ===== Iteration ===== */

int xs_bp_get_all(XsBreakpointManager *mgr, XsBreakpoint **out, int max) {
    if (!mgr || !out || max <= 0) return 0;

    int n = 0;
    for (int i = 0; i < XS_MAX_BREAKPOINTS && n < max; i++) {
        if (mgr->all[i]) {
            out[n++] = mgr->all[i];
        }
    }
    return n;
}

/* ===== Resolve offsets ===== */

void xs_bp_resolve_offsets(XsBreakpointManager *mgr, struct XsSourceMap *map) {
    if (!mgr || !map) return;

    /* Cast to the actual type - XsSourceMap is defined in debug_info.h */
    XsSourceMap *smap = (XsSourceMap *)map;

    for (int i = 0; i < XS_MAX_BREAKPOINTS; i++) {
        XsBreakpoint *bp = mgr->all[i];
        if (!bp) continue;
        if (bp->bytecode_offset != UINT32_MAX) continue; /* already resolved */

        /* Search the source map for matching file:line */
        for (int j = 0; j < smap->count; j++) {
            XsSourceMapEntry *e = &smap->entries[j];
            if (e->loc.line == bp->line) {
                if (!bp->file[0] || !e->loc.file ||
                    strcmp(e->loc.file, bp->file) == 0) {
                    bp->bytecode_offset = e->bytecode_offset;
                    break;
                }
            }
        }
    }
}
