/*
 * X# IDE Project Management
 */
#ifndef XS_IDE_PROJECT_H
#define XS_IDE_PROJECT_H

#include <stdbool.h>

typedef struct {
    char name[128];
    char version[32];
    char root_dir[1024];
    char entry_file[256];
    char build_output[256];
    char build_command[512];
    char run_command[512];
} XsProject;

XsProject *xs_project_new(void);
void       xs_project_free(XsProject *proj);
bool       xs_project_load(XsProject *proj, const char *path);
bool       xs_project_save(XsProject *proj, const char *path);
const char*xs_project_get_entry(XsProject *proj);

#endif
