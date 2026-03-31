/*
 * X# IDE - Project Panel
 * ========================
 * GtkTreeView file browser sidebar.
 */

#ifndef XS_PROJECT_PANEL_H
#define XS_PROJECT_PANEL_H

#include <gtk/gtk.h>
#include <stdbool.h>

/* File open callback: called when user double-clicks a file */
typedef void (*XsProjectOpenCallback)(const char *filepath, void *user_data);

typedef struct XsProjectPanel {
    GtkWidget        *scrolled_window;
    GtkWidget        *tree_view;
    GtkTreeStore     *tree_store;
    char              root_dir[1024];
    XsProjectOpenCallback open_callback;
    void             *callback_data;
} XsProjectPanel;

/* Lifecycle */
XsProjectPanel *xs_project_panel_new(void);
void            xs_project_panel_free(XsProjectPanel *panel);
GtkWidget      *xs_project_panel_get_widget(XsProjectPanel *panel);

/* Load a directory */
void xs_project_panel_load_dir(XsProjectPanel *panel, const char *dir);

/* Refresh the tree */
void xs_project_panel_refresh(XsProjectPanel *panel);

/* Set callback for file opening */
void xs_project_panel_set_open_callback(XsProjectPanel *panel,
                                         XsProjectOpenCallback cb, void *data);

#endif /* XS_PROJECT_PANEL_H */
