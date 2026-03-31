/*
 * X# IDE - Main Application Implementation
 * ==========================================
 * GTK3-based native IDE for the X# programming language.
 */

#include "ide_app.h"
#include "../editor/editor.h"
#include "../panels/console_panel.h"
#include "../panels/debug_panel.h"
#include "../panels/project_panel.h"
#include "../themes/theme.h"
#include "project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* ===== Forward Declarations ===== */
static void on_activate(GtkApplication* app, gpointer user_data);
static void build_menu_bar(XsIdeApp* ide);
static void build_toolbar(XsIdeApp* ide);
static void build_status_bar(XsIdeApp* ide);

/* ===== Menu Action Callbacks ===== */
static void on_new_file(GtkMenuItem* item, gpointer data) {
    xs_ide_app_new_file((XsIdeApp*)data);
}
static void on_open_file(GtkMenuItem* item, gpointer data) {
    xs_ide_app_open_file((XsIdeApp*)data);
}
static void on_save_file(GtkMenuItem* item, gpointer data) {
    xs_ide_app_save_file((XsIdeApp*)data);
}
static void on_save_as(GtkMenuItem* item, gpointer data) {
    xs_ide_app_save_file_as((XsIdeApp*)data);
}
static void on_quit(GtkMenuItem* item, gpointer data) {
    xs_ide_app_quit((XsIdeApp*)data);
}
static void on_build(GtkMenuItem* item, gpointer data) {
    xs_ide_app_build((XsIdeApp*)data);
}
static void on_run(GtkMenuItem* item, gpointer data) { /* run action */
    xs_ide_app_build((XsIdeApp*)data);
}
static void on_debug(GtkMenuItem* item, gpointer data) {
    xs_ide_app_debug_start((XsIdeApp*)data);
}

static void on_undo(GtkMenuItem* item, gpointer data) {
    XsIdeApp* ide = (XsIdeApp*)data;
    if (ide->editor)
        xs_editor_undo(ide->editor);
}

static void on_redo(GtkMenuItem* item, gpointer data) {
    XsIdeApp* ide = (XsIdeApp*)data;
    if (ide->editor)
        xs_editor_redo(ide->editor);
}

static void on_find(GtkMenuItem* item, gpointer data) {
    XsIdeApp* ide = (XsIdeApp*)data;
    if (ide->editor)
        xs_editor_show_find(ide->editor);
}

static void on_theme_toggle(GtkMenuItem* item, gpointer data) {
    XsIdeApp* ide = (XsIdeApp*)data;
    if (!ide->theme)
        return;
    if (strcmp(ide->prefs.theme_name, "Obsidian") == 0) {
        *ide->theme = xs_theme_radiance();
        strncpy(ide->prefs.theme_name, "Radiance", sizeof(ide->prefs.theme_name) - 1);
    } else {
        *ide->theme = xs_theme_obsidian();
        strncpy(ide->prefs.theme_name, "Obsidian", sizeof(ide->prefs.theme_name) - 1);
    }
    xs_theme_apply(ide->theme, ide->window);
}

