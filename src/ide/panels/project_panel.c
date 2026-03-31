/*
 * X# IDE - Project Panel Implementation
 * ========================================
 * GtkTreeView file browser with recursive directory scanning,
 * double-click-to-open, and context menu.
 */

#include "project_panel.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Tree store columns */
enum { COL_ICON, COL_NAME, COL_PATH, COL_IS_DIR, NUM_COLS };

/* ===== Recursive Directory Scan ===== */
static void scan_directory(GtkTreeStore* store, GtkTreeIter* parent, const char* dir_path) {
    DIR* d = opendir(dir_path);
    if (!d)
        return;

    struct dirent* ent;
    while ((ent = readdir(d)) != NULL) {
        /* Skip hidden files and . / .. */
        if (ent->d_name[0] == '.')
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0)
            continue;

        GtkTreeIter iter;
        gtk_tree_store_append(store, &iter, parent);

        bool is_dir = S_ISDIR(st.st_mode);
        const char* icon = is_dir ? "folder" : "text-x-generic";

        /* Use special icon for .xs files */
        if (!is_dir) {
            size_t nlen = strlen(ent->d_name);
            if (nlen > 3 && strcmp(ent->d_name + nlen - 3, ".xs") == 0) {
                icon = "text-x-script";
            }
        }

        gtk_tree_store_set(store, &iter, COL_ICON, icon, COL_NAME, ent->d_name, COL_PATH, full_path,
                           COL_IS_DIR, is_dir, -1);

        /* Recursively scan subdirectories */
        if (is_dir) {
            scan_directory(store, &iter, full_path);
        }
    }
    closedir(d);
}

/* ===== Double-click Callback ===== */
static void on_row_activated(GtkTreeView* tree_view, GtkTreePath* path, GtkTreeViewColumn* column,
                             gpointer data) {
    XsProjectPanel* panel = (XsProjectPanel*)data;
    (void)column;

    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(panel->tree_store), &iter, path))
        return;

    gboolean is_dir;
    gchar* filepath;
    gtk_tree_model_get(GTK_TREE_MODEL(panel->tree_store), &iter, COL_PATH, &filepath, COL_IS_DIR,
                       &is_dir, -1);

    if (!is_dir && panel->open_callback) {
        panel->open_callback(filepath, panel->callback_data);
    }

    g_free(filepath);
}

/* ===== Context Menu ===== */
static void on_context_refresh(GtkMenuItem* item, gpointer data) {
    (void)item;
    XsProjectPanel* panel = (XsProjectPanel*)data;
    xs_project_panel_refresh(panel);
}

static gboolean on_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    XsProjectPanel* panel = (XsProjectPanel*)data;
    (void)widget;

    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        /* Right-click: show context menu */
        GtkWidget* menu = gtk_menu_new();

        GtkWidget* refresh_item = gtk_menu_item_new_with_label("Refresh");
        g_signal_connect(refresh_item, "activate", G_CALLBACK(on_context_refresh), panel);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), refresh_item);

        GtkWidget* collapse_item = gtk_menu_item_new_with_label("Collapse All");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), collapse_item);

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
        return TRUE;
    }
    return FALSE;
}

/* ===== Lifecycle ===== */
XsProjectPanel* xs_project_panel_new(void) {
    XsProjectPanel* panel = (XsProjectPanel*)calloc(1, sizeof(XsProjectPanel));
    if (!panel)
        return NULL;

    /* Create tree store: icon-name, display-name, full-path, is-directory */
    panel->tree_store =
        gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

    /* Create tree view */
    panel->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(panel->tree_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(panel->tree_view), FALSE);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(panel->tree_view), TRUE);

    /* Icon + Name column */
    GtkTreeViewColumn* col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Files");

    GtkCellRenderer* icon_renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(col, icon_renderer, FALSE);
    gtk_tree_view_column_set_attributes(col, icon_renderer, "icon-name", COL_ICON, NULL);

    GtkCellRenderer* text_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, text_renderer, TRUE);
    gtk_tree_view_column_set_attributes(col, text_renderer, "text", COL_NAME, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(panel->tree_view), col);

    /* Signals */
    g_signal_connect(panel->tree_view, "row-activated", G_CALLBACK(on_row_activated), panel);
    g_signal_connect(panel->tree_view, "button-press-event", G_CALLBACK(on_button_press), panel);

    /* Scrolled window */
    panel->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(panel->scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(panel->scrolled_window), 200);
    gtk_container_add(GTK_CONTAINER(panel->scrolled_window), panel->tree_view);

    return panel;
}

void xs_project_panel_free(XsProjectPanel* panel) {
    if (!panel)
        return;
    if (panel->tree_store)
        g_object_unref(panel->tree_store);
    free(panel);
}

GtkWidget* xs_project_panel_get_widget(XsProjectPanel* panel) {
    if (!panel)
        return NULL;
    return panel->scrolled_window;
}

void xs_project_panel_load_dir(XsProjectPanel* panel, const char* dir) {
    if (!panel || !dir)
        return;
    strncpy(panel->root_dir, dir, sizeof(panel->root_dir) - 1);
    xs_project_panel_refresh(panel);
}

void xs_project_panel_refresh(XsProjectPanel* panel) {
    if (!panel || !panel->root_dir[0])
        return;
    gtk_tree_store_clear(panel->tree_store);

    /* Add root node */
    const char* basename = strrchr(panel->root_dir, '/');
    basename = basename ? basename + 1 : panel->root_dir;

    GtkTreeIter root_iter;
    gtk_tree_store_append(panel->tree_store, &root_iter, NULL);
    gtk_tree_store_set(panel->tree_store, &root_iter, COL_ICON, "folder", COL_NAME, basename,
                       COL_PATH, panel->root_dir, COL_IS_DIR, TRUE, -1);

    scan_directory(panel->tree_store, &root_iter, panel->root_dir);

    /* Expand root */
    GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(panel->tree_store), &root_iter);
    gtk_tree_view_expand_row(GTK_TREE_VIEW(panel->tree_view), path, FALSE);
    gtk_tree_path_free(path);
}

void xs_project_panel_set_open_callback(XsProjectPanel* panel, XsProjectOpenCallback cb,
                                        void* data) {
    if (!panel)
        return;
    panel->open_callback = cb;
    panel->callback_data = data;
}
