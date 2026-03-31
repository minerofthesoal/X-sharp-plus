/*
 * X# (Xsharp) Interactive Debugger
 * ==================================
 * Full-featured debugger with breakpoints, stepping, call stack,
 * variable inspection, watch expressions, and memory inspection.
 */

#ifndef XSHARP_DEBUGGER_H
#define XSHARP_DEBUGGER_H

#include "../runtime/runtime.h"
#include "breakpoint.h"
#include "debug_info.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ===== Constants ===== */
#define XS_DBG_MAX_CALL_STACK 256
#define XS_DBG_MAX_WATCH 64
#define XS_DBG_MAX_EXPR_LEN 512
#define XS_DBG_MAX_VAR_NAME 128
#define XS_DBG_MEMORY_DUMP_WIDTH 16

/* ===== Execution State ===== */
typedef enum {
    DBG_STATE_IDLE,          /* not started */
    DBG_STATE_RUNNING,       /* executing freely */
    DBG_STATE_PAUSED,        /* paused at breakpoint or step */
    DBG_STATE_STEPPING_IN,   /* single-step into calls */
    DBG_STATE_STEPPING_OVER, /* step over calls */
    DBG_STATE_STEPPING_OUT,  /* step out of current function */
    DBG_STATE_STOPPED,       /* execution finished or terminated */
    DBG_STATE_ERROR          /* stopped due to error */
} XsDbgState;

/* ===== Stop Reason ===== */
typedef enum {
    DBG_STOP_NONE,
    DBG_STOP_BREAKPOINT,
    DBG_STOP_STEP,
    DBG_STOP_PAUSE, /* user requested pause */
    DBG_STOP_EXCEPTION,
    DBG_STOP_ENTRY, /* stopped at program entry */
    DBG_STOP_EXIT,  /* program exited */
    DBG_STOP_ERROR
} XsDbgStopReason;

/* ===== Call Frame ===== */
typedef struct {
    int id;
    char func_name[XS_DBG_MAX_VAR_NAME];
    char source_file[XS_MAX_FILE_PATH];
    int line;
    int column;
    uint32_t bytecode_offset;
    uint32_t frame_base;        /* stack frame base pointer */
    XsFuncDebugInfo* func_info; /* debug info for this function */
} XsDbgCallFrame;

/* ===== Watch Expression ===== */
typedef struct {
    int id;
    char expression[XS_DBG_MAX_EXPR_LEN];
    XsValue last_value;
    bool valid;      /* true if last evaluation succeeded */
    char error[256]; /* error message if evaluation failed */
    bool enabled;
} XsDbgWatch;

/* ===== Variable Display ===== */
typedef struct {
    char name[XS_DBG_MAX_VAR_NAME];
    char type_name[64];
    XsValue value;
    XsVarScope scope;
    int slot;
    int frame_id; /* which call frame it belongs to */
} XsDbgVariable;

/* ===== Debug Event Types ===== */
typedef enum {
    DBG_EVENT_STOPPED,
    DBG_EVENT_CONTINUED,
    DBG_EVENT_BREAKPOINT_HIT,
    DBG_EVENT_STEP_COMPLETE,
    DBG_EVENT_EXCEPTION,
    DBG_EVENT_OUTPUT,
    DBG_EVENT_TERMINATED,
    DBG_EVENT_THREAD_STARTED,
    DBG_EVENT_THREAD_EXITED,
    DBG_EVENT_BREAKPOINT_RESOLVED,
    DBG_EVENT_LOADED_SOURCE,
    DBG_EVENT_MODULE_LOADED
} XsDbgEventType;

/* ===== Debug Event ===== */
typedef struct {
    XsDbgEventType type;
    XsDbgStopReason reason;
    int thread_id;
    int breakpoint_id;
    char text[1024];
    int line;
    char file[XS_MAX_FILE_PATH];
} XsDbgEvent;

/* ===== Event Callback ===== */
typedef void (*XsDbgEventCallback)(const XsDbgEvent* event, void* user_data);

/* ===== Simulated VM State (for debugger integration) ===== */
typedef struct {
    uint8_t* bytecode;
    size_t bytecode_size;
    uint32_t ip; /* instruction pointer */
    XsValue* stack;
    int stack_top;
    int stack_capacity;
    XsValue* globals;
    int global_count;
    int global_capacity;
    uint32_t* call_returns; /* return addresses */
    uint32_t* frame_bases;  /* frame base pointers */
    int call_depth;
    bool halted;
    int exit_code;
} XsDbgVMState;