static void on_about(GtkMenuItem* item, gpointer data) {
    XsIdeApp* ide = (XsIdeApp*)data;
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(ide->window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "X# IDE v%s\n\nA native IDE for the X# programming language.\n"
        "Built with GTK3 and GtkSourceView.",
        XS_IDE_VERSION);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* ===== Lifecycle ===== */

XsIdeApp* xs_ide_app_new(void) {
    XsIdeApp* ide = (XsIdeApp*)calloc(1, sizeof(XsIdeApp));
    if (!ide)
        return NULL;

    /* Set default preferences */
    strncpy(ide->prefs.theme_name, "Obsidian", sizeof(ide->prefs.theme_name) - 1);
    ide->prefs.font_size = 12;
    strncpy(ide->prefs.font_family, "Monospace", sizeof(ide->prefs.font_family) - 1);
    ide->prefs.show_line_numbers = true;
    ide->prefs.show_minimap = false;
    ide->prefs.auto_indent = true;
    ide->prefs.highlight_current_line = true;
    ide->prefs.bracket_matching = true;
    ide->prefs.word_wrap = false;
    ide->prefs.tab_width = 4;
    ide->prefs.use_spaces = true;
    ide->prefs.auto_save = false;
    ide->prefs.auto_save_interval_sec = 30;
    strncpy(ide->prefs.build_command, "xsharp build", sizeof(ide->prefs.build_command) - 1);
    strncpy(ide->prefs.run_command, "xsharp run", sizeof(ide->prefs.run_command) - 1);

    ide->recent_count = 0;
    ide->is_debugging = false;

    /* Config directory */
    const char* home = getenv("HOME");
    if (home) {
        snprintf(ide->config_dir, sizeof(ide->config_dir), "%s/.config/xsharp-ide", home);
        mkdir(ide->config_dir, 0755);
    }

    return ide;
}

void xs_ide_app_free(XsIdeApp* ide) {
    if (!ide)
        return;

    if (ide->editor)
        xs_editor_free(ide->editor);
    if (ide->project_panel)
        xs_project_panel_free(ide->project_panel);
    if (ide->console_panel)
        xs_console_panel_free(ide->console_panel);
    if (ide->debug_panel)
        xs_debug_panel_free(ide->debug_panel);
    if (ide->project)
        xs_project_free(ide->project);
    if (ide->theme)
        xs_theme_free(ide->theme);
    if (ide->app)
        g_object_unref(ide->app);

    free(ide);
}

/* ===== GtkApplication Activate Callback ===== */
static void on_activate(GtkApplication* app, gpointer user_data) {
    XsIdeApp* ide = (XsIdeApp*)user_data;

    /* Create main window */
    ide->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(ide->window), XS_IDE_APP_TITLE);
    gtk_window_set_default_size(GTK_WINDOW(ide->window), 1200, 800);
    gtk_window_set_position(GTK_WINDOW(ide->window), GTK_WIN_POS_CENTER);

    /* Main vertical box */
    ide->main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(ide->window), ide->main_vbox);

    /* Build menu bar */
    build_menu_bar(ide);
    gtk_box_pack_start(GTK_BOX(ide->main_vbox), ide->menu_bar, FALSE, FALSE, 0);

    /* Build toolbar */
    build_toolbar(ide);
    gtk_box_pack_start(GTK_BOX(ide->main_vbox), ide->toolbar, FALSE, FALSE, 0);

    /* Create components */
    ide->editor = xs_editor_new();
    ide->project_panel = xs_project_panel_new();
    ide->console_panel = xs_console_panel_new();
    ide->debug_panel = xs_debug_panel_new();
    ide->theme = xs_theme_new(); /* defaults to Obsidian */

    /* Apply default theme */
    xs_theme_apply(ide->theme, ide->window);

    /* HPaned: project panel | editor + console */
    ide->hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(ide->main_vbox), ide->hpaned, TRUE, TRUE, 0);

    /* Project panel in left side */
    GtkWidget* project_widget = xs_project_panel_get_widget(ide->project_panel);
    gtk_paned_pack1(GTK_PANED(ide->hpaned), project_widget, FALSE, TRUE);
    gtk_paned_set_position(GTK_PANED(ide->hpaned), 250);

    /* VPaned: editor | console in right side */
    ide->vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gtk_paned_pack2(GTK_PANED(ide->hpaned), ide->vpaned, TRUE, TRUE);

    /* Editor notebook at top */
    GtkWidget* editor_widget = xs_editor_get_widget(ide->editor);
    gtk_paned_pack1(GTK_PANED(ide->vpaned), editor_widget, TRUE, TRUE);

    /* Console panel at bottom */
    GtkWidget* console_widget = xs_console_panel_get_widget(ide->console_panel);
    gtk_paned_pack2(GTK_PANED(ide->vpaned), console_widget, FALSE, TRUE);
    gtk_paned_set_position(GTK_PANED(ide->vpaned), 500);

    /* Build status bar */
    build_status_bar(ide);
    gtk_box_pack_end(GTK_BOX(ide->main_vbox), ide->status_bar, FALSE, FALSE, 0);

    /* Show all widgets */
    gtk_widget_show_all(ide->window);
}

