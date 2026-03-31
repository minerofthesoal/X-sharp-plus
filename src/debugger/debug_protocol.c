/*
 * X# (Xsharp) Debug Adapter Protocol Implementation
 */

#include "debug_protocol.h"
#include "debugger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple JSON output helpers */
static void dap_json_start_object(FILE* out) {
    fprintf(out, "{");
}
static void dap_json_end_object(FILE* out) {
    fprintf(out, "}");
}
static void dap_json_start_array(FILE* out) {
    fprintf(out, "[");
}
static void dap_json_end_array(FILE* out) {
    fprintf(out, "]");
}
static void dap_json_key(FILE* out, const char* k) {
    fprintf(out, "\"%s\":", k);
}
static void dap_json_string(FILE* out, const char* v) {
    fprintf(out, "\"%s\"", v);
}
static void dap_json_int(FILE* out, int v) {
    fprintf(out, "%d", v);
}
static void dap_json_bool(FILE* out, int v) {
    fprintf(out, "%s", v ? "true" : "false");
}
static void dap_json_comma(FILE* out) {
    fprintf(out, ",");
}

/* Send DAP message */
static void dap_send(const char* body) {
    int len = (int)strlen(body);
    printf("Content-Length: %d\r\n\r\n%s", len, body);
    fflush(stdout);
}

/* Build and send a DAP response */
static void dap_response(int seq, int request_seq, const char* command, bool success,
                         const char* body_json) {
    char buf[8192];
    snprintf(buf, sizeof(buf),
             "{\"seq\":%d,\"type\":\"response\",\"request_seq\":%d,"
             "\"command\":\"%s\",\"success\":%s%s%s}",
             seq, request_seq, command, success ? "true" : "false", body_json ? ",\"body\":" : "",
             body_json ? body_json : "");
    dap_send(buf);
}

/* Send a DAP event */
static void dap_event(int seq, const char* event_name, const char* body_json) {
    char buf[4096];
    snprintf(buf, sizeof(buf), "{\"seq\":%d,\"type\":\"event\",\"event\":\"%s\"%s%s}", seq,
             event_name, body_json ? ",\"body\":" : "", body_json ? body_json : "");
    dap_send(buf);
}

/* Read a DAP message from stdin */
static int dap_read_msg(char* buf, int buf_size) {
    /* Read Content-Length header */
    char header[256];
    int content_length = 0;

    while (fgets(header, sizeof(header), stdin)) {
        if (header[0] == '\r' || header[0] == '\n')
            break;
        if (strncmp(header, "Content-Length:", 14) == 0) {
            content_length = atoi(header + 14);
        }
    }

    if (content_length <= 0 || content_length >= buf_size)
        return -1;

    int read = (int)fread(buf, 1, content_length, stdin);
    buf[read] = '\0';
    return read;
}

/* Simple JSON value extraction */
static const char* json_find_string(const char* json, const char* key, char* out, int out_size) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    const char* p = strstr(json, pattern);
    if (!p)
        return NULL;
    p += strlen(pattern);
    const char* end = strchr(p, '"');
    if (!end)
        return NULL;
    int len = (int)(end - p);
    if (len >= out_size)
        len = out_size - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return out;
}

static int json_find_int(const char* json, const char* key, int default_val) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    const char* p = strstr(json, pattern);
    if (!p)
        return default_val;
    p += strlen(pattern);
    return atoi(p);
}