/* ===== Main Debugger ===== */
typedef struct {
    /* Execution state */
    XsDbgState state;
    XsDbgStopReason stop_reason;

    /* VM connection */
    XsDbgVMState vm;

    /* Debug information */
    XsDebugInfo* debug_info;

    /* Breakpoint management */
    XsBreakpointManager bp_manager;

    /* Call stack */
    XsDbgCallFrame call_stack[XS_DBG_MAX_CALL_STACK];
    int call_stack_depth;

    /* Watch expressions */
    XsDbgWatch watches[XS_DBG_MAX_WATCH];
    int watch_count;
    int next_watch_id;

    /* Stepping state */
    int step_start_depth; /* call depth when step started */
    uint32_t step_start_offset;
    int step_start_line;
    uint32_t run_to_offset; /* for run-to-cursor */
    bool run_to_active;

    /* Event callback */
    XsDbgEventCallback event_callback;
    void* callback_data;

    /* Configuration */
    bool stop_on_entry;     /* pause at program start */
    bool stop_on_exception; /* pause on exceptions */
    bool source_stepping;   /* step by source lines */

    /* Current location cache */
    char current_file[XS_MAX_FILE_PATH];
    int current_line;
    int current_column;
} XsDebugger;

/* ===== Lifecycle ===== */
XsDebugger* xs_debugger_new(void);
void xs_debugger_free(XsDebugger* dbg);

/* ===== Program Loading ===== */
bool xs_debugger_load(XsDebugger* dbg, const uint8_t* bytecode, size_t size, XsDebugInfo* info);
bool xs_debugger_load_file(XsDebugger* dbg, const char* path);

/* ===== Execution Control ===== */
bool xs_debugger_launch(XsDebugger* dbg);
bool xs_debugger_continue(XsDebugger* dbg);
bool xs_debugger_pause(XsDebugger* dbg);
bool xs_debugger_step_in(XsDebugger* dbg);
bool xs_debugger_step_over(XsDebugger* dbg);
bool xs_debugger_step_out(XsDebugger* dbg);
bool xs_debugger_run_to_cursor(XsDebugger* dbg, const char* file, int line);
bool xs_debugger_stop(XsDebugger* dbg);
bool xs_debugger_restart(XsDebugger* dbg);

/* Execute a single bytecode instruction (used internally) */
bool xs_debugger_execute_one(XsDebugger* dbg);

/* ===== Breakpoints ===== */
int xs_debugger_set_breakpoint(XsDebugger* dbg, const char* file, int line);
int xs_debugger_set_conditional_bp(XsDebugger* dbg, const char* file, int line,
                                   const char* condition);
int xs_debugger_set_hit_count_bp(XsDebugger* dbg, const char* file, int line, int count,
                                 XsHitCountMode mode);
bool xs_debugger_remove_breakpoint(XsDebugger* dbg, int bp_id);
bool xs_debugger_enable_breakpoint(XsDebugger* dbg, int bp_id);
bool xs_debugger_disable_breakpoint(XsDebugger* dbg, int bp_id);

/* ===== Call Stack ===== */
int xs_debugger_get_call_stack(XsDebugger* dbg, XsDbgCallFrame* out, int max);
XsDbgCallFrame* xs_debugger_get_frame(XsDebugger* dbg, int frame_id);

/* ===== Variables ===== */
int xs_debugger_get_locals(XsDebugger* dbg, int frame_id, XsDbgVariable* out, int max);
int xs_debugger_get_globals(XsDebugger* dbg, XsDbgVariable* out, int max);
bool xs_debugger_get_variable(XsDebugger* dbg, const char* name, int frame_id, XsDbgVariable* out);

/* ===== Watch Expressions ===== */
int xs_debugger_add_watch(XsDebugger* dbg, const char* expression);
bool xs_debugger_remove_watch(XsDebugger* dbg, int watch_id);
bool xs_debugger_eval_watch(XsDebugger* dbg, int watch_id);
void xs_debugger_eval_all_watches(XsDebugger* dbg);
XsValue xs_debugger_evaluate(XsDebugger* dbg, const char* expression, int frame_id, bool* success);

/* ===== Memory Inspection ===== */
int xs_debugger_read_memory(XsDebugger* dbg, uint32_t address, uint8_t* buf, int size);
void xs_debugger_dump_memory(XsDebugger* dbg, uint32_t address, int size);

/* ===== Stack Inspection ===== */
int xs_debugger_get_stack(XsDebugger* dbg, XsValue* out, int max);
void xs_debugger_dump_stack(XsDebugger* dbg);

/* ===== Source Display ===== */
const char* xs_debugger_get_source_line(XsDebugger* dbg, int line, int* length);
void xs_debugger_show_source_context(XsDebugger* dbg, int center_line, int context_lines);

/* ===== Events ===== */
void xs_debugger_set_event_callback(XsDebugger* dbg, XsDbgEventCallback cb, void* data);
void xs_debugger_emit_event(XsDebugger* dbg, const XsDbgEvent* event);

/* ===== State Queries ===== */
XsDbgState xs_debugger_get_state(XsDebugger* dbg);
XsDbgStopReason xs_debugger_get_stop_reason(XsDebugger* dbg);
void xs_debugger_get_location(XsDebugger* dbg, char* file, int* line, int* column);
uint32_t xs_debugger_get_ip(XsDebugger* dbg);

/* ===== Utility ===== */
void xs_debugger_value_to_string(XsValue val, char* buf, size_t buf_size);
void xs_debugger_print_value(XsValue val);

#endif /* XSHARP_DEBUGGER_H */