/* ===== Run ===== */
int xs_ide_app_run(XsIdeApp* ide, int argc, char** argv) {
    if (!ide)
        return 1;

    ide->app = gtk_application_new(XS_IDE_APP_ID, G_APPLICATION_FLAGS_NONE);
    g_signal_connect(ide->app, "activate", G_CALLBACK(on_activate), ide);

    int status = g_application_run(G_APPLICATION(ide->app), argc, argv);
    return status;
}

/* ===== Menu Bar ===== */
static GtkWidget* create_menu_item(const char* label, GCallback cb, gpointer data) {
    GtkWidget* item = gtk_menu_item_new_with_mnemonic(label);
    if (cb)
        g_signal_connect(item, "activate", cb, data);
    return item;
}

static void build_menu_bar(XsIdeApp* ide) {
    ide->menu_bar = gtk_menu_bar_new();

    /* File menu */
    GtkWidget* file_menu = gtk_menu_new();
    GtkWidget* file_item = gtk_menu_item_new_with_mnemonic("_File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu),
                          create_menu_item("_New", G_CALLBACK(on_new_file), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu),
                          create_menu_item("_Open...", G_CALLBACK(on_open_file), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu),
                          create_menu_item("_Save", G_CALLBACK(on_save_file), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu),
                          create_menu_item("Save _As...", G_CALLBACK(on_save_as), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu),
                          create_menu_item("_Quit", G_CALLBACK(on_quit), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(ide->menu_bar), file_item);

    /* Edit menu */
    GtkWidget* edit_menu = gtk_menu_new();
    GtkWidget* edit_item = gtk_menu_item_new_with_mnemonic("_Edit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_item), edit_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu),
                          create_menu_item("_Undo", G_CALLBACK(on_undo), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu),
                          create_menu_item("_Redo", G_CALLBACK(on_redo), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu),
                          create_menu_item("_Find...", G_CALLBACK(on_find), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(ide->menu_bar), edit_item);

    /* View menu */
    GtkWidget* view_menu = gtk_menu_new();
    GtkWidget* view_item = gtk_menu_item_new_with_mnemonic("_View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_menu),
                          create_menu_item("Toggle _Theme", G_CALLBACK(on_theme_toggle), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(ide->menu_bar), view_item);

    /* Build menu */
    GtkWidget* build_menu = gtk_menu_new();
    GtkWidget* build_item = gtk_menu_item_new_with_mnemonic("_Build");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(build_item), build_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(build_menu),
                          create_menu_item("_Build Project", G_CALLBACK(on_build), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(build_menu),
                          create_menu_item("_Run", G_CALLBACK(on_run), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(build_menu),
                          create_menu_item("_Debug", G_CALLBACK(on_debug), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(ide->menu_bar), build_item);

    /* Help menu */
    GtkWidget* help_menu = gtk_menu_new();
    GtkWidget* help_item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu),
                          create_menu_item("_About", G_CALLBACK(on_about), ide));
    gtk_menu_shell_append(GTK_MENU_SHELL(ide->menu_bar), help_item);
}

/* ===== Toolbar ===== */
static void build_toolbar(XsIdeApp* ide) {
    ide->toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(ide->toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_icon_size(GTK_TOOLBAR(ide->toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);

    GtkToolItem* btn_new = gtk_tool_button_new(NULL, "New");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_new), "document-new");
    g_signal_connect(btn_new, "clicked", G_CALLBACK(on_new_file), ide);
    gtk_toolbar_insert(GTK_TOOLBAR(ide->toolbar), btn_new, -1);

    GtkToolItem* btn_open = gtk_tool_button_new(NULL, "Open");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_open), "document-open");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_file), ide);
    gtk_toolbar_insert(GTK_TOOLBAR(ide->toolbar), btn_open, -1);

    GtkToolItem* btn_save = gtk_tool_button_new(NULL, "Save");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_save), "document-save");
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_file), ide);
    gtk_toolbar_insert(GTK_TOOLBAR(ide->toolbar), btn_save, -1);

    gtk_toolbar_insert(GTK_TOOLBAR(ide->toolbar), gtk_separator_tool_item_new(), -1);

    GtkToolItem* btn_build = gtk_tool_button_new(NULL, "Build");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_build), "system-run");
    g_signal_connect(btn_build, "clicked", G_CALLBACK(on_build), ide);
    gtk_toolbar_insert(GTK_TOOLBAR(ide->toolbar), btn_build, -1);

    GtkToolItem* btn_run = gtk_tool_button_new(NULL, "Run");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_run), "media-playback-start");
    g_signal_connect(btn_run, "clicked", G_CALLBACK(on_run), ide);
    gtk_toolbar_insert(GTK_TOOLBAR(ide->toolbar), btn_run, -1);

    GtkToolItem* btn_debug = gtk_tool_button_new(NULL, "Debug");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(btn_debug), "tools-check-spelling");
    g_signal_connect(btn_debug, "clicked", G_CALLBACK(on_debug), ide);
    gtk_toolbar_insert(GTK_TOOLBAR(ide->toolbar), btn_debug, -1);
}

