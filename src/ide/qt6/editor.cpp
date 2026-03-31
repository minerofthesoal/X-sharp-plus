/*
 * X# IDE (Qt6) - Code Editor Implementation
 * ===========================================
 * Implements XsCodeEditor (single pane), LineNumberArea, and XsEditor
 * (tab manager).
 */

#include "editor.h"
#include "xsharp_highlighter.h"

#include <QPainter>
#include <QTextBlock>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFontDialog>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QApplication>

/* =========================================================================
 * LineNumberArea
 * ========================================================================= */

LineNumberArea::LineNumberArea(XsCodeEditor *editor)
    : QWidget(editor), m_codeEditor(editor)
{}

QSize LineNumberArea::sizeHint() const
{
    return QSize(m_codeEditor->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_codeEditor->lineNumberAreaPaintEvent(event);
}

/* =========================================================================
 * XsCodeEditor
 * ========================================================================= */

XsCodeEditor::XsCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);
    m_highlighter    = new XSharpHighlighter(document());

    /* Font */
    QFont font(QStringLiteral("JetBrains Mono"));
    if (!font.exactMatch()) {
        font.setFamily(QStringLiteral("Fira Code"));
    }
    if (!font.exactMatch()) {
        font.setFamily(QStringLiteral("Consolas"));
    }
    if (!font.exactMatch()) {
        font.setFamily(QStringLiteral("Monospace"));
    }
    font.setPointSize(12);
    font.setFixedPitch(true);
    setFont(font);

    /* Tab stop */
    const int tabWidth = 4;
    QFontMetrics metrics(font);
    setTabStopDistance(tabWidth * metrics.horizontalAdvance(QLatin1Char(' ')));

    /* Connections */
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &XsCodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &XsCodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &XsCodeEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &XsCodeEditor::onCursorPositionChanged);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    applyObsidianTheme();
}

/* ----- Obsidian (dark) theme ----- */
void XsCodeEditor::applyObsidianTheme()
{
    setStyleSheet(
        QStringLiteral(
            "XsCodeEditor {"
            "  background-color: #1E1E1E;"
            "  color: #D4D4D4;"
            "  selection-background-color: #264F78;"
            "  selection-color: #FFFFFF;"
            "  border: none;"
            "}"
        )
    );
    m_highlighter->applyDarkTheme();
}

/* ----- Radiance (light) theme ----- */
void XsCodeEditor::applyRadianceTheme()
{
    setStyleSheet(
        QStringLiteral(
            "XsCodeEditor {"
            "  background-color: #FFFFFF;"
            "  color: #000000;"
            "  selection-background-color: #ADD6FF;"
            "  selection-color: #000000;"
            "  border: none;"
            "}"
        )
    );
    m_highlighter->applyLightTheme();
}

/* ----- Line number gutter width ----- */
int XsCodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 1);
}

void XsCodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void XsCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void XsCodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void XsCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0x25, 0x25, 0x25)); /* #252525 */

    QTextBlock block       = firstVisibleBlock();
    int        blockNumber = block.blockNumber();
    int        top         = qRound(blockBoundingGeometry(block)
                                   .translated(contentOffset()).top());
    int        bottom      = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(0x85, 0x85, 0x85)); /* #858585 */
            painter.drawText(
                0, top,
                m_lineNumberArea->width() - 4,
                fontMetrics().height(),
                Qt::AlignRight,
                number);
        }

        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void XsCodeEditor::highlightCurrentLine()
{
    if (!m_highlightLine) {
        setExtraSelections({});
        return;
    }

    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(0x2F, 0x2F, 0x2F)); /* #2F2F2F */
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        extraSelections.append(sel);
    }
    setExtraSelections(extraSelections);
}

void XsCodeEditor::onCursorPositionChanged()
{
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber() + 1;
    int col  = cursor.columnNumber() + 1;
    emit cursorPositionChanged(line, col);
}

