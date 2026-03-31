/*
 * X# Standard Library - Registry
 * ================================
 * Central registration of all stdlib modules with the VM.
 */

#ifndef XS_STDLIB_REGISTRY_H
#define XS_STDLIB_REGISTRY_H

#include "../src/runtime/runtime.h"

/* Register every stdlib function with the VM in one call */
void xs_register_all_stdlib(VM *vm);

#endif /* XS_STDLIB_REGISTRY_H */