/* ===== Status Bar ===== */
static void build_status_bar(XsIdeApp* ide) {
    ide->status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    ide->status_cursor = gtk_label_new("Ln 1, Col 1");
    ide->status_file_info = gtk_label_new("Untitled");
    ide->status_language = gtk_label_new("X#");
    ide->status_encoding = gtk_label_new("UTF-8");

    gtk_box_pack_start(GTK_BOX(ide->status_bar), ide->status_file_info, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ide->status_bar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE,
                       FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ide->status_bar), ide->status_cursor, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(ide->status_bar), ide->status_encoding, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(ide->status_bar), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE,
                     FALSE, 0);
    gtk_box_pack_end(GTK_BOX(ide->status_bar), ide->status_language, FALSE, FALSE, 5);
}

/* ===== Window Management ===== */
void xs_ide_app_set_title(XsIdeApp* ide, const char* subtitle) {
    if (!ide || !ide->window)
        return;
    char title[512];
    if (subtitle && subtitle[0]) {
        snprintf(title, sizeof(title), "%s - %s", subtitle, XS_IDE_APP_TITLE);
    } else {
        snprintf(title, sizeof(title), "%s", XS_IDE_APP_TITLE);
    }
    gtk_window_set_title(GTK_WINDOW(ide->window), title);
}

void xs_ide_app_update_status(XsIdeApp* ide, int line, int col, const char* filename,
                              const char* encoding) {
    if (!ide)
        return;
    char buf[256];
    snprintf(buf, sizeof(buf), "Ln %d, Col %d", line, col);
    gtk_label_set_text(GTK_LABEL(ide->status_cursor), buf);
    if (filename)
        gtk_label_set_text(GTK_LABEL(ide->status_file_info), filename);
    if (encoding)
        gtk_label_set_text(GTK_LABEL(ide->status_encoding), encoding);
}

/* ===== Menu Actions ===== */
void xs_ide_app_new_file(XsIdeApp* ide) {
    if (!ide || !ide->editor)
        return;
    xs_editor_new_tab(ide->editor, NULL);
    xs_ide_app_set_title(ide, "Untitled");
}