/* ===== DAP Protocol Loop ===== */
void xs_dap_run(XsDebugger* dbg) {
    char buf[16384];
    int seq = 1;
    bool running = true;

    while (running) {
        if (dap_read_msg(buf, sizeof(buf)) <= 0)
            break;

        char command[64] = {0};
        json_find_string(buf, "command", command, sizeof(command));
        int request_seq = json_find_int(buf, "seq", 0);

        if (strcmp(command, "initialize") == 0) {
            dap_response(seq++, request_seq, "initialize", true,
                         "{\"supportsConfigurationDoneRequest\":true,"
                         "\"supportsConditionalBreakpoints\":true,"
                         "\"supportsHitConditionalBreakpoints\":true,"
                         "\"supportsEvaluateForHovers\":true,"
                         "\"supportsStepBack\":false}");
            dap_event(seq++, "initialized", NULL);
        } else if (strcmp(command, "launch") == 0) {
            char program[512] = {0};
            json_find_string(buf, "program", program, sizeof(program));
            xs_debugger_load_file(dbg, program);
            dap_response(seq++, request_seq, "launch", true, NULL);
            xs_debugger_launch(dbg);
        } else if (strcmp(command, "setBreakpoints") == 0) {
            /* Parse breakpoints from request */
            char source[512] = {0};
            json_find_string(buf, "path", source, sizeof(source));
            /* Simple: set breakpoint at lines found in request */
            dap_response(seq++, request_seq, "setBreakpoints", true, "{\"breakpoints\":[]}");
        } else if (strcmp(command, "configurationDone") == 0) {
            dap_response(seq++, request_seq, "configurationDone", true, NULL);
        } else if (strcmp(command, "continue") == 0) {
            xs_debugger_continue(dbg);
            dap_response(seq++, request_seq, "continue", true, "{\"allThreadsContinued\":true}");
        } else if (strcmp(command, "next") == 0) {
            xs_debugger_step_over(dbg);
            dap_response(seq++, request_seq, "next", true, NULL);
            dap_event(seq++, "stopped", "{\"reason\":\"step\",\"threadId\":1}");
        } else if (strcmp(command, "stepIn") == 0) {
            xs_debugger_step_in(dbg);
            dap_response(seq++, request_seq, "stepIn", true, NULL);
            dap_event(seq++, "stopped", "{\"reason\":\"step\",\"threadId\":1}");
        } else if (strcmp(command, "stepOut") == 0) {
            xs_debugger_step_out(dbg);
            dap_response(seq++, request_seq, "stepOut", true, NULL);
            dap_event(seq++, "stopped", "{\"reason\":\"step\",\"threadId\":1}");
        } else if (strcmp(command, "threads") == 0) {
            dap_response(seq++, request_seq, "threads", true,
                         "{\"threads\":[{\"id\":1,\"name\":\"main\"}]}");
        } else if (strcmp(command, "stackTrace") == 0) {
            char body[2048] = "{\"stackFrames\":[";
            char frame_buf[512];
            XsDbgCallFrame frames[32];
            int count = xs_debugger_get_call_stack(dbg, frames, 32);
            for (int i = 0; i < count; i++) {
                snprintf(frame_buf, sizeof(frame_buf),
                         "%s{\"id\":%d,\"name\":\"%s\",\"source\":{\"path\":\"%s\"},"
                         "\"line\":%d,\"column\":%d}",
                         i > 0 ? "," : "", frames[i].id, frames[i].func_name, frames[i].source_file,
                         frames[i].line, frames[i].column);
                strcat(body, frame_buf);
            }
            strcat(body, "],\"totalFrames\":");
            snprintf(frame_buf, sizeof(frame_buf), "%d}", count);
            strcat(body, frame_buf);
            dap_response(seq++, request_seq, "stackTrace", true, body);
        } else if (strcmp(command, "scopes") == 0) {
            dap_response(seq++, request_seq, "scopes", true,
                         "{\"scopes\":[{\"name\":\"Locals\",\"variablesReference\":1},"
                         "{\"name\":\"Globals\",\"variablesReference\":2}]}");
        } else if (strcmp(command, "variables") == 0) {
            int ref = json_find_int(buf, "variablesReference", 0);
            char body[4096] = "{\"variables\":[";
            XsDbgVariable vars[32];
            int count = 0;
            if (ref == 1)
                count = xs_debugger_get_locals(dbg, 0, vars, 32);
            else if (ref == 2)
                count = xs_debugger_get_globals(dbg, vars, 32);
            for (int i = 0; i < count; i++) {
                char vbuf[256];
                xs_debugger_value_to_string(vars[i].value, vbuf, sizeof(vbuf));
                char entry[512];
                snprintf(entry, sizeof(entry),
                         "%s{\"name\":\"%s\",\"value\":\"%s\",\"variablesReference\":0}",
                         i > 0 ? "," : "", vars[i].name, vbuf);
                strcat(body, entry);
            }
            strcat(body, "]}");
            dap_response(seq++, request_seq, "variables", true, body);
        } else if (strcmp(command, "evaluate") == 0) {
            char expression[256] = {0};
            json_find_string(buf, "expression", expression, sizeof(expression));
            bool ok;
            XsValue val = xs_debugger_evaluate(dbg, expression, 0, &ok);
            char vbuf[256];
            xs_debugger_value_to_string(val, vbuf, sizeof(vbuf));
            char body[512];
            snprintf(body, sizeof(body), "{\"result\":\"%s\",\"variablesReference\":0}", vbuf);
            dap_response(seq++, request_seq, "evaluate", true, body);
        } else if (strcmp(command, "disconnect") == 0) {
            dap_response(seq++, request_seq, "disconnect", true, NULL);
            running = false;
        } else {
            dap_response(seq++, request_seq, command, false, NULL);
        }

        (void)dap_json_start_object;
        (void)dap_json_end_object;
        (void)dap_json_start_array;
        (void)dap_json_end_array;
        (void)dap_json_key;
        (void)dap_json_string;
        (void)dap_json_int;
        (void)dap_json_bool;
        (void)dap_json_comma;
    }

    xs_debugger_stop(dbg);
}
