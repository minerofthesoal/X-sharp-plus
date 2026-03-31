/*
 * X# IDE - Syntax Highlighting Implementation
 * ==============================================
 * Registers the X# language with GtkSourceView via an embedded XML spec.
 */

#include "syntax.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Embedded X# language definition in GtkSourceView XML format */
static const char* XS_LANG_XML =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<language id=\"xsharp\" name=\"X#\" version=\"2.0\" _section=\"Source\">\n"
    "  <metadata>\n"
    "    <property name=\"mimetypes\">text/x-xsharp</property>\n"
    "    <property name=\"globs\">*.xs</property>\n"
    "    <property name=\"line-comment-start\">//</property>\n"
    "    <property name=\"block-comment-start\">/*</property>\n"
    "    <property name=\"block-comment-end\">*/</property>\n"
    "  </metadata>\n"
    "\n"
    "  <styles>\n"
    "    <style id=\"comment\"         name=\"Comment\"         map-to=\"def:comment\"/>\n"
    "    <style id=\"string\"          name=\"String\"          map-to=\"def:string\"/>\n"
    "    <style id=\"char\"            name=\"Character\"       map-to=\"def:character\"/>\n"
    "    <style id=\"keyword\"         name=\"Keyword\"         map-to=\"def:keyword\"/>\n"
    "    <style id=\"type\"            name=\"Type\"            map-to=\"def:type\"/>\n"
    "    <style id=\"boolean\"         name=\"Boolean\"         map-to=\"def:boolean\"/>\n"
    "    <style id=\"null\"            name=\"Null\"            map-to=\"def:special-constant\"/>\n"
    "    <style id=\"number\"          name=\"Number\"          map-to=\"def:number\"/>\n"
    "    <style id=\"operator\"        name=\"Operator\"        map-to=\"def:operator\"/>\n"
    "    <style id=\"builtin\"         name=\"Builtin\"         map-to=\"def:builtin\"/>\n"
    "    <style id=\"special-block\"   name=\"Special Block\"   map-to=\"def:preprocessor\"/>\n"
    "    <style id=\"interpolation\"   name=\"Interpolation\"   map-to=\"def:special-char\"/>\n"
    "  </styles>\n"
    "\n"
    "  <definitions>\n"
    "    <!-- Line comment -->\n"
    "    <context id=\"line-comment\" style-ref=\"comment\" end-at-line-end=\"true\" "
    "class=\"comment\">\n"
    "      <start>//</start>\n"
    "    </context>\n"
    "\n"
    "    <!-- Block comment -->\n"
    "    <context id=\"block-comment\" style-ref=\"comment\" class=\"comment\">\n"
    "      <start>/\\*</start>\n"
    "      <end>\\*/</end>\n"
    "    </context>\n"
    "\n"
    "    <!-- String with interpolation -->\n"
    "    <context id=\"string\" style-ref=\"string\" end-at-line-end=\"true\" class=\"string\">\n"
    "      <start>\"</start>\n"
    "      <end>\"</end>\n"
    "      <include>\n"
    "        <context id=\"interpolation\" style-ref=\"interpolation\">\n"
    "          <start>#\\{</start>\n"
    "          <end>\\}</end>\n"
    "        </context>\n"
    "        <context id=\"escape\" style-ref=\"def:special-char\">\n"
    "          <match>\\\\[\\\\\"nrt0]</match>\n"
    "        </context>\n"
    "      </include>\n"
    "    </context>\n"
    "\n"
    "    <!-- Character literal -->\n"
    "    <context id=\"char\" style-ref=\"char\" end-at-line-end=\"true\" class=\"string\">\n"
    "      <start>'</start>\n"
    "      <end>'</end>\n"
    "    </context>\n"
    "\n"
    "    <!-- Numbers -->\n"
    "    <context id=\"hex-number\" style-ref=\"number\">\n"
    "      <match>\\b0[xX][0-9a-fA-F_]+\\b</match>\n"
    "    </context>\n"
    "    <context id=\"bin-number\" style-ref=\"number\">\n"
    "      <match>\\b0[bB][01_]+\\b</match>\n"
    "    </context>\n"
    "    <context id=\"oct-number\" style-ref=\"number\">\n"
    "      <match>\\b0[oO][0-7_]+\\b</match>\n"
    "    </context>\n"
    "    <context id=\"float-number\" style-ref=\"number\">\n"
    "      <match>\\b[0-9][0-9_]*\\.[0-9_]*([eE][+-]?[0-9_]+)?\\b</match>\n"
    "    </context>\n"
    "    <context id=\"dec-number\" style-ref=\"number\">\n"
    "      <match>\\b[0-9][0-9_]*\\b</match>\n"
    "    </context>\n"
    "\n"
    "    <!-- Declaration keywords -->\n"
    "    <context id=\"declaration-keywords\" style-ref=\"keyword\">\n"
    "      <keyword>forge</keyword>\n"
    "      <keyword>entity</keyword>\n"
    "      <keyword>realm</keyword>\n"
    "      <keyword>quest</keyword>\n"
    "      <keyword>summon</keyword>\n"
    "      <keyword>eternal</keyword>\n"
    "      <keyword>morph</keyword>\n"
    "    </context>\n"
    "\n"
    "    <!-- Control flow keywords -->\n"
    "    <context id=\"control-keywords\" style-ref=\"keyword\">\n"
    "      <keyword>oracle</keyword>\n"
    "      <keyword>otherwise</keyword>\n"
    "      <keyword>cycle</keyword>\n"
    "      <keyword>while</keyword>\n"
    "      <keyword>unleash</keyword>\n"
    "      <keyword>shatter_cycle</keyword>\n"
    "      <keyword>skip</keyword>\n"
    "    </context>\n"
    "\n"
    "    <!-- OOP and misc keywords -->\n"
    "    <context id=\"oop-keywords\" style-ref=\"keyword\">\n"
    "      <keyword>conjure</keyword>\n"
    "      <keyword>self</keyword>\n"
    "      <keyword>spell</keyword>\n"
    "      <keyword>shield</keyword>\n"
    "      <keyword>deflect</keyword>\n"
    "      <keyword>shatter</keyword>\n"
    "      <keyword>extends</keyword>\n"
    "      <keyword>implements</keyword>\n"
    "      <keyword>engrave</keyword>\n"
    "      <keyword>as</keyword>\n"
    "      <keyword>in</keyword>\n"
    "      <keyword>from</keyword>\n"
    "    </context>\n"
    "\n"
    "    <!-- Types -->\n"
    "    <context id=\"types\" style-ref=\"type\">\n"
    "      <keyword>blade</keyword>\n"
    "      <keyword>spark</keyword>\n"
    "      <keyword>scroll</keyword>\n"
    "      <keyword>rune</keyword>\n"
    "      <keyword>fate</keyword>\n"
    "      <keyword>void</keyword>\n"
    "      <keyword>arsenal</keyword>\n"
    "    </context>\n"
    "\n"
    "    <!-- Boolean -->\n"
    "    <context id=\"boolean\" style-ref=\"boolean\">\n"
    "      <keyword>truth</keyword>\n"
    "      <keyword>lies</keyword>\n"
    "    </context>\n"
    "\n"
    "    <!-- Null -->\n"
    "    <context id=\"null\" style-ref=\"null\">\n"
    "      <keyword>abyss</keyword>\n"
    "    </context>\n"
    "\n"
    "    <!-- Special blocks -->\n"
    "    <context id=\"special-blocks\" style-ref=\"special-block\">\n"
    "      <match>@(gpu|ai)\\b</match>\n"
    "    </context>\n"
    "\n"
    "    <!-- Operators -->\n"
    "    <context id=\"operators\" style-ref=\"operator\">\n"
    "      "
    "<match>[+\\-*/"
    "%=!&lt;&gt;&amp;|^~?:]+|\\|&gt;|-&gt;|=&gt;|\\.\\.\\.|\\.\\.|\\.|(\\+\\+|--)|(\\*\\*)</"
    "match>\n"
    "    </context>\n"
    "\n"
    "    <!-- Main context -->\n"
    "    <context id=\"xsharp\" class=\"no-spell-check\">\n"
    "      <include>\n"
    "        <context ref=\"line-comment\"/>\n"
    "        <context ref=\"block-comment\"/>\n"
    "        <context ref=\"string\"/>\n"
    "        <context ref=\"char\"/>\n"
    "        <context ref=\"hex-number\"/>\n"
    "        <context ref=\"bin-number\"/>\n"
    "        <context ref=\"oct-number\"/>\n"
    "        <context ref=\"float-number\"/>\n"
    "        <context ref=\"dec-number\"/>\n"
    "        <context ref=\"declaration-keywords\"/>\n"
    "        <context ref=\"control-keywords\"/>\n"
    "        <context ref=\"oop-keywords\"/>\n"
    "        <context ref=\"types\"/>\n"
    "        <context ref=\"boolean\"/>\n"
    "        <context ref=\"null\"/>\n"
    "        <context ref=\"special-blocks\"/>\n"
    "        <context ref=\"operators\"/>\n"
    "      </include>\n"
    "    </context>\n"
    "  </definitions>\n"
    "</language>\n";

