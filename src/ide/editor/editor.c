/*
 * X# IDE - Editor Implementation
 * ================================
 * GtkSourceView editor with tabs, line numbers, syntax highlighting,
 * bracket matching, open/save/close functionality.
 */

#include "editor.h"
#include "syntax.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Tab Close Button Callback ===== */
typedef struct {
    XsEditor* editor;
    int tab_index;
} TabCloseData;

static void on_tab_close_clicked(GtkButton* btn, gpointer data) {
    TabCloseData* tcd = (TabCloseData*)data;
    xs_editor_close_tab(tcd->editor, tcd->tab_index);
    free(tcd);
}

/* ===== Buffer Modified Callback ===== */
static void on_buffer_modified(GtkTextBuffer* buffer, gpointer data) {
    XsEditorTab* tab = (XsEditorTab*)data;
    tab->modified = gtk_text_buffer_get_modified(buffer);
    if (tab->modified && tab->tab_label) {
        const char* current = gtk_label_get_text(GTK_LABEL(tab->tab_label));
        if (current && current[0] != '*') {
            char buf[256];
            snprintf(buf, sizeof(buf), "*%s", current);
            gtk_label_set_text(GTK_LABEL(tab->tab_label), buf);
        }
    }
}

/* ===== Create Source View with Settings ===== */
static GtkWidget* create_source_view(XsEditor* ed, GtkSourceBuffer** out_buffer) {
    GtkSourceBuffer* buffer = gtk_source_buffer_new(NULL);

    /* Set X# language if available */
    GtkSourceLanguage* lang = gtk_source_language_manager_get_language(ed->lang_manager, "xsharp");
    if (lang) {
        gtk_source_buffer_set_language(buffer, lang);
    }

    /* Enable bracket matching */
    gtk_source_buffer_set_highlight_matching_brackets(buffer, TRUE);
    gtk_source_buffer_set_highlight_syntax(buffer, TRUE);

    GtkWidget* view = gtk_source_view_new_with_buffer(buffer);

    /* Configure source view */
    gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_indent_on_tab(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(view), 4);
    gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(view), TRUE);
    gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(view), 100);

    /* Set monospace font */
    PangoFontDescription* font = pango_font_description_from_string("Monospace 12");
    gtk_widget_override_font(view, font);
    pango_font_description_free(font);

    /* Apply color scheme */
    GtkSourceStyleScheme* scheme =
        gtk_source_style_scheme_manager_get_scheme(ed->scheme_manager, "oblivion");
    if (scheme) {
        gtk_source_buffer_set_style_scheme(buffer, scheme);
    }

    *out_buffer = buffer;
    return view;
}

/* ===== Lifecycle ===== */
XsEditor* xs_editor_new(void) {
    XsEditor* ed = (XsEditor*)calloc(1, sizeof(XsEditor));
    if (!ed)
        return NULL;

    ed->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(ed->notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(ed->notebook), FALSE);

    ed->lang_manager = gtk_source_language_manager_get_default();
    ed->scheme_manager = gtk_source_style_scheme_manager_get_default();
    ed->tab_count = 0;

    /* Initialize X# syntax */
    xs_syntax_init(ed->lang_manager);

    /* Create initial empty tab */
    xs_editor_new_tab(ed, NULL);

    return ed;
}

void xs_editor_free(XsEditor* ed) {
    if (!ed)
        return;
    /* GTK handles widget cleanup when parent is destroyed */
    free(ed);
}

GtkWidget* xs_editor_get_widget(XsEditor* ed) {
    if (!ed)
        return NULL;
    return ed->notebook;
}

/* ===== Tab Management ===== */
int xs_editor_new_tab(XsEditor* ed, const char* filepath) {
    if (!ed || ed->tab_count >= XS_EDITOR_MAX_TABS)
        return -1;

    int idx = ed->tab_count;
    XsEditorTab* tab = &ed->tabs[idx];
    memset(tab, 0, sizeof(XsEditorTab));

    /* Create source view */
    GtkSourceBuffer* buffer = NULL;
    tab->source_view = create_source_view(ed, &buffer);
    tab->buffer = buffer;

    if (filepath) {
        strncpy(tab->file_path, filepath, sizeof(tab->file_path) - 1);
    }

    /* Connect modified signal */
    g_signal_connect(GTK_TEXT_BUFFER(buffer), "modified-changed", G_CALLBACK(on_buffer_modified),
                     tab);

    /* Create scrolled window */
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), tab->source_view);

    /* Create tab label with close button */
    const char* label_text = filepath ? strrchr(filepath, '/') : NULL;
    label_text = label_text ? label_text + 1 : (filepath ? filepath : "Untitled");

    tab->tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    tab->tab_label = gtk_label_new(label_text);
    gtk_box_pack_start(GTK_BOX(tab->tab_box), tab->tab_label, TRUE, TRUE, 0);

    GtkWidget* close_btn = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);

    TabCloseData* tcd = (TabCloseData*)malloc(sizeof(TabCloseData));
    tcd->editor = ed;
    tcd->tab_index = idx;
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_tab_close_clicked), tcd);

    gtk_box_pack_start(GTK_BOX(tab->tab_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab->tab_box);

    /* Add to notebook */
    gtk_notebook_append_page(GTK_NOTEBOOK(ed->notebook), scrolled, tab->tab_box);
    gtk_widget_show_all(scrolled);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(ed->notebook), idx);

    ed->tab_count++;
    return idx;
}

