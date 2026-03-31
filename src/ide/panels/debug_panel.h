/*
 * X# IDE - Debug Panel
 * ======================
 * Variable inspector and call stack display.
 */

#ifndef XS_DEBUG_PANEL_H
#define XS_DEBUG_PANEL_H

#include <gtk/gtk.h>
#include <stdbool.h>

typedef struct XsDebugPanel {
    GtkWidget    *main_box;
    /* Variables tree */
    GtkWidget    *var_tree_view;
    GtkListStore *var_store;
    /* Call stack tree */
    GtkWidget    *stack_tree_view;
    GtkListStore *stack_store;
    /* Step buttons toolbar */
    GtkWidget    *toolbar;
    GtkWidget    *btn_continue;
    GtkWidget    *btn_step_in;
    GtkWidget    *btn_step_over;
    GtkWidget    *btn_step_out;
    GtkWidget    *btn_stop;
} XsDebugPanel;

/* Lifecycle */
XsDebugPanel *xs_debug_panel_new(void);
void          xs_debug_panel_free(XsDebugPanel *panel);
GtkWidget    *xs_debug_panel_get_widget(XsDebugPanel *panel);

/* Update with data */
void xs_debug_panel_update_variables(XsDebugPanel *panel, const char **names,
                                      const char **types, const char **values, int count);
void xs_debug_panel_update_call_stack(XsDebugPanel *panel, const char **func_names,
                                       const char **locations, int count);
void xs_debug_panel_clear(XsDebugPanel *panel);

#endif /* XS_DEBUG_PANEL_H */
