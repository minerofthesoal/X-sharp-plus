/*
 * X# IDE (Qt6) - Console Panel Implementation
 * =============================================
 */

#include "consolepanel.h"

#include <QTextCharFormat>
#include <QTextCursor>
#include <QFont>
#include <QScrollBar>

XsConsolePanel::XsConsolePanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupColors(true);
    applyObsidianTheme();
}

void XsConsolePanel::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    /* Header bar */
    QWidget    *header = new QWidget(this);
    QHBoxLayout *hdr   = new QHBoxLayout(header);
    hdr->setContentsMargins(8, 4, 4, 4);
    hdr->setSpacing(6);

    m_titleLabel = new QLabel(QStringLiteral("OUTPUT"), header);
    m_clearBtn   = new QPushButton(QStringLiteral("Clear"), header);
    m_clearBtn->setFlat(true);
    m_clearBtn->setFixedHeight(22);

    hdr->addWidget(m_titleLabel);
    hdr->addStretch();
    hdr->addWidget(m_clearBtn);

    connect(m_clearBtn, &QPushButton::clicked,
            this, &XsConsolePanel::onClearClicked);

    /* Output view */
    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_output->setMaximumBlockCount(10000); /* Prevent unbounded growth */

    QFont font(QStringLiteral("Cascadia Code"));
    if (!font.exactMatch()) font.setFamily(QStringLiteral("Consolas"));
    if (!font.exactMatch()) font.setFamily(QStringLiteral("Monospace"));
    font.setPointSize(11);
    font.setFixedPitch(true);
    m_output->setFont(font);

    layout->addWidget(header);
    layout->addWidget(m_output);
}

void XsConsolePanel::setupColors(bool dark)
{
    if (dark) {
        m_colors[Info]    = QColor(0x56, 0x9C, 0xD6); /* #569CD6 – blue     */
        m_colors[Error]   = QColor(0xF4, 0x43, 0x36); /* #F44336 – red      */
        m_colors[Success] = QColor(0x4C, 0xAF, 0x50); /* #4CAF50 – green    */
        m_colors[Warning] = QColor(0xFF, 0xB7, 0x00); /* #FFB700 – amber    */
        m_colors[Output]  = QColor(0xD4, 0xD4, 0xD4); /* #D4D4D4 – default  */
        m_colors[Command] = QColor(0xCE, 0x91, 0x78); /* #CE9178 – orange   */
    } else {
        m_colors[Info]    = QColor(0x00, 0x70, 0xC1); /* #0070C1 */
        m_colors[Error]   = QColor(0xCC, 0x00, 0x00); /* #CC0000 */
        m_colors[Success] = QColor(0x00, 0x7E, 0x33); /* #007E33 */
        m_colors[Warning] = QColor(0xE6, 0x5C, 0x00); /* #E65C00 */
        m_colors[Output]  = QColor(0x00, 0x00, 0x00); /* #000000 */
        m_colors[Command] = QColor(0x80, 0x00, 0x80); /* #800080 */
    }
}

void XsConsolePanel::append(const QString &text, OutputType type)
{
    QTextCharFormat fmt;
    fmt.setForeground(m_colors.value(type, QColor(Qt::white)));

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, fmt);

    /* Auto-scroll to bottom */
    QScrollBar *sb = m_output->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void XsConsolePanel::appendTagged(const QString &text, const QString &tagName)
{
    static const QMap<QString, OutputType> tagMap = {
        { QStringLiteral("info"),    Info    },
        { QStringLiteral("error"),   Error   },
        { QStringLiteral("success"), Success },
        { QStringLiteral("warning"), Warning },
        { QStringLiteral("output"),  Output  },
        { QStringLiteral("command"), Command },
    };
    append(text, tagMap.value(tagName, Output));
}

void XsConsolePanel::clear()
{
    m_output->clear();
}

void XsConsolePanel::onClearClicked()
{
    clear();
}

void XsConsolePanel::applyObsidianTheme()
{
    setupColors(true);
    setStyleSheet(
        QStringLiteral(
            "XsConsolePanel { background-color: #1E1E1E; }"
            "QWidget#header  { background-color: #252526; }"
            "QLabel {"
            "  color: #BBBBBB;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "  background-color: transparent;"
            "}"
            "QPushButton {"
            "  color: #9D9D9D;"
            "  background: transparent;"
            "  border: 1px solid transparent;"
            "  padding: 1px 8px;"
            "  font-size: 11px;"
            "}"
            "QPushButton:hover { color: #FFFFFF; border: 1px solid #555555; }"
            "QPlainTextEdit {"
            "  background-color: #1E1E1E;"
            "  color: #D4D4D4;"
            "  border: none;"
            "}"
            "QScrollBar:vertical {"
            "  background: #1E1E1E;"
            "  width: 10px;"
            "}"
            "QScrollBar::handle:vertical {"
            "  background: #424242;"
            "  border-radius: 4px;"
            "}"
        )
    );
}

void XsConsolePanel::applyRadianceTheme()
{
    setupColors(false);
    setStyleSheet(
        QStringLiteral(
            "XsConsolePanel { background-color: #FFFFFF; }"
            "QLabel {"
            "  color: #424242;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "  background-color: transparent;"
            "}"
            "QPushButton {"
            "  color: #616161;"
            "  background: transparent;"
            "  border: 1px solid transparent;"
            "  padding: 1px 8px;"
            "  font-size: 11px;"
            "}"
            "QPushButton:hover { color: #000000; border: 1px solid #CCCCCC; }"
            "QPlainTextEdit {"
            "  background-color: #FFFFFF;"
            "  color: #333333;"
            "  border: none;"
            "}"
        )
    );
}
