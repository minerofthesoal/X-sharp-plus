/*
 * X# (Xsharp) Breakpoint System
 * ===============================
 * Line, conditional, and hit-count breakpoints with hash table storage.
 */

#ifndef XSHARP_BREAKPOINT_H
#define XSHARP_BREAKPOINT_H

#include <stdint.h>
#include <stdbool.h>

/* Maximum breakpoints */
#define XS_MAX_BREAKPOINTS    256
#define XS_BP_HASH_SIZE       128
#define XS_MAX_CONDITION_LEN  256
#define XS_MAX_FILE_PATH      512

/* ===== Breakpoint Type ===== */
typedef enum {
    BP_LINE,            /* break at a specific line */
    BP_CONDITIONAL,     /* break when condition is true */
    BP_HIT_COUNT,       /* break after N hits */
    BP_LOG              /* log message without stopping (logpoint) */
} XsBreakpointType;

/* ===== Breakpoint State ===== */
typedef enum {
    BP_STATE_ENABLED,
    BP_STATE_DISABLED,
    BP_STATE_DELETED
} XsBreakpointState;

/* ===== Hit Count Mode ===== */
typedef enum {
    HC_EQUAL,           /* break when hit_count == target */
    HC_GREATER_EQUAL,   /* break when hit_count >= target */
    HC_MULTIPLE         /* break when hit_count % target == 0 */
} XsHitCountMode;

/* ===== Breakpoint ===== */
typedef struct XsBreakpoint {
    int                 id;             /* unique breakpoint id */
    XsBreakpointType    type;
    XsBreakpointState   state;

    /* Location */
    char                file[XS_MAX_FILE_PATH];
    int                 line;
    int                 column;         /* 0 = any column */
    uint32_t            bytecode_offset; /* resolved offset, UINT32_MAX if unresolved */

    /* Condition (for BP_CONDITIONAL) */
    char                condition[XS_MAX_CONDITION_LEN];

    /* Hit count */
    int                 hit_count;      /* current hits */
    int                 hit_target;     /* target for BP_HIT_COUNT */
    XsHitCountMode      hit_mode;

    /* Log message (for BP_LOG) */
    char                log_message[XS_MAX_CONDITION_LEN];

    /* Linked list for hash collision */
    struct XsBreakpoint *next;
} XsBreakpoint;

/* ===== Breakpoint Manager ===== */
typedef struct {
    XsBreakpoint       *hash_table[XS_BP_HASH_SIZE]; /* file:line -> bp chain */
    XsBreakpoint       *all[XS_MAX_BREAKPOINTS];     /* flat array for id lookup */
    int                 count;
    int                 next_id;
} XsBreakpointManager;

/* ===== API ===== */

/* Initialize / cleanup */
void xs_bp_manager_init(XsBreakpointManager *mgr);
void xs_bp_manager_free(XsBreakpointManager *mgr);

/* Add breakpoints */
int  xs_bp_add_line(XsBreakpointManager *mgr, const char *file, int line);
int  xs_bp_add_conditional(XsBreakpointManager *mgr, const char *file,
                            int line, const char *condition);
int  xs_bp_add_hit_count(XsBreakpointManager *mgr, const char *file,
                          int line, int target, XsHitCountMode mode);
int  xs_bp_add_logpoint(XsBreakpointManager *mgr, const char *file,
                         int line, const char *message);

/* Remove / modify */
bool xs_bp_remove(XsBreakpointManager *mgr, int id);
bool xs_bp_enable(XsBreakpointManager *mgr, int id);
bool xs_bp_disable(XsBreakpointManager *mgr, int id);
bool xs_bp_set_condition(XsBreakpointManager *mgr, int id,
                          const char *condition);
bool xs_bp_set_hit_count(XsBreakpointManager *mgr, int id,
                          int target, XsHitCountMode mode);

/* Lookup */
XsBreakpoint *xs_bp_find_by_id(XsBreakpointManager *mgr, int id);
XsBreakpoint *xs_bp_find_by_location(XsBreakpointManager *mgr,
                                      const char *file, int line);
XsBreakpoint *xs_bp_find_by_offset(XsBreakpointManager *mgr, uint32_t offset);

/* Check if a breakpoint should trigger (handles hit counts, conditions checked externally) */
bool xs_bp_should_break(XsBreakpoint *bp);

/* Record a hit on a breakpoint */
void xs_bp_record_hit(XsBreakpoint *bp);

/* Clear all breakpoints */
void xs_bp_clear_all(XsBreakpointManager *mgr);

/* Iteration */
int  xs_bp_get_all(XsBreakpointManager *mgr, XsBreakpoint **out, int max);

/* Resolve bytecode offsets using a source map */
struct XsSourceMap;
void xs_bp_resolve_offsets(XsBreakpointManager *mgr, struct XsSourceMap *map);

#endif /* XSHARP_BREAKPOINT_H */
