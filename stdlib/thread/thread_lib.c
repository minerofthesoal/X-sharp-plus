/*
 * X# Standard Library - Thread Module Implementation
 * ====================================================
 * Threading primitives using POSIX pthreads.
 */

#include "thread_lib.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

/* ===== Internal channel structure ===== */

#define CHANNEL_QUEUE_CAP 256

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
    XsValue         queue[CHANNEL_QUEUE_CAP];
    int             head;
    int             tail;
    int             count;
} XsChannel;

/* ===== Thread wrapper ===== */

typedef struct {
    XsNativeFn fn;
    XsValue    arg;
} ThreadArg;

static void *thread_entry(void *raw) {
    ThreadArg *ta = (ThreadArg *)raw;
    XsValue arg = ta->arg;
    XsNativeFn fn = ta->fn;
    free(ta);
    fn(1, &arg);
    return NULL;
}

/* ===== Thread functions ===== */

XsValue xs_thread_spawn(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_NATIVE_FN) return xs_abyss();

    XsNativeFn fn = (XsNativeFn)args[0].object;
    ThreadArg *ta = (ThreadArg *)malloc(sizeof(ThreadArg));
    if (!ta) return xs_abyss();
    ta->fn = fn;
    ta->arg = (argc >= 2) ? args[1] : xs_abyss();

    pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
    if (!thread) { free(ta); return xs_abyss(); }

    if (pthread_create(thread, NULL, thread_entry, ta) != 0) {
        free(ta);
        free(thread);
        return xs_abyss();
    }

    XsValue val;
    val.type = VAL_ENTITY;
    val.object = thread;
    return val;
}

XsValue xs_thread_join(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_ENTITY) return xs_abyss();
    pthread_t *thread = (pthread_t *)args[0].object;
    if (!thread) return xs_abyss();
    pthread_join(*thread, NULL);
    free(thread);
    return xs_fate(true);
}

XsValue xs_thread_detach(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_ENTITY) return xs_abyss();
    pthread_t *thread = (pthread_t *)args[0].object;
    if (!thread) return xs_abyss();
    pthread_detach(*thread);
    free(thread);
    return xs_fate(true);
}

/* ===== Mutex functions ===== */

XsValue xs_thread_mutex_new(int argc, XsValue *args) {
    (void)argc; (void)args;
    pthread_mutex_t *mtx = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (!mtx) return xs_abyss();
    pthread_mutex_init(mtx, NULL);
    XsValue val;
    val.type = VAL_ENTITY;
    val.object = mtx;
    return val;
}

XsValue xs_thread_mutex_lock(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_ENTITY) return xs_fate(false);
    pthread_mutex_t *mtx = (pthread_mutex_t *)args[0].object;
    if (!mtx) return xs_fate(false);
    pthread_mutex_lock(mtx);
    return xs_fate(true);
}

XsValue xs_thread_mutex_unlock(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_ENTITY) return xs_fate(false);
    pthread_mutex_t *mtx = (pthread_mutex_t *)args[0].object;
    if (!mtx) return xs_fate(false);
    pthread_mutex_unlock(mtx);
    return xs_fate(true);
}

/* ===== Channel functions ===== */

XsValue xs_thread_channel_new(int argc, XsValue *args) {
    (void)argc; (void)args;
    XsChannel *ch = (XsChannel *)calloc(1, sizeof(XsChannel));
    if (!ch) return xs_abyss();
    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->not_empty, NULL);
    pthread_cond_init(&ch->not_full, NULL);
    ch->head = 0;
    ch->tail = 0;
    ch->count = 0;
    XsValue val;
    val.type = VAL_ENTITY;
    val.object = ch;
    return val;
}

XsValue xs_thread_channel_send(int argc, XsValue *args) {
    if (argc < 2 || args[0].type != VAL_ENTITY) return xs_fate(false);
    XsChannel *ch = (XsChannel *)args[0].object;
    if (!ch) return xs_fate(false);

    pthread_mutex_lock(&ch->mutex);
    while (ch->count >= CHANNEL_QUEUE_CAP) {
        pthread_cond_wait(&ch->not_full, &ch->mutex);
    }
    ch->queue[ch->tail] = args[1];
    ch->tail = (ch->tail + 1) % CHANNEL_QUEUE_CAP;
    ch->count++;
    pthread_cond_signal(&ch->not_empty);
    pthread_mutex_unlock(&ch->mutex);
    return xs_fate(true);
}

XsValue xs_thread_channel_recv(int argc, XsValue *args) {
    if (argc < 1 || args[0].type != VAL_ENTITY) return xs_abyss();
    XsChannel *ch = (XsChannel *)args[0].object;
    if (!ch) return xs_abyss();

    pthread_mutex_lock(&ch->mutex);
    while (ch->count == 0) {
        pthread_cond_wait(&ch->not_empty, &ch->mutex);
    }
    XsValue val = ch->queue[ch->head];
    ch->head = (ch->head + 1) % CHANNEL_QUEUE_CAP;
    ch->count--;
    pthread_cond_signal(&ch->not_full);
    pthread_mutex_unlock(&ch->mutex);
    return val;
}

/* ===== Atomic functions ===== */

XsValue xs_thread_atomic_inc(int argc, XsValue *args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_BLADE) {
        int64_t result = __atomic_add_fetch(&args[0].blade, 1, __ATOMIC_SEQ_CST);
        return xs_blade(result);
    }
    return xs_abyss();
}

XsValue xs_thread_atomic_dec(int argc, XsValue *args) {
    if (argc < 1) return xs_abyss();
    if (args[0].type == VAL_BLADE) {
        int64_t result = __atomic_sub_fetch(&args[0].blade, 1, __ATOMIC_SEQ_CST);
        return xs_blade(result);
    }
    return xs_abyss();
}

/* ===== Registration ===== */

void xs_thread_register(VM *vm) {
    vm_register_native(vm, "Thread.spawn",       xs_thread_spawn);
    vm_register_native(vm, "Thread.join",        xs_thread_join);
    vm_register_native(vm, "Thread.detach",      xs_thread_detach);
    vm_register_native(vm, "Thread.mutexNew",    xs_thread_mutex_new);
    vm_register_native(vm, "Thread.mutexLock",   xs_thread_mutex_lock);
    vm_register_native(vm, "Thread.mutexUnlock", xs_thread_mutex_unlock);
    vm_register_native(vm, "Thread.channelNew",  xs_thread_channel_new);
    vm_register_native(vm, "Thread.channelSend", xs_thread_channel_send);
    vm_register_native(vm, "Thread.channelRecv", xs_thread_channel_recv);
    vm_register_native(vm, "Thread.atomicInc",   xs_thread_atomic_inc);
    vm_register_native(vm, "Thread.atomicDec",   xs_thread_atomic_dec);
}