/* ----- Key handling: auto-indent + bracket insertion ----- */
void XsCodeEditor::keyPressEvent(QKeyEvent *event)
{
    if (m_autoIndent && event->key() == Qt::Key_Return) {
        /* Copy indentation from the current line */
        QTextCursor cursor = textCursor();
        QString     line   = cursor.block().text();
        QString     indent;
        for (QChar c : line) {
            if (c == QLatin1Char(' ') || c == QLatin1Char('\t'))
                indent += c;
            else
                break;
        }
        /* Extra indent if line ends with '{' */
        if (line.trimmed().endsWith(QLatin1Char('{')))
            indent += QString(m_tabWidth, QLatin1Char(' '));

        QPlainTextEdit::keyPressEvent(event);
        insertPlainText(indent);
        return;
    }

    if (m_bracketMatching) {
        /* Auto-close brackets / quotes */
        static const QMap<QChar, QChar> pairs = {
            { QLatin1Char('('),  QLatin1Char(')') },
            { QLatin1Char('['),  QLatin1Char(']') },
            { QLatin1Char('{'),  QLatin1Char('}') },
            { QLatin1Char('"'),  QLatin1Char('"') },
        };

        if (event->text().length() == 1) {
            QChar ch = event->text().at(0);
            if (pairs.contains(ch)) {
                QPlainTextEdit::keyPressEvent(event);
                insertPlainText(QString(pairs[ch]));
                QTextCursor cur = textCursor();
                cur.movePosition(QTextCursor::PreviousCharacter);
                setTextCursor(cur);
                return;
            }
        }
    }

    QPlainTextEdit::keyPressEvent(event);
}

/* ----- Settings ----- */
void XsCodeEditor::setTabWidth(int spaces)
{
    m_tabWidth = spaces;
    QFontMetrics metrics(font());
    setTabStopDistance(spaces * metrics.horizontalAdvance(QLatin1Char(' ')));
}

void XsCodeEditor::setAutoIndent(bool enabled)        { m_autoIndent      = enabled; }
void XsCodeEditor::setBracketMatching(bool enabled)   { m_bracketMatching = enabled; }
void XsCodeEditor::setHighlightCurrentLine(bool enabled)
{
    m_highlightLine = enabled;
    highlightCurrentLine();
}

/* =========================================================================
 * XsEditor – tab manager
 * ========================================================================= */

XsEditor::XsEditor(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setStyleSheet(
        QStringLiteral(
            "QTabWidget::pane { border: none; }"
            "QTabBar::tab {"
            "  background: #2D2D2D;"
            "  color: #9D9D9D;"
            "  padding: 6px 16px;"
            "  border: none;"
            "  border-right: 1px solid #1E1E1E;"
            "}"
            "QTabBar::tab:selected {"
            "  background: #1E1E1E;"
            "  color: #FFFFFF;"
            "  border-top: 2px solid #007ACC;"
            "}"
            "QTabBar::tab:hover:!selected { background: #3C3C3C; }"
            "QTabBar::close-button {"
            "  image: url(none);"
            "  subcontrol-position: right;"
            "}"
        )
    );

    layout->addWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested,
            this, &XsEditor::onTabCloseRequested);
    connect(m_tabs, &QTabWidget::currentChanged,
            this, &XsEditor::onTabChanged);

    /* Open a blank tab by default */
    newTab();
}

/* ----- Create new tab ----- */
int XsEditor::newTab(const QString &filePath)
{
    EditorTab tab;
    tab.editor   = new XsCodeEditor(this);
    tab.filePath = filePath;
    tab.modified = false;

    tab.editor->setTabWidth(tabWidth);
    tab.editor->setAutoIndent(autoIndent);
    tab.editor->setBracketMatching(bracketMatching);
    tab.editor->setHighlightCurrentLine(highlightCurrentLine);

    connect(tab.editor->document(), &QTextDocument::modificationChanged,
            this, &XsEditor::onEditorModified);
    connect(tab.editor, &XsCodeEditor::cursorPositionChanged,
            this, &XsEditor::onCursorMoved);

    QString label = filePath.isEmpty()
                    ? QStringLiteral("Untitled")
                    : QFileInfo(filePath).fileName();

    int idx = m_tabs->addTab(tab.editor, label);
    m_tabData.append(tab);
    m_tabs->setCurrentIndex(idx);
    return idx;
}

/* ----- Open file ----- */
bool XsEditor::openFile(const QString &filePath)
{
    if (filePath.isEmpty()) return false;

    /* Check if already open */
    for (int i = 0; i < m_tabData.size(); ++i) {
        if (m_tabData[i].filePath == filePath) {
            m_tabs->setCurrentIndex(i);
            return true;
        }
    }

    QString text = readFile(filePath);
    if (text.isNull()) return false;

    int idx = newTab(filePath);
    XsCodeEditor *ed = m_tabData[idx].editor;
    ed->setPlainText(text);
    ed->document()->setModified(false);
    m_tabData[idx].modified = false;
    updateTabTitle(idx);

    emit fileChanged(filePath);
    return true;
}

