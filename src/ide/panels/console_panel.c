/*
 * X# IDE - Console Panel Implementation
 * ========================================
 * GtkTextView console output with colored text tags.
 */

#include "console_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

XsConsolePanel *xs_console_panel_new(void) {
    XsConsolePanel *panel = (XsConsolePanel *)calloc(1, sizeof(XsConsolePanel));
    if (!panel) return NULL;

    panel->buffer = gtk_text_buffer_new(NULL);
    panel->text_view = gtk_text_view_new_with_buffer(panel->buffer);

    /* Make read-only */
    gtk_text_view_set_editable(GTK_TEXT_VIEW(panel->text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(panel->text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(panel->text_view), GTK_WRAP_WORD_CHAR);

    /* Set monospace font */
    PangoFontDescription *font = pango_font_description_from_string("Monospace 10");
    gtk_widget_override_font(panel->text_view, font);
    pango_font_description_free(font);

    /* Set dark background */
    GdkRGBA bg = { 0.12, 0.12, 0.14, 1.0 };
    GdkRGBA fg = { 0.85, 0.85, 0.85, 1.0 };
    gtk_widget_override_background_color(panel->text_view, GTK_STATE_FLAG_NORMAL, &bg);
    gtk_widget_override_color(panel->text_view, GTK_STATE_FLAG_NORMAL, &fg);

    /* Left padding */
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(panel->text_view), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(panel->text_view), 4);

    /* Create color tags */
    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(panel->buffer);

    panel->tag_info = gtk_text_tag_new("info");
    g_object_set(panel->tag_info, "foreground", "#78DCE8", NULL);  /* cyan */
    gtk_text_tag_table_add(tag_table, panel->tag_info);

    panel->tag_error = gtk_text_tag_new("error");
    g_object_set(panel->tag_error, "foreground", "#FF6188", NULL);  /* red */
    gtk_text_tag_table_add(tag_table, panel->tag_error);

    panel->tag_success = gtk_text_tag_new("success");
    g_object_set(panel->tag_success, "foreground", "#A9DC76", NULL); /* green */
    gtk_text_tag_table_add(tag_table, panel->tag_success);

    panel->tag_output = gtk_text_tag_new("output");
    g_object_set(panel->tag_output, "foreground", "#D4D4D4", NULL); /* light grey */
    gtk_text_tag_table_add(tag_table, panel->tag_output);

    panel->tag_warning = gtk_text_tag_new("warning");
    g_object_set(panel->tag_warning, "foreground", "#FFD866", NULL); /* yellow */
    gtk_text_tag_table_add(tag_table, panel->tag_warning);

    /* Scrolled window */
    panel->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(panel->scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(panel->scrolled_window), 120);
    gtk_container_add(GTK_CONTAINER(panel->scrolled_window), panel->text_view);

    return panel;
}

void xs_console_panel_free(XsConsolePanel *panel) {
    if (!panel) return;
    /* Tags are owned by the tag table; widgets are owned by GTK */
    free(panel);
}

GtkWidget *xs_console_panel_get_widget(XsConsolePanel *panel) {
    if (!panel) return NULL;
    return panel->scrolled_window;
}

void xs_console_panel_append(XsConsolePanel *panel, const char *text, const char *tag_name) {
    if (!panel || !text) return;

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(panel->buffer, &end);

    if (tag_name) {
        gtk_text_buffer_insert_with_tags_by_name(panel->buffer, &end, text, -1, tag_name, NULL);
    } else {
        gtk_text_buffer_insert(panel->buffer, &end, text, -1);
    }

    /* Auto-scroll to bottom */
    gtk_text_buffer_get_end_iter(panel->buffer, &end);
    GtkTextMark *mark = gtk_text_buffer_create_mark(panel->buffer, NULL, &end, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(panel->text_view), mark, 0.0, FALSE, 0.0, 0.0);
    gtk_text_buffer_delete_mark(panel->buffer, mark);
}

void xs_console_panel_clear(XsConsolePanel *panel) {
    if (!panel) return;
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(panel->buffer, &start, &end);
    gtk_text_buffer_delete(panel->buffer, &start, &end);
}
