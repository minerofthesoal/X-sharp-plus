/*
 * X# (Xsharp) Runtime - Core Value System Implementation
 * ========================================================
 */

#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== String interning table ===== */
#define INTERN_TABLE_SIZE 1024

typedef struct InternEntry {
    char *str;
    uint32_t hash;
    struct InternEntry *next;
} InternEntry;

static InternEntry *intern_table[INTERN_TABLE_SIZE];
static bool runtime_initialized = false;

/* ===== Memory tracking for GC ===== */
#define MAX_TRACKED_ALLOCS 8192

typedef struct {
    void *ptr;
    size_t size;
    bool marked;
} TrackedAlloc;

static TrackedAlloc tracked_allocs[MAX_TRACKED_ALLOCS];
static int tracked_count = 0;
static size_t total_allocated = 0;
static size_t gc_threshold = 1024 * 1024; /* 1 MB */

/* ===== Hash function (FNV-1a) ===== */
static uint32_t fnv1a_hash(const char *str) {
    uint32_t hash = 2166136261u;
    while (*str) {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash;
}

/* ===== Entity hash (for entity fields) ===== */
static uint32_t entity_hash(const char *key, int bucket_count) {
    return fnv1a_hash(key) % (uint32_t)bucket_count;
}

/* ===== String interning ===== */

static char *intern_string(const char *str) {
    if (!str) return NULL;
    uint32_t hash = fnv1a_hash(str);
    int bucket = (int)(hash % INTERN_TABLE_SIZE);

    /* Look for existing interned string */
    InternEntry *entry = intern_table[bucket];
    while (entry) {
        if (entry->hash == hash && strcmp(entry->str, str) == 0) {
            return entry->str;
        }
        entry = entry->next;
    }

    /* Not found: intern it */
    InternEntry *new_entry = (InternEntry *)malloc(sizeof(InternEntry));
    if (!new_entry) return NULL;
    new_entry->str = strdup(str);
    new_entry->hash = hash;
    new_entry->next = intern_table[bucket];
    intern_table[bucket] = new_entry;
    return new_entry->str;
}

static void free_intern_table(void) {
    for (int i = 0; i < INTERN_TABLE_SIZE; i++) {
        InternEntry *entry = intern_table[i];
        while (entry) {
            InternEntry *next = entry->next;
            free(entry->str);
            free(entry);
            entry = next;
        }
        intern_table[i] = NULL;
    }
}

/* ===== Memory tracking ===== */

static void track_alloc(void *ptr, size_t size) {
    if (tracked_count >= MAX_TRACKED_ALLOCS) return;
    tracked_allocs[tracked_count].ptr = ptr;
    tracked_allocs[tracked_count].size = size;
    tracked_allocs[tracked_count].marked = false;
    tracked_count++;
    total_allocated += size;
}

static void untrack_alloc(void *ptr) {
    for (int i = 0; i < tracked_count; i++) {
        if (tracked_allocs[i].ptr == ptr) {
            total_allocated -= tracked_allocs[i].size;
            tracked_allocs[i] = tracked_allocs[tracked_count - 1];
            tracked_count--;
            return;
        }
    }
}

/* ===== xs_strdup ===== */

char *xs_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = (char *)malloc(len);
    if (!dup) return NULL;
    memcpy(dup, s, len);
    track_alloc(dup, len);
    return dup;
}

/* ===== Arsenal (dynamic array) ===== */

XsArsenal *xs_arsenal_new(int initial_cap) {
    XsArsenal *a = (XsArsenal *)malloc(sizeof(XsArsenal));
    if (!a) return NULL;
    if (initial_cap < 4) initial_cap = 4;
    a->items = (XsValue *)calloc((size_t)initial_cap, sizeof(XsValue));
    if (!a->items) { free(a); return NULL; }
    a->count = 0;
    a->capacity = initial_cap;
    track_alloc(a, sizeof(XsArsenal) + sizeof(XsValue) * (size_t)initial_cap);
    return a;
}

