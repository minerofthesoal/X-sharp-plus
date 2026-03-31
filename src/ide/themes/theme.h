/*
 * X# IDE - Theme System
 * =======================
 * Dark "Obsidian" and light "Radiance" themes using GtkCssProvider.
 */

#ifndef XS_IDE_THEME_H
#define XS_IDE_THEME_H

#include <gtk/gtk.h>
#include <stdbool.h>

typedef struct {
    char name[64];
    /* Editor colors */
    char bg[16];
    char fg[16];
    char keyword[16];
    char string_color[16];
    char comment[16];
    char number[16];
    char type_color[16];
    char operator_color[16];
    /* UI chrome */
    char sidebar_bg[16];
    char statusbar_bg[16];
    char toolbar_bg[16];
    char console_bg[16];
    char border[16];
    char accent[16];
    char selection_bg[16];
    char selection_fg[16];
    bool is_dark;
} XsTheme;

/* Lifecycle (pointer-based API for IDE integration) */
XsTheme *xs_theme_new(void);
void     xs_theme_free(XsTheme *theme);

/* Presets - by-value for simple use */
XsTheme xs_theme_obsidian(void);
XsTheme xs_theme_radiance(void);

/* Apply theme via CSS */
void xs_theme_apply(XsTheme *theme, GtkWidget *window);

#endif /* XS_IDE_THEME_H */
