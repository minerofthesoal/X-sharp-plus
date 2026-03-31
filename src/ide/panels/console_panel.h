/*
 * X# IDE - Console Panel
 * ========================
 * GtkTextView-based console output with colored text.
 */

#ifndef XS_CONSOLE_PANEL_H
#define XS_CONSOLE_PANEL_H

#include <gtk/gtk.h>

typedef struct XsConsolePanel {
    GtkWidget* scrolled_window;
    GtkWidget* text_view;
    GtkTextBuffer* buffer;
    GtkTextTag* tag_info;
    GtkTextTag* tag_error;
    GtkTextTag* tag_success;
    GtkTextTag* tag_output;
    GtkTextTag* tag_warning;
} XsConsolePanel;

/* Lifecycle */
XsConsolePanel* xs_console_panel_new(void);
void xs_console_panel_free(XsConsolePanel* panel);
GtkWidget* xs_console_panel_get_widget(XsConsolePanel* panel);

/* Operations */
void xs_console_panel_append(XsConsolePanel* panel, const char* text, const char* tag_name);
void xs_console_panel_clear(XsConsolePanel* panel);

#endif /* XS_CONSOLE_PANEL_H */