bool xs_editor_open_file(XsEditor* ed, const char* filepath) {
    if (!ed || !filepath)
        return false;

    /* Check if already open */
    for (int i = 0; i < ed->tab_count; i++) {
        if (strcmp(ed->tabs[i].file_path, filepath) == 0) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(ed->notebook), i);
            return true;
        }
    }

    /* Read file content */
    FILE* f = fopen(filepath, "r");
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* content = (char*)malloc((size_t)size + 1);
    if (!content) {
        fclose(f);
        return false;
    }
    size_t nread = fread(content, 1, (size_t)size, f);
    content[nread] = '\0';
    fclose(f);

    /* Create new tab */
    int idx = xs_editor_new_tab(ed, filepath);
    if (idx < 0) {
        free(content);
        return false;
    }

    /* Set content */
    XsEditorTab* tab = &ed->tabs[idx];
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(tab->buffer), content, -1);
    gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(tab->buffer), FALSE);
    tab->modified = false;

    free(content);
    return true;
}

bool xs_editor_save_current(XsEditor* ed) {
    if (!ed)
        return false;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page < 0 || page >= ed->tab_count)
        return false;

    XsEditorTab* tab = &ed->tabs[page];
    if (!tab->file_path[0])
        return false;

    char* text = xs_editor_get_current_text(ed);
    if (!text)
        return false;

    FILE* f = fopen(tab->file_path, "w");
    if (!f) {
        free(text);
        return false;
    }
    fputs(text, f);
    fclose(f);
    free(text);

    gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(tab->buffer), FALSE);
    tab->modified = false;

    /* Update tab label (remove asterisk) */
    const char* basename = strrchr(tab->file_path, '/');
    basename = basename ? basename + 1 : tab->file_path;
    gtk_label_set_text(GTK_LABEL(tab->tab_label), basename);

    return true;
}

bool xs_editor_save_as(XsEditor* ed, const char* filepath) {
    if (!ed || !filepath)
        return false;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page < 0 || page >= ed->tab_count)
        return false;

    XsEditorTab* tab = &ed->tabs[page];
    strncpy(tab->file_path, filepath, sizeof(tab->file_path) - 1);

    return xs_editor_save_current(ed);
}

void xs_editor_close_current(XsEditor* ed) {
    if (!ed)
        return;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page >= 0)
        xs_editor_close_tab(ed, page);
}

void xs_editor_close_tab(XsEditor* ed, int index) {
    if (!ed || index < 0 || index >= ed->tab_count)
        return;

    gtk_notebook_remove_page(GTK_NOTEBOOK(ed->notebook), index);

    /* Shift tabs down */
    for (int i = index; i < ed->tab_count - 1; i++) {
        ed->tabs[i] = ed->tabs[i + 1];
    }
    ed->tab_count--;

    /* Ensure at least one tab exists */
    if (ed->tab_count == 0) {
        xs_editor_new_tab(ed, NULL);
    }
}

const char* xs_editor_get_current_path(XsEditor* ed) {
    if (!ed)
        return NULL;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page < 0 || page >= ed->tab_count)
        return NULL;
    return ed->tabs[page].file_path;
}

char* xs_editor_get_current_text(XsEditor* ed) {
    if (!ed)
        return NULL;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page < 0 || page >= ed->tab_count)
        return NULL;

    GtkTextIter start, end;
    GtkTextBuffer* buf = GTK_TEXT_BUFFER(ed->tabs[page].buffer);
    gtk_text_buffer_get_bounds(buf, &start, &end);
    return gtk_text_buffer_get_text(buf, &start, &end, FALSE);
}

/* ===== Editing Actions ===== */
void xs_editor_undo(XsEditor* ed) {
    if (!ed)
        return;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page < 0 || page >= ed->tab_count)
        return;
    if (gtk_source_buffer_can_undo(ed->tabs[page].buffer))
        gtk_source_buffer_undo(ed->tabs[page].buffer);
}

void xs_editor_redo(XsEditor* ed) {
    if (!ed)
        return;
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page < 0 || page >= ed->tab_count)
        return;
    if (gtk_source_buffer_can_redo(ed->tabs[page].buffer))
        gtk_source_buffer_redo(ed->tabs[page].buffer);
}

void xs_editor_show_find(XsEditor* ed) {
    /* In a full implementation, show a search bar overlay.
     * For now, use a simple dialog. */
    (void)ed;
}

void xs_editor_get_cursor(XsEditor* ed, int* line, int* col) {
    if (!ed || !line || !col)
        return;
    *line = 1;
    *col = 1;

    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(ed->notebook));
    if (page < 0 || page >= ed->tab_count)
        return;

    GtkTextBuffer* buf = GTK_TEXT_BUFFER(ed->tabs[page].buffer);
    GtkTextMark* mark = gtk_text_buffer_get_insert(buf);
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buf, &iter, mark);

    *line = gtk_text_iter_get_line(&iter) + 1;
    *col = gtk_text_iter_get_line_offset(&iter) + 1;
}
