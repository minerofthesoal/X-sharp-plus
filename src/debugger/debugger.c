/*
 * X# (Xsharp) Debugger Implementation
 */

#include "debugger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Lifecycle ===== */
XsDebugger *xs_debugger_new(void) {
    XsDebugger *dbg = calloc(1, sizeof(XsDebugger));
    dbg->state = DBG_STATE_IDLE;
    dbg->stop_reason = DBG_STOP_NONE;
    xs_bp_manager_init(&dbg->bp_manager);
    dbg->watch_count = 0;
    dbg->next_watch_id = 1;
    dbg->call_stack_depth = 0;
    dbg->stop_on_entry = false;
    dbg->stop_on_exception = true;
    dbg->source_stepping = true;
    dbg->event_callback = NULL;
    dbg->callback_data = NULL;
    dbg->vm.stack_capacity = 4096;
    dbg->vm.stack = calloc(dbg->vm.stack_capacity, sizeof(XsValue));
    dbg->vm.stack_top = 0;
    dbg->vm.global_capacity = 256;
    dbg->vm.globals = calloc(dbg->vm.global_capacity, sizeof(XsValue));
    dbg->vm.global_count = 0;
    dbg->vm.call_returns = calloc(256, sizeof(uint32_t));
    dbg->vm.frame_bases = calloc(256, sizeof(uint32_t));
    dbg->vm.call_depth = 0;
    dbg->vm.halted = false;
    return dbg;
}

void xs_debugger_free(XsDebugger *dbg) {
    if (!dbg) return;
    xs_bp_manager_free(&dbg->bp_manager);
    free(dbg->vm.stack);
    free(dbg->vm.globals);
    free(dbg->vm.call_returns);
    free(dbg->vm.frame_bases);
    free(dbg);
}

/* ===== Program loading ===== */
bool xs_debugger_load(XsDebugger *dbg, const uint8_t *bytecode,
                      size_t size, XsDebugInfo *info) {
    dbg->vm.bytecode = (uint8_t *)malloc(size);
    memcpy(dbg->vm.bytecode, bytecode, size);
    dbg->vm.bytecode_size = size;
    dbg->vm.ip = 0;
    dbg->debug_info = info;
    dbg->state = DBG_STATE_IDLE;
    return true;
}

bool xs_debugger_load_file(XsDebugger *dbg, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    uint8_t *buf = malloc(size);
    fread(buf, 1, size, f);
    fclose(f);
    bool ok = xs_debugger_load(dbg, buf, size, NULL);
    free(buf);
    return ok;
}

/* ===== Location update ===== */
static void update_location(XsDebugger *dbg) {
    if (!dbg->debug_info) return;
    XsSourceLoc *loc = xs_source_map_lookup(&dbg->debug_info->source_map, dbg->vm.ip);
    if (loc) {
        if (loc->file) strncpy(dbg->current_file, loc->file, XS_MAX_FILE_PATH - 1);
        dbg->current_line = loc->line;
        dbg->current_column = loc->column;
    }
}

static void emit_stopped(XsDebugger *dbg, XsDbgStopReason reason) {
    dbg->state = DBG_STATE_PAUSED;
    dbg->stop_reason = reason;
    update_location(dbg);
    if (dbg->event_callback) {
        XsDbgEvent event = {0};
        event.type = DBG_EVENT_STOPPED;
        event.reason = reason;
        event.line = dbg->current_line;
        strncpy(event.file, dbg->current_file, XS_MAX_FILE_PATH - 1);
        dbg->event_callback(&event, dbg->callback_data);
    }
}

/* ===== Execution control ===== */
bool xs_debugger_launch(XsDebugger *dbg) {
    dbg->vm.ip = 0;
    dbg->vm.halted = false;
    if (dbg->stop_on_entry) {
        emit_stopped(dbg, DBG_STOP_ENTRY);
        return true;
    }
    dbg->state = DBG_STATE_RUNNING;
    return xs_debugger_continue(dbg);
}

bool xs_debugger_execute_one(XsDebugger *dbg) {
    if (dbg->vm.ip >= dbg->vm.bytecode_size || dbg->vm.halted) {
        dbg->state = DBG_STATE_STOPPED;
        return false;
    }
    /* Check breakpoints */
    XsBreakpoint *bp = xs_bp_find_by_offset(&dbg->bp_manager, dbg->vm.ip);
    if (bp && bp->state == BP_STATE_ENABLED && xs_bp_should_break(bp)) {
        xs_bp_record_hit(bp);
        emit_stopped(dbg, DBG_STOP_BREAKPOINT);
        return false;
    }
    /* Execute instruction (simplified) */
    uint8_t op = dbg->vm.bytecode[dbg->vm.ip++];
    /* Basic instruction decoding */
    switch (op) {
        case 0x00: /* OP_PUSH_BLADE */ dbg->vm.ip += 2; dbg->vm.stack_top++; break;
        case 0x01: /* OP_PUSH_SPARK */ dbg->vm.ip += 2; dbg->vm.stack_top++; break;
        case 0x06: /* OP_POP */ if (dbg->vm.stack_top > 0) dbg->vm.stack_top--; break;
        case 0x4A: /* OP_HALT */ dbg->vm.halted = true; break;
        default: /* Skip operands based on opcode class */
            if (op >= 0x24 && op <= 0x29) dbg->vm.ip += 2; /* var ops */
            else if (op >= 0x2A && op <= 0x2D) dbg->vm.ip += 2; /* jumps */
            break;
    }
    update_location(dbg);
    return true;
}