/* Write the embedded XML to a temp file and add its directory to the search path */
void xs_syntax_init(GtkSourceLanguageManager* manager) {
    if (!manager)
        return;

    /* Write language definition to a config directory */
    const char* home = getenv("HOME");
    if (!home)
        home = "/tmp";

    char dir[1024];
    snprintf(dir, sizeof(dir), "%s/.local/share/gtksourceview-3.0/language-specs", home);

    /* Create directories recursively */
    char mkdir_cmd[1100];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p '%s'", dir);
    (void)system(mkdir_cmd);

    /* Write the .lang file */
    char path[1100];
    snprintf(path, sizeof(path), "%s/xsharp.lang", dir);

    FILE* f = fopen(path, "w");
    if (f) {
        fputs(XS_LANG_XML, f);
        fclose(f);
    }

    /* Add the directory to the language manager search path */
    const gchar* const* current_paths = gtk_source_language_manager_get_search_path(manager);
    int count = 0;
    if (current_paths) {
        while (current_paths[count])
            count++;
    }

    const gchar** new_paths = (const gchar**)g_malloc0(sizeof(gchar*) * (count + 2));
    new_paths[0] = dir;
    for (int i = 0; i < count; i++) {
        new_paths[i + 1] = current_paths[i];
    }
    new_paths[count + 1] = NULL;

    gtk_source_language_manager_set_search_path(manager, (gchar**)new_paths);
    g_free(new_paths);
}

GtkSourceLanguage* xs_syntax_get_language(GtkSourceLanguageManager* manager) {
    if (!manager)
        return NULL;
    return gtk_source_language_manager_get_language(manager, "xsharp");
}
