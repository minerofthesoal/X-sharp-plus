/*
 * X# (Xsharp) Project Templates
 * ===============================
 * Create new projects from templates: game, ai, console, library.
 */

#ifndef XSHARP_PROJECT_TEMPLATE_H
#define XSHARP_PROJECT_TEMPLATE_H

#include <stdbool.h>

/* Available project templates */
typedef enum {
    TEMPLATE_CONSOLE,   /* hello world console app */
    TEMPLATE_GAME,      /* game loop with rendering */
    TEMPLATE_AI,        /* neural network scaffold */
    TEMPLATE_LIBRARY    /* library scaffold */
} XsProjectTemplate;

/* Create a new project directory with the given name and template.
 * template_name: "console", "game", "ai", "library"
 * Returns true on success. */
bool xs_create_project(const char *name, const char *template_name);

/* List available templates to stdout. */
void xs_list_templates(void);

#endif /* XSHARP_PROJECT_TEMPLATE_H */
