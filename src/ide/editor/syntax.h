/*
 * X# IDE - Syntax Highlighting
 * ==============================
 * Register X# language with GtkSourceLanguageManager.
 */

#ifndef XS_SYNTAX_H
#define XS_SYNTAX_H

#include <gtksourceview/gtksource.h>

/* Initialize X# language definition and register it */
void xs_syntax_init(GtkSourceLanguageManager *manager);

/* Get the X# language object (after init) */
GtkSourceLanguage *xs_syntax_get_language(GtkSourceLanguageManager *manager);

#endif /* XS_SYNTAX_H */