void xs_ide_app_open_file(XsIdeApp* ide) {
    if (!ide || !ide->window)
        return;
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Open File", GTK_WINDOW(ide->window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel",
        GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "X# Files (*.xs)");
    gtk_file_filter_add_pattern(filter, "*.xs");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    GtkFileFilter* all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All Files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            xs_editor_open_file(ide->editor, filename);
            xs_ide_app_set_title(ide, filename);
            xs_ide_recent_add(ide, filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

void xs_ide_app_save_file(XsIdeApp* ide) {
    if (!ide || !ide->editor)
        return;
    const char* path = xs_editor_get_current_path(ide->editor);
    if (path && path[0]) {
        xs_editor_save_current(ide->editor);
    } else {
        xs_ide_app_save_file_as(ide);
    }
}

void xs_ide_app_save_file_as(XsIdeApp* ide) {
    if (!ide || !ide->window || !ide->editor)
        return;
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Save File As", GTK_WINDOW(ide->window), GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel",
        GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            xs_editor_save_as(ide->editor, filename);
            xs_ide_app_set_title(ide, filename);
            xs_ide_recent_add(ide, filename);
            g_free(filename);
        }
    }
    gtk_widget_destroy(dialog);
}

void xs_ide_app_open_project(XsIdeApp* ide) {
    if (!ide || !ide->window)
        return;
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Open Project Directory", GTK_WINDOW(ide->window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* dirname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (dirname) {
            xs_project_panel_load_dir(ide->project_panel, dirname);
            xs_ide_app_set_title(ide, dirname);
            g_free(dirname);
        }
    }
    gtk_widget_destroy(dialog);
}

void xs_ide_app_close_file(XsIdeApp* ide) {
    if (!ide || !ide->editor)
        return;
    xs_editor_close_current(ide->editor);
}

void xs_ide_app_quit(XsIdeApp* ide) {
    if (!ide)
        return;
    xs_ide_prefs_save(ide);
    if (ide->app) {
        g_application_quit(G_APPLICATION(ide->app));
    }
}

void xs_ide_app_build(XsIdeApp* ide) {
    if (!ide || !ide->console_panel)
        return;
    xs_console_panel_clear(ide->console_panel);
    xs_console_panel_append(ide->console_panel, "Building project...\n", "info");

    /* Save current file first */
    if (ide->editor)
        xs_editor_save_current(ide->editor);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "%s 2>&1", ide->prefs.build_command);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        xs_console_panel_append(ide->console_panel, "Failed to execute build command.\n", "error");
        return;
    }

    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) {
        xs_console_panel_append(ide->console_panel, buf, "output");
    }

    int status = pclose(pipe);
    if (status == 0) {
        xs_console_panel_append(ide->console_panel, "\nBuild succeeded.\n", "success");
    } else {
        xs_console_panel_append(ide->console_panel, "\nBuild failed.\n", "error");
    }
}

void xs_ide_app_debug_start(XsIdeApp* ide) {
    if (!ide || !ide->console_panel)
        return;
    ide->is_debugging = true;
    xs_console_panel_append(ide->console_panel, "Starting debugger...\n", "info");
    /* In a full implementation, this would launch the DAP debugger */
}

void xs_ide_app_debug_stop(XsIdeApp* ide) {
    if (!ide)
        return;
    ide->is_debugging = false;
    if (ide->console_panel)
        xs_console_panel_append(ide->console_panel, "Debugger stopped.\n", "info");
}

/* ===== Preferences ===== */
void xs_ide_prefs_load(XsIdeApp* ide) {
    if (!ide)
        return;
    char path[1100];
    snprintf(path, sizeof(path), "%s/preferences.ini", ide->config_dir);

    FILE* f = fopen(path, "r");
    if (!f)
        return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char* eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = '\0';
        char* key = line;
        char* val = eq + 1;
        /* Trim newline */
        char* nl = strchr(val, '\n');
        if (nl)
            *nl = '\0';
        /* Trim leading space */
        while (*val == ' ')
            val++;
        while (*key == ' ')
            key++;

        if (strcmp(key, "theme") == 0)
            strncpy(ide->prefs.theme_name, val, sizeof(ide->prefs.theme_name) - 1);
        else if (strcmp(key, "font_size") == 0)
            ide->prefs.font_size = atoi(val);
        else if (strcmp(key, "tab_width") == 0)
            ide->prefs.tab_width = atoi(val);
    }
    fclose(f);
}

