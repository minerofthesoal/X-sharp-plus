/*
 * X# IDE - Editor Component
 * ==========================
 * GtkSourceView-based editor with tab management.
 */

#ifndef XS_EDITOR_H
#define XS_EDITOR_H

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <stdbool.h>

#define XS_EDITOR_MAX_TABS 64

/* A single editor tab */
typedef struct {
    GtkWidget       *source_view;
    GtkSourceBuffer *buffer;
    char             file_path[1024];
    bool             modified;
    GtkWidget       *tab_label;
    GtkWidget       *tab_box;    /* hbox with label + close button */
} XsEditorTab;

/* Editor component with notebook and tabs */
typedef struct XsEditor {
    GtkWidget     *notebook;
    XsEditorTab    tabs[XS_EDITOR_MAX_TABS];
    int            tab_count;
    GtkSourceLanguageManager *lang_manager;
    GtkSourceStyleSchemeManager *scheme_manager;
} XsEditor;

/* Lifecycle */
XsEditor   *xs_editor_new(void);
void        xs_editor_free(XsEditor *ed);
GtkWidget  *xs_editor_get_widget(XsEditor *ed);

/* Tab management */
int         xs_editor_new_tab(XsEditor *ed, const char *filepath);
bool        xs_editor_open_file(XsEditor *ed, const char *filepath);
bool        xs_editor_save_current(XsEditor *ed);
bool        xs_editor_save_as(XsEditor *ed, const char *filepath);
void        xs_editor_close_current(XsEditor *ed);
void        xs_editor_close_tab(XsEditor *ed, int index);
const char *xs_editor_get_current_path(XsEditor *ed);
char       *xs_editor_get_current_text(XsEditor *ed);

/* Editing actions */
void        xs_editor_undo(XsEditor *ed);
void        xs_editor_redo(XsEditor *ed);
void        xs_editor_show_find(XsEditor *ed);

/* Cursor info */
void        xs_editor_get_cursor(XsEditor *ed, int *line, int *col);

#endif /* XS_EDITOR_H */
