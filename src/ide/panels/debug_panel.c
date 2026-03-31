/*
 * X# IDE - Debug Panel Implementation
 * ======================================
 * GtkTreeView for variables and call stack, with step buttons.
 */

#include "debug_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Variable store columns */
enum {
    VAR_COL_NAME,
    VAR_COL_TYPE,
    VAR_COL_VALUE,
    VAR_NUM_COLS
};

/* Call stack store columns */
enum {
    STACK_COL_FRAME,
    STACK_COL_FUNCTION,
    STACK_COL_LOCATION,
    STACK_NUM_COLS
};

XsDebugPanel *xs_debug_panel_new(void) {
    XsDebugPanel *panel = (XsDebugPanel *)calloc(1, sizeof(XsDebugPanel));
    if (!panel) return NULL;

    panel->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    /* ===== Step Buttons Toolbar ===== */
    panel->toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(panel->toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(panel->toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

    panel->btn_continue = GTK_WIDGET(gtk_tool_button_new(NULL, "Continue"));
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(panel->btn_continue), "media-playback-start");
    gtk_toolbar_insert(GTK_TOOLBAR(panel->toolbar), GTK_TOOL_ITEM(panel->btn_continue), -1);

    panel->btn_step_over = GTK_WIDGET(gtk_tool_button_new(NULL, "Step Over"));
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(panel->btn_step_over), "go-next");
    gtk_toolbar_insert(GTK_TOOLBAR(panel->toolbar), GTK_TOOL_ITEM(panel->btn_step_over), -1);

    panel->btn_step_in = GTK_WIDGET(gtk_tool_button_new(NULL, "Step In"));
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(panel->btn_step_in), "go-down");
    gtk_toolbar_insert(GTK_TOOLBAR(panel->toolbar), GTK_TOOL_ITEM(panel->btn_step_in), -1);

    panel->btn_step_out = GTK_WIDGET(gtk_tool_button_new(NULL, "Step Out"));
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(panel->btn_step_out), "go-up");
    gtk_toolbar_insert(GTK_TOOLBAR(panel->toolbar), GTK_TOOL_ITEM(panel->btn_step_out), -1);

    panel->btn_stop = GTK_WIDGET(gtk_tool_button_new(NULL, "Stop"));
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(panel->btn_stop), "process-stop");
    gtk_toolbar_insert(GTK_TOOLBAR(panel->toolbar), GTK_TOOL_ITEM(panel->btn_stop), -1);

    gtk_box_pack_start(GTK_BOX(panel->main_box), panel->toolbar, FALSE, FALSE, 0);

    /* ===== Variables Section ===== */
    GtkWidget *var_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(var_label), "<b>Variables</b>");
    gtk_widget_set_halign(var_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel->main_box), var_label, FALSE, FALSE, 4);

    panel->var_store = gtk_list_store_new(VAR_NUM_COLS,
                                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    panel->var_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(panel->var_store));

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *col;

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", VAR_COL_NAME, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->var_tree_view), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", VAR_COL_TYPE, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->var_tree_view), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Value", renderer, "text", VAR_COL_VALUE, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->var_tree_view), col);

    GtkWidget *var_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(var_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(var_scrolled), panel->var_tree_view);
    gtk_box_pack_start(GTK_BOX(panel->main_box), var_scrolled, TRUE, TRUE, 0);

    /* ===== Call Stack Section ===== */
    GtkWidget *stack_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(stack_label), "<b>Call Stack</b>");
    gtk_widget_set_halign(stack_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel->main_box), stack_label, FALSE, FALSE, 4);

    panel->stack_store = gtk_list_store_new(STACK_NUM_COLS,
                                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    panel->stack_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(panel->stack_store));

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("#", renderer, "text", STACK_COL_FRAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->stack_tree_view), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Function", renderer, "text", STACK_COL_FUNCTION, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->stack_tree_view), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Location", renderer, "text", STACK_COL_LOCATION, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->stack_tree_view), col);

    GtkWidget *stack_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(stack_scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(stack_scrolled), panel->stack_tree_view);
    gtk_box_pack_start(GTK_BOX(panel->main_box), stack_scrolled, TRUE, TRUE, 0);

    return panel;
}

void xs_debug_panel_free(XsDebugPanel *panel) {
    if (!panel) return;
    if (panel->var_store) g_object_unref(panel->var_store);
    if (panel->stack_store) g_object_unref(panel->stack_store);
    free(panel);
}

GtkWidget *xs_debug_panel_get_widget(XsDebugPanel *panel) {
    if (!panel) return NULL;
    return panel->main_box;
}

void xs_debug_panel_update_variables(XsDebugPanel *panel, const char **names,
                                      const char **types, const char **values, int count) {
    if (!panel) return;
    gtk_list_store_clear(panel->var_store);

    for (int i = 0; i < count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(panel->var_store, &iter);
        gtk_list_store_set(panel->var_store, &iter,
                           VAR_COL_NAME, names[i],
                           VAR_COL_TYPE, types[i],
                           VAR_COL_VALUE, values[i],
                           -1);
    }
}

void xs_debug_panel_update_call_stack(XsDebugPanel *panel, const char **func_names,
                                       const char **locations, int count) {
    if (!panel) return;
    gtk_list_store_clear(panel->stack_store);

    for (int i = 0; i < count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(panel->stack_store, &iter);
        char frame_num[16];
        snprintf(frame_num, sizeof(frame_num), "%d", i);
        gtk_list_store_set(panel->stack_store, &iter,
                           STACK_COL_FRAME, frame_num,
                           STACK_COL_FUNCTION, func_names[i],
                           STACK_COL_LOCATION, locations[i],
                           -1);
    }
}

void xs_debug_panel_clear(XsDebugPanel *panel) {
    if (!panel) return;
    gtk_list_store_clear(panel->var_store);
    gtk_list_store_clear(panel->stack_store);
}
