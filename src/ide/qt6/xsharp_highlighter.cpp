/*
 * X# IDE (Qt6) - X# Syntax Highlighter Implementation
 * =====================================================
 * Provides syntax highlighting for the X# programming language using
 * QSyntaxHighlighter with regular-expression-based rules.
 *
 * X# Keywords reference:
 *   Control flow : forge, morph, oracle, otherwise, while, patrol, shift
 *   Declarations : entity, quest, engrave, summon, banish
 *   Literals     : truth, lies, abyss
 *   Types        : fate, blade, spark, scroll, arsenal, tome, rune
 *   Special      : self, yeet, rescue, shield
 */

#include "xsharp_highlighter.h"

/* ===== Construction ===== */

XSharpHighlighter::XSharpHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupFormats();
    setupRules();
}

/* ===== Dark Theme (default - Obsidian) ===== */

void XSharpHighlighter::applyDarkTheme()
{
    /* Control-flow keywords – vivid purple/violet (VS Code-style) */
    m_keywordFormat.setForeground(QColor(0xC5, 0x86, 0xC0)); /* #C586C0 */
    m_keywordFormat.setFontWeight(QFont::Bold);

    /* Type keywords – cyan/teal */
    m_typeKeywordFormat.setForeground(QColor(0x4E, 0xC9, 0xB0)); /* #4EC9B0 */
    m_typeKeywordFormat.setFontWeight(QFont::Bold);

    /* Built-in / special keywords – bright blue */
    m_builtinFormat.setForeground(QColor(0x56, 0x9C, 0xD6)); /* #569CD6 */
    m_builtinFormat.setFontWeight(QFont::Bold);

    /* Function/method names – yellow */
    m_functionFormat.setForeground(QColor(0xDC, 0xDC, 0xAA)); /* #DCDCAA */

    /* String literals – orange */
    m_stringFormat.setForeground(QColor(0xCE, 0x91, 0x78)); /* #CE9178 */

    /* Numeric literals – light green */
    m_numberFormat.setForeground(QColor(0xB5, 0xCE, 0xA8)); /* #B5CEA8 */

    /* Comments – olive/grey-green */
    m_commentFormat.setForeground(QColor(0x6A, 0x99, 0x55)); /* #6A9955 */
    m_commentFormat.setFontItalic(true);

    /* Operators – white/light-grey */
    m_operatorFormat.setForeground(QColor(0xD4, 0xD4, 0xD4)); /* #D4D4D4 */

    /* Preprocessor / pragmas – grey */
    m_preprocessorFormat.setForeground(QColor(0x9B, 0x9B, 0x9B)); /* #9B9B9B */

    setupRules();
    rehighlight();
}

/* ===== Light Theme (Radiance) ===== */

void XSharpHighlighter::applyLightTheme()
{
    m_keywordFormat.setForeground(QColor(0xAF, 0x00, 0xDB)); /* #AF00DB */
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_typeKeywordFormat.setForeground(QColor(0x26, 0x7F, 0x99)); /* #267F99 */
    m_typeKeywordFormat.setFontWeight(QFont::Bold);

    m_builtinFormat.setForeground(QColor(0x00, 0x00, 0xFF)); /* #0000FF */
    m_builtinFormat.setFontWeight(QFont::Bold);

    m_functionFormat.setForeground(QColor(0x79, 0x5E, 0x26)); /* #795E26 */

    m_stringFormat.setForeground(QColor(0xA3, 0x15, 0x15)); /* #A31515 */

    m_numberFormat.setForeground(QColor(0x09, 0x88, 0x58)); /* #098858 */

    m_commentFormat.setForeground(QColor(0x00, 0x80, 0x00)); /* #008000 */
    m_commentFormat.setFontItalic(true);

    m_operatorFormat.setForeground(QColor(0x00, 0x00, 0x00)); /* #000000 */

    m_preprocessorFormat.setForeground(QColor(0x60, 0x60, 0x60)); /* #606060 */

    setupRules();
    rehighlight();
}

/* ===== Format Initialisation ===== */

void XSharpHighlighter::setupFormats()
{
    applyDarkTheme();
}

/* ===== Rule Setup ===== */