void xs_arsenal_free(XsArsenal *a) {
    if (!a) return;
    /* Free any string values in the array */
    for (int i = 0; i < a->count; i++) {
        if (a->items[i].type == VAL_SCROLL && a->items[i].scroll) {
            /* Don't free interned strings */
        }
    }
    untrack_alloc(a);
    free(a->items);
    free(a);
}

void xs_arsenal_push(XsArsenal *a, XsValue v) {
    if (!a) return;
    if (a->count >= a->capacity) {
        int new_cap = a->capacity * 2;
        XsValue *tmp = (XsValue *)realloc(a->items, sizeof(XsValue) * (size_t)new_cap);
        if (!tmp) return;
        a->items = tmp;
        a->capacity = new_cap;
    }
    a->items[a->count++] = v;
}

XsValue xs_arsenal_pop(XsArsenal *a) {
    if (!a || a->count == 0) return xs_abyss();
    return a->items[--a->count];
}

XsValue xs_arsenal_get(XsArsenal *a, int index) {
    if (!a || index < 0 || index >= a->count) return xs_abyss();
    return a->items[index];
}

void xs_arsenal_set(XsArsenal *a, int index, XsValue v) {
    if (!a || index < 0 || index >= a->count) return;
    a->items[index] = v;
}

/* ===== Entity (hash-map based object) ===== */

#define ENTITY_DEFAULT_BUCKETS 16

XsEntity *xs_entity_new(void) {
    XsEntity *e = (XsEntity *)malloc(sizeof(XsEntity));
    if (!e) return NULL;
    e->bucket_count = ENTITY_DEFAULT_BUCKETS;
    e->buckets = (XsEntityEntry **)calloc((size_t)e->bucket_count,
                                          sizeof(XsEntityEntry *));
    if (!e->buckets) { free(e); return NULL; }
    e->count = 0;
    track_alloc(e, sizeof(XsEntity) +
                sizeof(XsEntityEntry *) * (size_t)e->bucket_count);
    return e;
}

void xs_entity_free(XsEntity *e) {
    if (!e) return;
    for (int i = 0; i < e->bucket_count; i++) {
        XsEntityEntry *entry = e->buckets[i];
        while (entry) {
            XsEntityEntry *next = entry->next;
            free(entry->key);
            /* Free scroll values */
            if (entry->value.type == VAL_SCROLL && entry->value.scroll) {
                /* Only free non-interned strings */
            }
            free(entry);
            entry = next;
        }
    }
    untrack_alloc(e);
    free(e->buckets);
    free(e);
}

void xs_entity_set(XsEntity *e, const char *key, XsValue v) {
    if (!e || !key) return;

    uint32_t idx = entity_hash(key, e->bucket_count);

    /* Check if key already exists */
    XsEntityEntry *entry = e->buckets[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = v;
            return;
        }
        entry = entry->next;
    }

    /* Insert new entry */
    XsEntityEntry *new_entry = (XsEntityEntry *)malloc(sizeof(XsEntityEntry));
    if (!new_entry) return;
    new_entry->key = strdup(key);
    new_entry->value = v;
    new_entry->next = e->buckets[idx];
    e->buckets[idx] = new_entry;
    e->count++;

    /* Rehash if load factor > 0.75 */
    if (e->count > e->bucket_count * 3 / 4) {
        int new_bucket_count = e->bucket_count * 2;
        XsEntityEntry **new_buckets = (XsEntityEntry **)calloc(
            (size_t)new_bucket_count, sizeof(XsEntityEntry *));
        if (!new_buckets) return;

        for (int i = 0; i < e->bucket_count; i++) {
            XsEntityEntry *ent = e->buckets[i];
            while (ent) {
                XsEntityEntry *next = ent->next;
                uint32_t new_idx = entity_hash(ent->key, new_bucket_count);
                ent->next = new_buckets[new_idx];
                new_buckets[new_idx] = ent;
                ent = next;
            }
        }
        free(e->buckets);
        e->buckets = new_buckets;
        e->bucket_count = new_bucket_count;
    }
}

