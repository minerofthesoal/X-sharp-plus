/*
 * X# (Xsharp) Debug Adapter Protocol
 * =====================================
 * Simple DAP-like protocol over stdio using JSON messages.
 * Format: "Content-Length: N\r\n\r\n{...json...}"
 */

#ifndef XSHARP_DEBUG_PROTOCOL_H
#define XSHARP_DEBUG_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "debugger.h"

/* ===== JSON Value Types ===== */
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonValueType;

/* Forward declaration */
typedef struct JsonValue JsonValue;
typedef struct JsonPair  JsonPair;

/* JSON key-value pair */
struct JsonPair {
    char      *key;
    JsonValue *value;
};

/* JSON value */
struct JsonValue {
    JsonValueType type;
    union {
        bool        boolean;
        double      number;
        char       *string;
        struct {
            JsonValue **items;
            int         count;
            int         capacity;
        } array;
        struct {
            JsonPair   *pairs;
            int         count;
            int         capacity;
        } object;
    } as;
};

/* ===== JSON API ===== */
JsonValue *json_null(void);
JsonValue *json_bool(bool val);
JsonValue *json_number(double val);
JsonValue *json_string(const char *val);
JsonValue *json_array(void);
JsonValue *json_object(void);
void       json_array_push(JsonValue *arr, JsonValue *item);
void       json_object_set(JsonValue *obj, const char *key, JsonValue *val);
JsonValue *json_object_get(const JsonValue *obj, const char *key);
const char*json_object_get_string(const JsonValue *obj, const char *key);
double     json_object_get_number(const JsonValue *obj, const char *key);
bool       json_object_get_bool(const JsonValue *obj, const char *key);
void       json_free(JsonValue *val);

/* Parse JSON from string. Returns NULL on error. */
JsonValue *json_parse(const char *text, const char **end_ptr);

/* Emit JSON to a newly allocated string. Caller must free. */
char      *json_emit(const JsonValue *val);

/* ===== DAP Message Types ===== */
typedef enum {
    DAP_REQUEST,
    DAP_RESPONSE,
    DAP_EVENT
} DapMessageType;

/* ===== DAP Message ===== */
typedef struct {
    DapMessageType  type;
    int             seq;
    char            command[128];
    int             request_seq;    /* for responses */
    bool            success;        /* for responses */
    char            message[256];   /* error message for failed responses */
    JsonValue      *body;           /* message body (object) */
} DapMessage;

/* ===== Protocol I/O ===== */

/* Read a DAP message from stdin. Returns NULL on EOF/error. */
DapMessage *dap_read_message(void);

/* Write a DAP message to stdout. */
void dap_write_message(const DapMessage *msg);

/* Free a DAP message. */
void dap_free_message(DapMessage *msg);

/* ===== DAP Server ===== */

/* Run the DAP server loop, handling messages until disconnect.
 * Uses the provided debugger instance. */
void dap_server_run(XsDebugger *dbg);

/* ===== Message Construction Helpers ===== */
DapMessage *dap_make_response(int request_seq, const char *command, bool success);
DapMessage *dap_make_event(const char *event_name);

#endif /* XSHARP_DEBUG_PROTOCOL_H */
