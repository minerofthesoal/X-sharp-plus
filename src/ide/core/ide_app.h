/*
 * X# IDE - Main Application Header
 * ==================================
 * GTK3-based native IDE for the X# programming language.
 */

#ifndef XS_IDE_APP_H
#define XS_IDE_APP_H

#include <gtk/gtk.h>
#include <stdbool.h>

/* Maximum recent files tracked */
#define XS_IDE_MAX_RECENT_FILES 20
#define XS_IDE_APP_ID           "org.xsharp.ide"
#define XS_IDE_APP_TITLE        "X# IDE"
#define XS_IDE_VERSION          "1.0.0"

/* Forward declarations */
typedef struct XsEditor      XsEditor;
typedef struct XsProjectPanel XsProjectPanel;
typedef struct XsConsolePanel XsConsolePanel;
typedef struct XsDebugPanel  XsDebugPanel;
typedef struct XsProject     XsProject;
typedef struct XsTheme       XsTheme;

/* ===== IDE Preferences ===== */
typedef struct {
    char    theme_name[64];         /* "Obsidian" or "Radiance" */
    int     font_size;
    char    font_family[128];
    bool    show_line_numbers;
    bool    show_minimap;
    bool    auto_indent;
    bool    highlight_current_line;
    bool    bracket_matching;
    bool    word_wrap;
    int     tab_width;
    bool    use_spaces;
    bool    auto_save;
    int     auto_save_interval_sec;
    char    build_command[256];
    char    run_command[256];
} XsIdePrefs;

/* ===== Recent File Entry ===== */
typedef struct {
    char    path[1024];
    int64_t timestamp;
} XsRecentFile;

/* ===== Main IDE Application ===== */
typedef struct {
    GtkApplication  *app;
    GtkWidget       *window;

    /* Main layout */
    GtkWidget       *main_vbox;
    GtkWidget       *menu_bar;
    GtkWidget       *toolbar;
    GtkWidget       *hpaned;        /* horizontal: project panel | editor+debug */
    GtkWidget       *vpaned;        /* vertical:   editor | console */
    GtkWidget       *status_bar;

    /* Status bar labels */
    GtkWidget       *status_cursor;
    GtkWidget       *status_file_info;
    GtkWidget       *status_language;
    GtkWidget       *status_encoding;

    /* Components */
    XsEditor        *editor;
    XsProjectPanel  *project_panel;
    XsConsolePanel  *console_panel;
    XsDebugPanel    *debug_panel;
    XsProject       *project;
    XsTheme         *theme;

    /* State */
    XsIdePrefs       prefs;
    XsRecentFile     recent_files[XS_IDE_MAX_RECENT_FILES];
    int              recent_count;
    char             config_dir[1024];
    bool             is_debugging;
} XsIdeApp;

/* ===== Lifecycle ===== */
XsIdeApp*   xs_ide_app_new(void);
void        xs_ide_app_free(XsIdeApp *ide);
int         xs_ide_app_run(XsIdeApp *ide, int argc, char **argv);

/* ===== Window Management ===== */
void        xs_ide_app_set_title(XsIdeApp *ide, const char *subtitle);
void        xs_ide_app_update_status(XsIdeApp *ide, int line, int col,
                                     const char *filename, const char *encoding);

/* ===== Menu Actions ===== */
void        xs_ide_app_new_file(XsIdeApp *ide);
void        xs_ide_app_open_file(XsIdeApp *ide);
void        xs_ide_app_save_file(XsIdeApp *ide);
void        xs_ide_app_save_file_as(XsIdeApp *ide);
void        xs_ide_app_open_project(XsIdeApp *ide);
void        xs_ide_app_close_file(XsIdeApp *ide);
void        xs_ide_app_quit(XsIdeApp *ide);

void        xs_ide_app_build(XsIdeApp *ide);
void        xs_ide_app_run(XsIdeApp *ide, int argc, char **argv);
void        xs_ide_app_debug_start(XsIdeApp *ide);
void        xs_ide_app_debug_stop(XsIdeApp *ide);

/* ===== Preferences ===== */
void        xs_ide_prefs_load(XsIdeApp *ide);
void        xs_ide_prefs_save(XsIdeApp *ide);
void        xs_ide_prefs_show_dialog(XsIdeApp *ide);

/* ===== Recent Files ===== */
void        xs_ide_recent_add(XsIdeApp *ide, const char *path);
void        xs_ide_recent_load(XsIdeApp *ide);
void        xs_ide_recent_save(XsIdeApp *ide);

#endif /* XS_IDE_APP_H */