void xs_ide_prefs_save(XsIdeApp* ide) {
    if (!ide)
        return;
    char path[1100];
    snprintf(path, sizeof(path), "%s/preferences.ini", ide->config_dir);

    FILE* f = fopen(path, "w");
    if (!f)
        return;

    fprintf(f, "theme = %s\n", ide->prefs.theme_name);
    fprintf(f, "font_size = %d\n", ide->prefs.font_size);
    fprintf(f, "font_family = %s\n", ide->prefs.font_family);
    fprintf(f, "tab_width = %d\n", ide->prefs.tab_width);
    fprintf(f, "show_line_numbers = %d\n", ide->prefs.show_line_numbers);
    fprintf(f, "auto_indent = %d\n", ide->prefs.auto_indent);
    fprintf(f, "bracket_matching = %d\n", ide->prefs.bracket_matching);
    fprintf(f, "word_wrap = %d\n", ide->prefs.word_wrap);
    fprintf(f, "use_spaces = %d\n", ide->prefs.use_spaces);
    fprintf(f, "build_command = %s\n", ide->prefs.build_command);
    fprintf(f, "run_command = %s\n", ide->prefs.run_command);
    fclose(f);
}

void xs_ide_prefs_show_dialog(XsIdeApp* ide) {
    /* Stub: in full implementation, show a preferences dialog */
    (void)ide;
}

/* ===== Recent Files ===== */
void xs_ide_recent_add(XsIdeApp* ide, const char* path) {
    if (!ide || !path)
        return;

    /* Check if already in list */
    for (int i = 0; i < ide->recent_count; i++) {
        if (strcmp(ide->recent_files[i].path, path) == 0) {
            ide->recent_files[i].timestamp = (int64_t)time(NULL);
            return;
        }
    }

    /* Add new entry */
    if (ide->recent_count < XS_IDE_MAX_RECENT_FILES) {
        XsRecentFile* rf = &ide->recent_files[ide->recent_count++];
        strncpy(rf->path, path, sizeof(rf->path) - 1);
        rf->timestamp = (int64_t)time(NULL);
    } else {
        /* Replace oldest */
        int oldest = 0;
        for (int i = 1; i < ide->recent_count; i++) {
            if (ide->recent_files[i].timestamp < ide->recent_files[oldest].timestamp)
                oldest = i;
        }
        strncpy(ide->recent_files[oldest].path, path, sizeof(ide->recent_files[oldest].path) - 1);
        ide->recent_files[oldest].timestamp = (int64_t)time(NULL);
    }
}

void xs_ide_recent_load(XsIdeApp* ide) {
    if (!ide)
        return;
    char path[1100];
    snprintf(path, sizeof(path), "%s/recent_files.txt", ide->config_dir);

    FILE* f = fopen(path, "r");
    if (!f)
        return;

    char line[1100];
    ide->recent_count = 0;
    while (fgets(line, sizeof(line), f) && ide->recent_count < XS_IDE_MAX_RECENT_FILES) {
        char* nl = strchr(line, '\n');
        if (nl)
            *nl = '\0';
        if (line[0]) {
            strncpy(ide->recent_files[ide->recent_count].path, line,
                    sizeof(ide->recent_files[ide->recent_count].path) - 1);
            ide->recent_files[ide->recent_count].timestamp = (int64_t)time(NULL);
            ide->recent_count++;
        }
    }
    fclose(f);
}

void xs_ide_recent_save(XsIdeApp* ide) {
    if (!ide)
        return;
    char path[1100];
    snprintf(path, sizeof(path), "%s/recent_files.txt", ide->config_dir);

    FILE* f = fopen(path, "w");
    if (!f)
        return;

    for (int i = 0; i < ide->recent_count; i++) {
        fprintf(f, "%s\n", ide->recent_files[i].path);
    }
    fclose(f);
}
