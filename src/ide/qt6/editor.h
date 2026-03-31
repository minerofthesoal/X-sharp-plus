/*
 * X# IDE (Qt6) - Code Editor Component
 * ======================================
 * QPlainTextEdit-based editor with:
 *   - Line-number gutter
 *   - X# syntax highlighting via XSharpHighlighter
 *   - Multi-tab support (QTabWidget)
 *   - Auto-indent, bracket matching, current-line highlighting
 *   - Find/Replace dialog
 */

#pragma once

#include <QAction>
#include <QColor>
#include <QPlainTextEdit>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

class XSharpHighlighter;
class LineNumberArea;

/* ===================================================================
 * XsCodeEditor
 * A single QPlainTextEdit pane with a line-number gutter and
 * X# syntax highlighting.
 * =================================================================== */
class XsCodeEditor : public QPlainTextEdit {
    Q_OBJECT

  public:
    explicit XsCodeEditor(QWidget* parent = nullptr);

    /* Called by LineNumberArea to paint itself */
    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;

    /* Settings */
    void setTabWidth(int spaces);
    void setAutoIndent(bool enabled);
    void setBracketMatching(bool enabled);
    void setHighlightCurrentLine(bool enabled);
    void applyObsidianTheme();
    void applyRadianceTheme();

  signals:
    void cursorPositionChanged(int line, int col);

  protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

  private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);
    void onCursorPositionChanged();

  private:
    QWidget* m_lineNumberArea;
    XSharpHighlighter* m_highlighter;

    bool m_autoIndent = true;
    bool m_bracketMatching = true;
    bool m_highlightLine = true;
    int m_tabWidth = 4;
};

/* ===================================================================
 * LineNumberArea
 * A thin widget painted alongside XsCodeEditor.
 * =================================================================== */
class LineNumberArea : public QWidget {
    Q_OBJECT

  public:
    explicit LineNumberArea(XsCodeEditor* editor);

    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    XsCodeEditor* m_codeEditor;
};

/* ===================================================================
 * EditorTab
 * Metadata associated with each open file tab.
 * =================================================================== */
struct EditorTab {
    XsCodeEditor* editor = nullptr;
    QString filePath; /* empty = untitled */
    bool modified = false;
};

/* ===================================================================
 * XsEditor
 * Top-level editor widget containing a QTabWidget of EditorTabs.
 * =================================================================== */
class XsEditor : public QWidget {
    Q_OBJECT

  public:
    explicit XsEditor(QWidget* parent = nullptr);

    /* Tab management */
    int newTab(const QString& filePath = QString());
    bool openFile(const QString& filePath);
    bool saveCurrent();
    bool saveAs(const QString& filePath);
    void closeCurrent();
    void closeTab(int index);

    /* Accessors */
    QString currentFilePath() const;
    QString currentText() const;
    QTabWidget* tabWidget() const {
        return m_tabs;
    }

    /* Editing actions */
    void undo();
    void redo();
    void showFindDialog();

    /* Apply dark/light theme to all child editors */
    void applyObsidianTheme();
    void applyRadianceTheme();

    /* Settings forwarding */
    void setTabWidth(int w);
    void setAutoIndent(bool b);
    void setBracketMatching(bool b);
    void setHighlightCurrentLine(bool b);

  signals:
    void cursorMoved(int line, int col);
    void fileChanged(const QString& path);
    void modificationChanged(bool modified);

  private slots:
    void onTabCloseRequested(int index);
    void onTabChanged(int index);
    void onEditorModified();
    void onCursorMoved(int line, int col);

  private:
    QTabWidget* m_tabs;
    QVector<EditorTab> m_tabData;

    int tabWidth = 4;
    bool autoIndent = true;
    bool bracketMatching = true;
    bool highlightCurrentLine = true;

    XsCodeEditor* currentEditor() const;
    EditorTab* currentTabData();
    void updateTabTitle(int index);
    bool saveFile(int index, const QString& path);
    QString readFile(const QString& path);
    bool writeFile(const QString& path, const QString& text);
};
