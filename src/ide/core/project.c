/*
 * X# IDE - Project Management Implementation
 * =============================================
 * Parse .xsproj INI files and manage project settings.
 */

#include "project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ===== Helpers ===== */
static char *trim_inplace(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

static void strip_quotes(char *s) {
    size_t len = strlen(s);
    if (len >= 2 && s[0] == '"' && s[len - 1] == '"') {
        memmove(s, s + 1, len - 2);
        s[len - 2] = '\0';
    }
}

/* ===== Lifecycle ===== */
XsProject *xs_project_new(void) {
    XsProject *p = (XsProject *)calloc(1, sizeof(XsProject));
    if (!p) return NULL;
    strcpy(p->name, "untitled");
    strcpy(p->version, "0.1.0");
    strcpy(p->entry_file, "main.xs");
    strcpy(p->build_output, "build/");
    strcpy(p->build_command, "xsharp build main.xs");
    strcpy(p->run_command, "xsharp run main.xs");
    return p;
}

void xs_project_free(XsProject *proj) {
    free(proj);
}

/* ===== Load .xsproj ===== */
bool xs_project_load(XsProject *proj, const char *path) {
    if (!proj || !path) return false;

    FILE *f = fopen(path, "r");
    if (!f) return false;

    /* Extract root directory */
    strncpy(proj->root_dir, path, sizeof(proj->root_dir) - 1);
    char *last_slash = strrchr(proj->root_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
    } else {
        strcpy(proj->root_dir, ".");
    }

    char line[1024];
    char section[64] = "";

    while (fgets(line, sizeof(line), f)) {
        char *t = trim_inplace(line);

        /* Skip empty lines and comments */
        if (t[0] == '\0' || t[0] == '#' || t[0] == ';') continue;

        /* Section header */
        if (t[0] == '[') {
            char *end = strchr(t, ']');
            if (end) {
                *end = '\0';
                strncpy(section, t + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }

        /* Key = Value */
        char *eq = strchr(t, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = trim_inplace(t);
        char *val = trim_inplace(eq + 1);
        strip_quotes(val);

        if (strcmp(section, "project") == 0) {
            if (strcmp(key, "name") == 0)
                strncpy(proj->name, val, sizeof(proj->name) - 1);
            else if (strcmp(key, "version") == 0)
                strncpy(proj->version, val, sizeof(proj->version) - 1);
        } else if (strcmp(section, "build") == 0) {
            if (strcmp(key, "entry") == 0)
                strncpy(proj->entry_file, val, sizeof(proj->entry_file) - 1);
            else if (strcmp(key, "output") == 0)
                strncpy(proj->build_output, val, sizeof(proj->build_output) - 1);
            else if (strcmp(key, "command") == 0)
                strncpy(proj->build_command, val, sizeof(proj->build_command) - 1);
        } else if (strcmp(section, "run") == 0) {
            if (strcmp(key, "command") == 0)
                strncpy(proj->run_command, val, sizeof(proj->run_command) - 1);
        }
    }

    fclose(f);
    return true;
}

/* ===== Save .xsproj ===== */
bool xs_project_save(XsProject *proj, const char *path) {
    if (!proj || !path) return false;

    FILE *f = fopen(path, "w");
    if (!f) return false;

    fprintf(f, "[project]\n");
    fprintf(f, "name = \"%s\"\n", proj->name);
    fprintf(f, "version = \"%s\"\n", proj->version);
    fprintf(f, "\n");

    fprintf(f, "[build]\n");
    fprintf(f, "entry = \"%s\"\n", proj->entry_file);
    fprintf(f, "output = \"%s\"\n", proj->build_output);
    if (proj->build_command[0])
        fprintf(f, "command = \"%s\"\n", proj->build_command);
    fprintf(f, "\n");

    if (proj->run_command[0]) {
        fprintf(f, "[run]\n");
        fprintf(f, "command = \"%s\"\n", proj->run_command);
        fprintf(f, "\n");
    }

    fprintf(f, "[dependencies]\n");
    fprintf(f, "# Add dependencies here\n");

    fclose(f);
    return true;
}

const char *xs_project_get_entry(XsProject *proj) {
    if (!proj) return NULL;
    return proj->entry_file;
}
