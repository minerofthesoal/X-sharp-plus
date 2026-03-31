/*
 * X# IDE (Qt6) - Console Panel
 * ==============================
 * QPlainTextEdit-based output console with colored output categories:
 *   info, error, success, warning, output, command
 *
 * Mirrors the feature set of the GTK3 XsConsolePanel.
 */

#pragma once

#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

class XsConsolePanel : public QWidget {
    Q_OBJECT

  public:
    /* Output categories – matching the GTK3 tag names */
    enum OutputType { Info, Error, Success, Warning, Output, Command };

    explicit XsConsolePanel(QWidget* parent = nullptr);

    /* Append text with a given type/color */
    void append(const QString& text, OutputType type = Output);

    /* Convenience overload that accepts the GTK tag name strings
       ("info", "error", "success", "warning", "output") */
    void appendTagged(const QString& text, const QString& tagName);

    /* Clear all output */
    void clear();

    /* Apply themes */
    void applyObsidianTheme();
    void applyRadianceTheme();

  private slots:
    void onClearClicked();

  private:
    QPlainTextEdit* m_output;
    QPushButton* m_clearBtn;
    QLabel* m_titleLabel;

    QMap<OutputType, QColor> m_colors;

    void setupUi();
    void setupColors(bool dark);
};
