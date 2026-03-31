/*
 * X# (Xsharp) Shortcuts
 * =======================
 * Quick-action shortcuts: @run, @build, @clean, @test, @watch, @check, @info
 */

#ifndef XSHARP_SHORTCUTS_H
#define XSHARP_SHORTCUTS_H

#include <stdbool.h>

/* Handle a shortcut command (name without the '@' prefix).
 * Returns exit code (0 = success). */
int xs_handle_shortcut(const char* name);

/* Individual shortcut handlers */
int xs_shortcut_run(void);   /* find main.xs, run it */
int xs_shortcut_build(void); /* glob *.xs, compile all */
int xs_shortcut_clean(void); /* remove build/ directory */
int xs_shortcut_test(void);  /* find tests .xs files, run each */
int xs_shortcut_watch(void); /* stat() loop, rebuild on change */
int xs_shortcut_check(void); /* lint all .xs files */
int xs_shortcut_info(void);  /* print project info from .xsproj */

#endif /* XSHARP_SHORTCUTS_H */