XsValue xs_entity_get(XsEntity *e, const char *key) {
    if (!e || !key) return xs_abyss();

    uint32_t idx = entity_hash(key, e->bucket_count);
    XsEntityEntry *entry = e->buckets[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return xs_abyss();
}

bool xs_entity_has(XsEntity *e, const char *key) {
    if (!e || !key) return false;

    uint32_t idx = entity_hash(key, e->bucket_count);
    XsEntityEntry *entry = e->buckets[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) return true;
        entry = entry->next;
    }
    return false;
}

void xs_entity_delete(XsEntity *e, const char *key) {
    if (!e || !key) return;

    uint32_t idx = entity_hash(key, e->bucket_count);
    XsEntityEntry **prev_ptr = &e->buckets[idx];
    XsEntityEntry *entry = e->buckets[idx];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            *prev_ptr = entry->next;
            free(entry->key);
            free(entry);
            e->count--;
            return;
        }
        prev_ptr = &entry->next;
        entry = entry->next;
    }
}

/* ===== Value printing ===== */

void xs_value_print(XsValue val) {
    switch (val.type) {
        case VAL_BLADE:
            printf("%lld", (long long)val.blade);
            break;
        case VAL_SPARK:
            printf("%g", val.spark);
            break;
        case VAL_SCROLL:
            printf("%s", val.scroll ? val.scroll : "");
            break;
        case VAL_FATE:
            printf("%s", val.fate ? "truth" : "lies");
            break;
        case VAL_ABYSS:
            printf("abyss");
            break;
        case VAL_ARSENAL: {
            XsArsenal *a = (XsArsenal *)val.object;
            if (!a) { printf("[]"); break; }
            printf("[");
            for (int i = 0; i < a->count; i++) {
                if (i > 0) printf(", ");
                xs_value_print(a->items[i]);
            }
            printf("]");
            break;
        }
        case VAL_ENTITY:
            printf("<entity@%p>", val.object);
            break;
        case VAL_SPELL:
            printf("<spell@%p>", val.object);
            break;
        case VAL_NATIVE_FN:
            printf("<native_fn@%p>", val.object);
            break;
    }
}

/* ===== Value comparison ===== */

bool xs_values_equal(XsValue a, XsValue b) {
    if (a.type != b.type) {
        /* Allow blade/spark comparison */
        if (a.type == VAL_BLADE && b.type == VAL_SPARK) {
            return (double)a.blade == b.spark;
        }
        if (a.type == VAL_SPARK && b.type == VAL_BLADE) {
            return a.spark == (double)b.blade;
        }
        return false;
    }
    switch (a.type) {
        case VAL_BLADE:  return a.blade == b.blade;
        case VAL_SPARK:  return a.spark == b.spark;
        case VAL_SCROLL: {
            if (a.scroll == b.scroll) return true; /* interned match */
            if (!a.scroll || !b.scroll) return false;
            return strcmp(a.scroll, b.scroll) == 0;
        }
        case VAL_FATE:   return a.fate == b.fate;
        case VAL_ABYSS:  return true;
        default:         return a.object == b.object;
    }
}

/* ===== Value hashing ===== */

uint32_t xs_value_hash(XsValue val) {
    switch (val.type) {
        case VAL_BLADE: {
            uint64_t v = (uint64_t)val.blade;
            v = ((v >> 16) ^ v) * 0x45d9f3b;
            v = ((v >> 16) ^ v) * 0x45d9f3b;
            v = (v >> 16) ^ v;
            return (uint32_t)v;
        }
        case VAL_SPARK: {
            union { double d; uint64_t u; } conv;
            conv.d = val.spark;
            return (uint32_t)(conv.u ^ (conv.u >> 32));
        }
        case VAL_SCROLL:
            return val.scroll ? fnv1a_hash(val.scroll) : 0;
        case VAL_FATE:
            return val.fate ? 1 : 0;
        case VAL_ABYSS:
            return 0;
        default:
            return (uint32_t)(uintptr_t)val.object;
    }
}