bool xs_debugger_continue(XsDebugger *dbg) {
    dbg->state = DBG_STATE_RUNNING;
    while (dbg->state == DBG_STATE_RUNNING) {
        if (!xs_debugger_execute_one(dbg)) break;
    }
    return true;
}

bool xs_debugger_pause(XsDebugger *dbg) {
    emit_stopped(dbg, DBG_STOP_PAUSE);
    return true;
}

bool xs_debugger_step_in(XsDebugger *dbg) {
    dbg->state = DBG_STATE_STEPPING_IN;
    int start_line = dbg->current_line;
    while (dbg->state == DBG_STATE_STEPPING_IN) {
        if (!xs_debugger_execute_one(dbg)) break;
        if (dbg->current_line != start_line) {
            emit_stopped(dbg, DBG_STOP_STEP);
            break;
        }
    }
    return true;
}

bool xs_debugger_step_over(XsDebugger *dbg) {
    dbg->state = DBG_STATE_STEPPING_OVER;
    int start_depth = dbg->vm.call_depth;
    int start_line = dbg->current_line;
    while (dbg->state == DBG_STATE_STEPPING_OVER) {
        if (!xs_debugger_execute_one(dbg)) break;
        if (dbg->vm.call_depth <= start_depth && dbg->current_line != start_line) {
            emit_stopped(dbg, DBG_STOP_STEP);
            break;
        }
    }
    return true;
}

bool xs_debugger_step_out(XsDebugger *dbg) {
    dbg->state = DBG_STATE_STEPPING_OUT;
    int start_depth = dbg->vm.call_depth;
    while (dbg->state == DBG_STATE_STEPPING_OUT) {
        if (!xs_debugger_execute_one(dbg)) break;
        if (dbg->vm.call_depth < start_depth) {
            emit_stopped(dbg, DBG_STOP_STEP);
            break;
        }
    }
    return true;
}

bool xs_debugger_run_to_cursor(XsDebugger *dbg, const char *file, int line) {
    dbg->run_to_active = true;
    if (dbg->debug_info) {
        dbg->run_to_offset = xs_source_map_find_offset(&dbg->debug_info->source_map, file, line);
    }
    dbg->state = DBG_STATE_RUNNING;
    while (dbg->state == DBG_STATE_RUNNING) {
        if (!xs_debugger_execute_one(dbg)) break;
        if (dbg->run_to_active && dbg->vm.ip == dbg->run_to_offset) {
            dbg->run_to_active = false;
            emit_stopped(dbg, DBG_STOP_STEP);
            break;
        }
    }
    return true;
}

bool xs_debugger_stop(XsDebugger *dbg) {
    dbg->state = DBG_STATE_STOPPED;
    dbg->vm.halted = true;
    return true;
}

bool xs_debugger_restart(XsDebugger *dbg) {
    dbg->vm.ip = 0;
    dbg->vm.stack_top = 0;
    dbg->vm.call_depth = 0;
    dbg->vm.halted = false;
    return xs_debugger_launch(dbg);
}

/* ===== Breakpoints ===== */
int xs_debugger_set_breakpoint(XsDebugger *dbg, const char *file, int line) {
    int id = xs_bp_add_line(&dbg->bp_manager, file, line);
    if (dbg->debug_info) {
        XsBreakpoint *bp = xs_bp_find_by_id(&dbg->bp_manager, id);
        if (bp) bp->bytecode_offset = xs_source_map_find_offset(
            &dbg->debug_info->source_map, file, line);
    }
    return id;
}

int xs_debugger_set_conditional_bp(XsDebugger *dbg, const char *file,
                                    int line, const char *condition) {
    return xs_bp_add_conditional(&dbg->bp_manager, file, line, condition);
}

int xs_debugger_set_hit_count_bp(XsDebugger *dbg, const char *file,
                                  int line, int count, XsHitCountMode mode) {
    return xs_bp_add_hit_count(&dbg->bp_manager, file, line, count, mode);
}

bool xs_debugger_remove_breakpoint(XsDebugger *dbg, int bp_id) {
    return xs_bp_remove(&dbg->bp_manager, bp_id);
}