/* ----- Save current tab ----- */
bool XsEditor::saveCurrent()
{
    EditorTab *tab = currentTabData();
    if (!tab) return false;
    if (tab->filePath.isEmpty()) return saveAs(QString());
    return saveFile(m_tabs->currentIndex(), tab->filePath);
}

/* ----- Save as ----- */
bool XsEditor::saveAs(const QString &filePath)
{
    QString path = filePath;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("Save File As"),
            QString(),
            QStringLiteral("X# Files (*.xs);;All Files (*)"));
    }
    if (path.isEmpty()) return false;

    int idx = m_tabs->currentIndex();
    if (!saveFile(idx, path)) return false;

    m_tabData[idx].filePath = path;
    updateTabTitle(idx);
    emit fileChanged(path);
    return true;
}

/* ----- Close current tab ----- */
void XsEditor::closeCurrent()
{
    closeTab(m_tabs->currentIndex());
}

void XsEditor::closeTab(int index)
{
    if (index < 0 || index >= m_tabData.size()) return;

    EditorTab &tab = m_tabData[index];
    if (tab.modified) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            this,
            QStringLiteral("Unsaved Changes"),
            QStringLiteral("Save changes to '%1'?")
                .arg(tab.filePath.isEmpty()
                     ? QStringLiteral("Untitled")
                     : QFileInfo(tab.filePath).fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (btn == QMessageBox::Cancel) return;
        if (btn == QMessageBox::Save)   saveCurrent();
    }

    m_tabs->removeTab(index);
    m_tabData.removeAt(index);

    /* Always keep at least one tab */
    if (m_tabData.isEmpty()) newTab();
}

/* ----- Accessors ----- */
QString XsEditor::currentFilePath() const
{
    int idx = m_tabs->currentIndex();
    if (idx < 0 || idx >= m_tabData.size()) return {};
    return m_tabData[idx].filePath;
}

QString XsEditor::currentText() const
{
    XsCodeEditor *ed = currentEditor();
    return ed ? ed->toPlainText() : QString();
}

/* ----- Edit actions ----- */
void XsEditor::undo() { if (auto *e = currentEditor()) e->undo(); }
void XsEditor::redo() { if (auto *e = currentEditor()) e->redo(); }

void XsEditor::showFindDialog()
{
    XsCodeEditor *ed = currentEditor();
    if (!ed) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Find"));
    dlg.setFixedWidth(420);

    QVBoxLayout *vl   = new QVBoxLayout(&dlg);
    QHBoxLayout *row1 = new QHBoxLayout;
    QHBoxLayout *row2 = new QHBoxLayout;

    QLineEdit *findEdit    = new QLineEdit(&dlg);
    QLineEdit *replaceEdit = new QLineEdit(&dlg);
    QCheckBox *caseCheck   = new QCheckBox(QStringLiteral("Match Case"), &dlg);
    QPushButton *btnFind   = new QPushButton(QStringLiteral("Find Next"),    &dlg);
    QPushButton *btnRepl   = new QPushButton(QStringLiteral("Replace"),      &dlg);
    QPushButton *btnAll    = new QPushButton(QStringLiteral("Replace All"),  &dlg);
    QPushButton *btnClose  = new QPushButton(QStringLiteral("Close"),        &dlg);

    row1->addWidget(new QLabel(QStringLiteral("Find:"),    &dlg));
    row1->addWidget(findEdit);
    row2->addWidget(new QLabel(QStringLiteral("Replace:"), &dlg));
    row2->addWidget(replaceEdit);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addWidget(caseCheck);
    btnRow->addStretch();
    btnRow->addWidget(btnFind);
    btnRow->addWidget(btnRepl);
    btnRow->addWidget(btnAll);
    btnRow->addWidget(btnClose);

    vl->addLayout(row1);
    vl->addLayout(row2);
    vl->addLayout(btnRow);

    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);

    connect(btnFind, &QPushButton::clicked, [&]() {
        QTextDocument::FindFlags flags;
        if (caseCheck->isChecked()) flags |= QTextDocument::FindCaseSensitively;
        ed->find(findEdit->text(), flags);
    });

    connect(btnRepl, &QPushButton::clicked, [&]() {
        QTextDocument::FindFlags flags;
        if (caseCheck->isChecked()) flags |= QTextDocument::FindCaseSensitively;
        QTextCursor cur = ed->textCursor();
        if (cur.hasSelection() &&
            cur.selectedText().compare(findEdit->text(),
                caseCheck->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive) == 0)
        {
            cur.insertText(replaceEdit->text());
        }
        ed->find(findEdit->text(), flags);
    });

    connect(btnAll, &QPushButton::clicked, [&]() {
        QString src  = findEdit->text();
        QString repl = replaceEdit->text();
        if (src.isEmpty()) return;
        Qt::CaseSensitivity cs = caseCheck->isChecked()
                                 ? Qt::CaseSensitive
                                 : Qt::CaseInsensitive;
        QString all = ed->toPlainText();
        all.replace(src, repl, cs);
        ed->setPlainText(all);
    });

    dlg.exec();
}