/* ===== Type checking and conversion ===== */

const char *xs_type_name(ValueType type) {
    switch (type) {
        case VAL_BLADE:     return "blade";
        case VAL_SPARK:     return "spark";
        case VAL_SCROLL:    return "scroll";
        case VAL_FATE:      return "fate";
        case VAL_ABYSS:     return "abyss";
        case VAL_ARSENAL:   return "arsenal";
        case VAL_ENTITY:    return "entity";
        case VAL_SPELL:     return "spell";
        case VAL_NATIVE_FN: return "native_fn";
    }
    return "unknown";
}

bool xs_is_truthy(XsValue val) {
    switch (val.type) {
        case VAL_FATE:   return val.fate;
        case VAL_ABYSS:  return false;
        case VAL_BLADE:  return val.blade != 0;
        case VAL_SPARK:  return val.spark != 0.0;
        case VAL_SCROLL: return val.scroll != NULL && val.scroll[0] != '\0';
        default:         return val.object != NULL;
    }
}

XsValue xs_convert_to_blade(XsValue val) {
    switch (val.type) {
        case VAL_BLADE:  return val;
        case VAL_SPARK:  return xs_blade((int64_t)val.spark);
        case VAL_FATE:   return xs_blade(val.fate ? 1 : 0);
        case VAL_SCROLL: {
            if (!val.scroll) return xs_blade(0);
            char *end;
            int64_t result = strtoll(val.scroll, &end, 10);
            return xs_blade(result);
        }
        default: return xs_blade(0);
    }
}

XsValue xs_convert_to_spark(XsValue val) {
    switch (val.type) {
        case VAL_SPARK:  return val;
        case VAL_BLADE:  return xs_spark((double)val.blade);
        case VAL_FATE:   return xs_spark(val.fate ? 1.0 : 0.0);
        case VAL_SCROLL: {
            if (!val.scroll) return xs_spark(0.0);
            char *end;
            double result = strtod(val.scroll, &end);
            return xs_spark(result);
        }
        default: return xs_spark(0.0);
    }
}

XsValue xs_convert_to_scroll(XsValue val) {
    char buf[128];
    switch (val.type) {
        case VAL_SCROLL: return val;
        case VAL_BLADE:
            snprintf(buf, sizeof(buf), "%lld", (long long)val.blade);
            return xs_scroll(xs_strdup(buf));
        case VAL_SPARK:
            snprintf(buf, sizeof(buf), "%g", val.spark);
            return xs_scroll(xs_strdup(buf));
        case VAL_FATE:
            return xs_scroll(xs_strdup(val.fate ? "truth" : "lies"));
        case VAL_ABYSS:
            return xs_scroll(xs_strdup("abyss"));
        default:
            snprintf(buf, sizeof(buf), "<object@%p>", val.object);
            return xs_scroll(xs_strdup(buf));
    }
}

XsValue xs_convert_to_fate(XsValue val) {
    return xs_fate(xs_is_truthy(val));
}

/* ===== Runtime init/shutdown ===== */

void xs_runtime_init(void) {
    if (runtime_initialized) return;
    memset(intern_table, 0, sizeof(intern_table));
    memset(tracked_allocs, 0, sizeof(tracked_allocs));
    tracked_count = 0;
    total_allocated = 0;
    runtime_initialized = true;
}

void xs_runtime_shutdown(void) {
    if (!runtime_initialized) return;
    free_intern_table();
    tracked_count = 0;
    total_allocated = 0;
    runtime_initialized = false;
}
