/*
 * X# IDE - Theme Implementation
 * ===============================
 * Dark "Obsidian" and light "Radiance" themes with CSS application.
 */

#include "theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Lifecycle ===== */
XsTheme* xs_theme_new(void) {
    XsTheme* t = (XsTheme*)calloc(1, sizeof(XsTheme));
    if (!t)
        return NULL;
    /* Default to Obsidian */
    *t = xs_theme_obsidian();
    return t;
}

void xs_theme_free(XsTheme* theme) {
    free(theme);
}

/* ===== Obsidian (Dark Theme) ===== */
XsTheme xs_theme_obsidian(void) {
    XsTheme t;
    memset(&t, 0, sizeof(t));

    strcpy(t.name, "Obsidian");
    t.is_dark = true;

    /* Editor */
    strcpy(t.bg, "#1a1a2e");
    strcpy(t.fg, "#e0e0e0");
    strcpy(t.keyword, "#ff6b6b");
    strcpy(t.string_color, "#98c379");
    strcpy(t.comment, "#5c6370");
    strcpy(t.number, "#d19a66");
    strcpy(t.type_color, "#61afef");
    strcpy(t.operator_color, "#56b6c2");

    /* UI chrome */
    strcpy(t.sidebar_bg, "#16213e");
    strcpy(t.statusbar_bg, "#0f3460");
    strcpy(t.toolbar_bg, "#1a1a2e");
    strcpy(t.console_bg, "#121220");
    strcpy(t.border, "#2a2a4a");
    strcpy(t.accent, "#e94560");
    strcpy(t.selection_bg, "#3a3a5c");
    strcpy(t.selection_fg, "#ffffff");

    return t;
}

/* ===== Radiance (Light Theme) ===== */
XsTheme xs_theme_radiance(void) {
    XsTheme t;
    memset(&t, 0, sizeof(t));

    strcpy(t.name, "Radiance");
    t.is_dark = false;

    /* Editor */
    strcpy(t.bg, "#fafafa");
    strcpy(t.fg, "#383838");
    strcpy(t.keyword, "#d73a49");
    strcpy(t.string_color, "#22863a");
    strcpy(t.comment, "#6a737d");
    strcpy(t.number, "#005cc5");
    strcpy(t.type_color, "#6f42c1");
    strcpy(t.operator_color, "#e36209");

    /* UI chrome */
    strcpy(t.sidebar_bg, "#f0f0f0");
    strcpy(t.statusbar_bg, "#e0e0e0");
    strcpy(t.toolbar_bg, "#f5f5f5");
    strcpy(t.console_bg, "#ffffff");
    strcpy(t.border, "#d0d0d0");
    strcpy(t.accent, "#0366d6");
    strcpy(t.selection_bg, "#bbdefb");
    strcpy(t.selection_fg, "#000000");

    return t;
}

/* ===== Apply Theme ===== */
void xs_theme_apply(XsTheme* theme, GtkWidget* window) {
    if (!theme || !window)
        return;

    GtkCssProvider* css = gtk_css_provider_new();
    char style[4096];

    snprintf(style, sizeof(style),
             /* Window background */
             "window, .background {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "}\n"
             /* Text editor */
             "textview, textview text {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "}\n"
             /* Sidebar / tree view */
             "treeview, treeview.view {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "}\n"
             "treeview:selected {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "}\n"
             /* Toolbar */
             "toolbar {\n"
             "  background-color: %s;\n"
             "  border-bottom: 1px solid %s;\n"
             "}\n"
             /* Menu bar */
             "menubar {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "}\n"
             "menubar > menuitem {\n"
             "  color: %s;\n"
             "}\n"
             "menu {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "}\n"
             /* Status bar */
             ".statusbar, statusbar {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "  padding: 2px 8px;\n"
             "  border-top: 1px solid %s;\n"
             "}\n"
             /* Paned separator */
             "paned > separator {\n"
             "  background-color: %s;\n"
             "  min-width: 2px;\n"
             "  min-height: 2px;\n"
             "}\n"
             /* Notebook tabs */
             "notebook tab {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "  padding: 4px 8px;\n"
             "  border: 1px solid %s;\n"
             "}\n"
             "notebook tab:checked {\n"
             "  background-color: %s;\n"
             "  border-bottom-color: %s;\n"
             "}\n"
             /* Buttons */
             "button {\n"
             "  background-color: %s;\n"
             "  color: %s;\n"
             "  border: 1px solid %s;\n"
             "}\n"
             "button:hover {\n"
             "  background-color: %s;\n"
             "}\n"
             /* Scrollbar */
             "scrollbar {\n"
             "  background-color: %s;\n"
             "}\n"
             "scrollbar slider {\n"
             "  background-color: %s;\n"
             "  min-width: 8px;\n"
             "  min-height: 8px;\n"
             "}\n",
             /* Window */
             theme->bg, theme->fg,
             /* Editor */
             theme->bg, theme->fg,
             /* Sidebar */
             theme->sidebar_bg, theme->fg,
             /* Selection */
             theme->selection_bg, theme->selection_fg,
             /* Toolbar */
             theme->toolbar_bg, theme->border,
             /* Menu bar */
             theme->toolbar_bg, theme->fg, theme->fg,
             /* Menu dropdown */
             theme->sidebar_bg, theme->fg,
             /* Status bar */
             theme->statusbar_bg, theme->fg, theme->border,
             /* Paned separator */
             theme->border,
             /* Notebook tab inactive */
             theme->sidebar_bg, theme->fg, theme->border,
             /* Notebook tab active */
             theme->bg, theme->accent,
             /* Button */
             theme->sidebar_bg, theme->fg, theme->border,
             /* Button hover */
             theme->selection_bg,
             /* Scrollbar */
             theme->bg,
             /* Scrollbar slider */
             theme->border);

    gtk_css_provider_load_from_data(css, style, -1, NULL);

    GdkScreen* screen = gtk_widget_get_screen(window);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(css);
}