void XSharpHighlighter::setupRules()
{
    m_rules.clear();
    HighlightingRule rule;

    /* -----------------------------------------------------------------
     * 1. X# CONTROL-FLOW KEYWORDS
     * ----------------------------------------------------------------- */
    const QStringList controlKeywords = {
        QStringLiteral("forge"),
        QStringLiteral("morph"),
        QStringLiteral("oracle"),
        QStringLiteral("otherwise"),
        QStringLiteral("while"),
        QStringLiteral("patrol"),
        QStringLiteral("shift"),
        QStringLiteral("yeet"),
        QStringLiteral("rescue"),
        QStringLiteral("shield"),
    };

    for (const QString &kw : controlKeywords) {
        rule.pattern = QRegularExpression(QStringLiteral("\\b") + kw + QStringLiteral("\\b"));
        rule.format  = m_keywordFormat;
        m_rules.append(rule);
    }

    /* -----------------------------------------------------------------
     * 2. X# TYPE KEYWORDS
     * ----------------------------------------------------------------- */
    const QStringList typeKeywords = {
        QStringLiteral("fate"),
        QStringLiteral("blade"),
        QStringLiteral("spark"),
        QStringLiteral("scroll"),
        QStringLiteral("arsenal"),
        QStringLiteral("tome"),
        QStringLiteral("rune"),
        QStringLiteral("truth"),
        QStringLiteral("lies"),
        QStringLiteral("abyss"),
    };

    for (const QString &kw : typeKeywords) {
        rule.pattern = QRegularExpression(QStringLiteral("\\b") + kw + QStringLiteral("\\b"));
        rule.format  = m_typeKeywordFormat;
        m_rules.append(rule);
    }

    /* -----------------------------------------------------------------
     * 3. X# DECLARATION / BUILT-IN KEYWORDS
     * ----------------------------------------------------------------- */
    const QStringList builtinKeywords = {
        QStringLiteral("entity"),
        QStringLiteral("quest"),
        QStringLiteral("engrave"),
        QStringLiteral("summon"),
        QStringLiteral("banish"),
        QStringLiteral("self"),
    };

    for (const QString &kw : builtinKeywords) {
        rule.pattern = QRegularExpression(QStringLiteral("\\b") + kw + QStringLiteral("\\b"));
        rule.format  = m_builtinFormat;
        m_rules.append(rule);
    }

    /* -----------------------------------------------------------------
     * 4. FUNCTION / QUEST CALLS  –  identifier followed by '('
     * ----------------------------------------------------------------- */
    rule.pattern = QRegularExpression(QStringLiteral("\\b([A-Za-z_][A-Za-z0-9_]*)(?=\\s*\\()"));
    rule.format  = m_functionFormat;
    m_rules.append(rule);

    /* -----------------------------------------------------------------
     * 5. NUMERIC LITERALS
     *    Hex, binary, float, integer
     * ----------------------------------------------------------------- */
    rule.pattern = QRegularExpression(
        QStringLiteral("\\b(0x[0-9A-Fa-f]+|0b[01]+|\\d+\\.\\d*([eE][+-]?\\d+)?|\\d+)\\b"));
    rule.format  = m_numberFormat;
    m_rules.append(rule);

    /* -----------------------------------------------------------------
     * 6. OPERATORS
     * ----------------------------------------------------------------- */
    rule.pattern = QRegularExpression(
        QStringLiteral("[+\\-*/%&|^~<>=!?:]+"));
    rule.format  = m_operatorFormat;
    m_rules.append(rule);

    /* -----------------------------------------------------------------
     * 7. STRING LITERALS  (double-quoted, supports \" escape)
     * ----------------------------------------------------------------- */
    rule.pattern = QRegularExpression(QStringLiteral("\"(?:[^\"\\\\]|\\\\.)*\""));
    rule.format  = m_stringFormat;
    m_rules.append(rule);

    /* -----------------------------------------------------------------
     * 8. CHARACTER LITERALS  (single-quoted)
     * ----------------------------------------------------------------- */
    rule.pattern = QRegularExpression(QStringLiteral("'(?:[^'\\\\]|\\\\.)'"));
    rule.format  = m_stringFormat;
    m_rules.append(rule);

    /* -----------------------------------------------------------------
     * 9. SINGLE-LINE COMMENT  ( // ... )
     * ----------------------------------------------------------------- */
    rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
    rule.format  = m_commentFormat;
    m_rules.append(rule);

    /* Block comment delimiters stored separately for multi-line handling */
    m_blockCommentStart = QRegularExpression(QStringLiteral("/\\*"));
    m_blockCommentEnd   = QRegularExpression(QStringLiteral("\\*/"));
}

/* ===== highlightBlock ===== */

void XSharpHighlighter::highlightBlock(const QString &text)
{
    /* Apply all single-line rules first */
    for (const HighlightingRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    /* -----------------------------------------------------------------------
     * Multi-line block comment handling
     * State 0 = normal, State 1 = inside block comment
     * ---------------------------------------------------------------------- */
    setCurrentBlockState(0);

    int searchStart = 0;

    if (previousBlockState() != 1) {
        /* Not inside a block comment – look for opening /* */
        QRegularExpressionMatch startMatch = m_blockCommentStart.match(text, searchStart);
        if (!startMatch.hasMatch()) {
            return; /* No block comment on this line */
        }
        searchStart = startMatch.capturedStart();
        /* Fall through: we found a start delimiter */
    }

    /* We are (or just entered) a block comment */
    while (searchStart >= 0) {
        QRegularExpressionMatch endMatch = m_blockCommentEnd.match(text, searchStart);
        int commentLength;

        if (endMatch.hasMatch()) {
            /* Block comment closes on this line */
            commentLength = endMatch.capturedEnd() - searchStart;
            setFormat(searchStart, commentLength, m_commentFormat);
            setCurrentBlockState(0);

            /* Look for another opening after this close */
            QRegularExpressionMatch nextStart = m_blockCommentStart.match(
                text, endMatch.capturedEnd());
            if (nextStart.hasMatch()) {
                searchStart = nextStart.capturedStart();
            } else {
                break; /* No further block comments on this line */
            }
        } else {
            /* Block comment continues to next line */
            commentLength = text.length() - searchStart;
            setFormat(searchStart, commentLength, m_commentFormat);
            setCurrentBlockState(1);
            break;
        }
    }
}