/* ----- Theme forwarding ----- */
void XsEditor::applyObsidianTheme()
{
    for (const EditorTab &tab : m_tabData) {
        if (tab.editor) tab.editor->applyObsidianTheme();
    }
}

void XsEditor::applyRadianceTheme()
{
    for (const EditorTab &tab : m_tabData) {
        if (tab.editor) tab.editor->applyRadianceTheme();
    }
}

/* ----- Settings forwarding ----- */
void XsEditor::setTabWidth(int w)
{
    tabWidth = w;
    for (const EditorTab &t : m_tabData) if (t.editor) t.editor->setTabWidth(w);
}

void XsEditor::setAutoIndent(bool b)
{
    autoIndent = b;
    for (const EditorTab &t : m_tabData) if (t.editor) t.editor->setAutoIndent(b);
}

void XsEditor::setBracketMatching(bool b)
{
    bracketMatching = b;
    for (const EditorTab &t : m_tabData) if (t.editor) t.editor->setBracketMatching(b);
}

void XsEditor::setHighlightCurrentLine(bool b)
{
    highlightCurrentLine = b;
    for (const EditorTab &t : m_tabData) if (t.editor) t.editor->setHighlightCurrentLine(b);
}

/* ----- Slots ----- */
void XsEditor::onTabCloseRequested(int index) { closeTab(index); }

void XsEditor::onTabChanged(int index)
{
    if (index >= 0 && index < m_tabData.size())
        emit fileChanged(m_tabData[index].filePath);
}

void XsEditor::onEditorModified()
{
    for (int i = 0; i < m_tabData.size(); ++i) {
        if (m_tabData[i].editor &&
            m_tabData[i].editor->document()->isModified())
        {
            m_tabData[i].modified = true;
            updateTabTitle(i);
        }
    }
    emit modificationChanged(true);
}

void XsEditor::onCursorMoved(int line, int col)
{
    emit cursorMoved(line, col);
}

/* ----- Helpers ----- */
XsCodeEditor *XsEditor::currentEditor() const
{
    int idx = m_tabs->currentIndex();
    if (idx < 0 || idx >= m_tabData.size()) return nullptr;
    return m_tabData[idx].editor;
}

EditorTab *XsEditor::currentTabData()
{
    int idx = m_tabs->currentIndex();
    if (idx < 0 || idx >= m_tabData.size()) return nullptr;
    return &m_tabData[idx];
}

void XsEditor::updateTabTitle(int index)
{
    if (index < 0 || index >= m_tabData.size()) return;
    const EditorTab &tab = m_tabData[index];
    QString name = tab.filePath.isEmpty()
                   ? QStringLiteral("Untitled")
                   : QFileInfo(tab.filePath).fileName();
    if (tab.modified) name += QStringLiteral(" *");
    m_tabs->setTabText(index, name);
}

bool XsEditor::saveFile(int index, const QString &path)
{
    if (index < 0 || index >= m_tabData.size()) return false;
    XsCodeEditor *ed = m_tabData[index].editor;
    if (!ed) return false;

    if (!writeFile(path, ed->toPlainText())) return false;

    ed->document()->setModified(false);
    m_tabData[index].modified = false;
    updateTabTitle(index);
    return true;
}

QString XsEditor::readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

bool XsEditor::writeFile(const QString &path, const QString &text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << text;
    return true;
}
