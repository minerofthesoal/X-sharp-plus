/*
 * X# IDE (Qt6) - X# Syntax Highlighter
 * =======================================
 * QSyntaxHighlighter subclass providing full syntax highlighting
 * for the X# programming language.
 */

#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class XSharpHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit XSharpHighlighter(QTextDocument *parent = nullptr);

    /* Update colors when theme changes */
    void applyDarkTheme();
    void applyLightTheme();

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    QVector<HighlightingRule> m_rules;

    /* Multi-line block comment state */
    QRegularExpression m_blockCommentStart;
    QRegularExpression m_blockCommentEnd;

    /* Formats */
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeKeywordFormat;
    QTextCharFormat m_builtinFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_preprocessorFormat;

    void setupFormats();
    void setupRules();
};