bool xs_debugger_enable_breakpoint(XsDebugger *dbg, int bp_id) {
    return xs_bp_enable(&dbg->bp_manager, bp_id);
}

bool xs_debugger_disable_breakpoint(XsDebugger *dbg, int bp_id) {
    return xs_bp_disable(&dbg->bp_manager, bp_id);
}

/* ===== Call stack ===== */
int xs_debugger_get_call_stack(XsDebugger *dbg, XsDbgCallFrame *out, int max) {
    int count = dbg->call_stack_depth < max ? dbg->call_stack_depth : max;
    if (count > 0) memcpy(out, dbg->call_stack, count * sizeof(XsDbgCallFrame));
    return count;
}

XsDbgCallFrame *xs_debugger_get_frame(XsDebugger *dbg, int frame_id) {
    if (frame_id < 0 || frame_id >= dbg->call_stack_depth) return NULL;
    return &dbg->call_stack[frame_id];
}

/* ===== Variables ===== */
int xs_debugger_get_locals(XsDebugger *dbg, int frame_id,
                           XsDbgVariable *out, int max) {
    (void)frame_id;
    int count = 0;
    if (dbg->debug_info) {
        XsFuncDebugInfo *fi = xs_debug_info_find_func(dbg->debug_info, dbg->vm.ip);
        if (fi) {
            for (int i = 0; i < fi->local_count && count < max; i++) {
                strncpy(out[count].name, fi->locals[i].name, XS_DBG_MAX_VAR_NAME - 1);
                strncpy(out[count].type_name, fi->locals[i].type_name, 63);
                out[count].scope = fi->locals[i].scope;
                out[count].slot = fi->locals[i].slot;
                if (fi->locals[i].slot < dbg->vm.stack_top)
                    out[count].value = dbg->vm.stack[fi->locals[i].slot];
                else
                    out[count].value = (XsValue){.type = VAL_ABYSS};
                count++;
            }
        }
    }
    return count;
}

int xs_debugger_get_globals(XsDebugger *dbg, XsDbgVariable *out, int max) {
    int count = dbg->vm.global_count < max ? dbg->vm.global_count : max;
    for (int i = 0; i < count; i++) {
        snprintf(out[i].name, XS_DBG_MAX_VAR_NAME, "global_%d", i);
        out[i].value = dbg->vm.globals[i];
        out[i].scope = VAR_SCOPE_GLOBAL;
    }
    return count;
}

bool xs_debugger_get_variable(XsDebugger *dbg, const char *name,
                              int frame_id, XsDbgVariable *out) {
    (void)frame_id;
    if (dbg->debug_info) {
        XsFuncDebugInfo *fi = xs_debug_info_find_func(dbg->debug_info, dbg->vm.ip);
        if (fi) {
            for (int i = 0; i < fi->local_count; i++) {
                if (strcmp(fi->locals[i].name, name) == 0) {
                    strncpy(out->name, name, XS_DBG_MAX_VAR_NAME - 1);
                    strncpy(out->type_name, fi->locals[i].type_name, 63);
                    out->slot = fi->locals[i].slot;
                    if (out->slot < dbg->vm.stack_top)
                        out->value = dbg->vm.stack[out->slot];
                    return true;
                }
            }
        }
    }
    return false;
}

/* ===== Watch expressions ===== */
int xs_debugger_add_watch(XsDebugger *dbg, const char *expression) {
    if (dbg->watch_count >= XS_DBG_MAX_WATCH) return -1;
    XsDbgWatch *w = &dbg->watches[dbg->watch_count++];
    w->id = dbg->next_watch_id++;
    strncpy(w->expression, expression, XS_DBG_MAX_EXPR_LEN - 1);
    w->enabled = true;
    w->valid = false;
    return w->id;
}

bool xs_debugger_remove_watch(XsDebugger *dbg, int watch_id) {
    for (int i = 0; i < dbg->watch_count; i++) {
        if (dbg->watches[i].id == watch_id) {
            memmove(&dbg->watches[i], &dbg->watches[i + 1],
                    (dbg->watch_count - i - 1) * sizeof(XsDbgWatch));
            dbg->watch_count--;
            return true;
        }
    }
    return false;
}

bool xs_debugger_eval_watch(XsDebugger *dbg, int watch_id) {
    for (int i = 0; i < dbg->watch_count; i++) {
        if (dbg->watches[i].id == watch_id) {
            bool ok;
            dbg->watches[i].last_value = xs_debugger_evaluate(
                dbg, dbg->watches[i].expression, 0, &ok);
            dbg->watches[i].valid = ok;
            return true;
        }
    }
    return false;
}

void xs_debugger_eval_all_watches(XsDebugger *dbg) {
    for (int i = 0; i < dbg->watch_count; i++) {
        if (dbg->watches[i].enabled) {
            xs_debugger_eval_watch(dbg, dbg->watches[i].id);
        }
    }
}

XsValue xs_debugger_evaluate(XsDebugger *dbg, const char *expression,
                              int frame_id, bool *success) {
    (void)dbg; (void)frame_id;
    /* Simplified: try to find variable by name */
    *success = false;
    XsDbgVariable var;
    if (xs_debugger_get_variable(dbg, expression, frame_id, &var)) {
        *success = true;
        return var.value;
    }
    return (XsValue){.type = VAL_ABYSS};
}

/* ===== Memory/Stack ===== */
int xs_debugger_read_memory(XsDebugger *dbg, uint32_t address,
                            uint8_t *buf, int size) {
    int avail = (int)dbg->vm.bytecode_size - (int)address;
    if (avail <= 0) return 0;
    int to_read = size < avail ? size : avail;
    memcpy(buf, dbg->vm.bytecode + address, to_read);
    return to_read;
}

void xs_debugger_dump_memory(XsDebugger *dbg, uint32_t address, int size) {
    uint8_t buf[256];
    int read = xs_debugger_read_memory(dbg, address, buf, size < 256 ? size : 256);
    for (int i = 0; i < read; i++) {
        if (i % 16 == 0) printf("%08x: ", address + i);
        printf("%02x ", buf[i]);
        if (i % 16 == 15) printf("\n");
    }
    if (read % 16 != 0) printf("\n");
}

int xs_debugger_get_stack(XsDebugger *dbg, XsValue *out, int max) {
    int count = dbg->vm.stack_top < max ? dbg->vm.stack_top : max;
    for (int i = 0; i < count; i++) out[i] = dbg->vm.stack[i];
    return count;
}

void xs_debugger_dump_stack(XsDebugger *dbg) {
    printf("Stack (top=%d):\n", dbg->vm.stack_top);
    for (int i = dbg->vm.stack_top - 1; i >= 0; i--) {
        char buf[256];
        xs_debugger_value_to_string(dbg->vm.stack[i], buf, sizeof(buf));
        printf("  [%d] %s\n", i, buf);
    }
}

/* ===== Source display ===== */
const char *xs_debugger_get_source_line(XsDebugger *dbg, int line, int *length) {
    if (!dbg->debug_info) return NULL;
    return xs_debug_info_get_line(dbg->debug_info, line, length);
}

void xs_debugger_show_source_context(XsDebugger *dbg, int center_line, int context) {
    for (int i = center_line - context; i <= center_line + context; i++) {
        if (i < 1) continue;
        int len;
        const char *line = xs_debugger_get_source_line(dbg, i, &len);
        if (!line) break;
        printf("%s %4d | %.*s\n", i == center_line ? "->" : "  ", i, len, line);
    }
}

/* ===== Events ===== */
void xs_debugger_set_event_callback(XsDebugger *dbg, XsDbgEventCallback cb, void *data) {
    dbg->event_callback = cb;
    dbg->callback_data = data;
}

void xs_debugger_emit_event(XsDebugger *dbg, const XsDbgEvent *event) {
    if (dbg->event_callback) dbg->event_callback(event, dbg->callback_data);
}

/* ===== State queries ===== */
XsDbgState xs_debugger_get_state(XsDebugger *dbg) { return dbg->state; }
XsDbgStopReason xs_debugger_get_stop_reason(XsDebugger *dbg) { return dbg->stop_reason; }

void xs_debugger_get_location(XsDebugger *dbg, char *file, int *line, int *column) {
    if (file) strncpy(file, dbg->current_file, XS_MAX_FILE_PATH);
    if (line) *line = dbg->current_line;
    if (column) *column = dbg->current_column;
}

uint32_t xs_debugger_get_ip(XsDebugger *dbg) { return dbg->vm.ip; }

/* ===== Utility ===== */
void xs_debugger_value_to_string(XsValue val, char *buf, size_t buf_size) {
    switch (val.type) {
        case VAL_BLADE:  snprintf(buf, buf_size, "%ld", (long)val.blade); break;
        case VAL_SPARK:  snprintf(buf, buf_size, "%g", val.spark); break;
        case VAL_SCROLL: snprintf(buf, buf_size, "\"%s\"", val.scroll ? val.scroll : ""); break;
        case VAL_FATE:   snprintf(buf, buf_size, "%s", val.fate ? "truth" : "lies"); break;
        case VAL_ABYSS:  snprintf(buf, buf_size, "abyss"); break;
        default:         snprintf(buf, buf_size, "<object>"); break;
    }
}

void xs_debugger_print_value(XsValue val) {
    char buf[256];
    xs_debugger_value_to_string(val, buf, sizeof(buf));
    printf("%s", buf);
}
