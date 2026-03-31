/*
 * X# IDE (Qt6) - Debug Panel
 * ============================
 * Variable inspector and call stack display with step controls.
 * Mirrors the feature set of the GTK3 XsDebugPanel.
 */

#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

class XsDebugPanel : public QWidget {
    Q_OBJECT

  public:
    explicit XsDebugPanel(QWidget* parent = nullptr);

    /* Update variable inspector */
    void updateVariables(const QStringList& names, const QStringList& types,
                         const QStringList& values);

    /* Update call stack view */
    void updateCallStack(const QStringList& funcNames, const QStringList& locations);

    /* Clear both views */
    void clearAll();

    /* Enable / disable the step buttons */
    void setDebugActive(bool active);

    /* Apply themes */
    void applyObsidianTheme();
    void applyRadianceTheme();

  signals:
    void continueRequested();
    void stepInRequested();
    void stepOverRequested();
    void stepOutRequested();
    void stopRequested();

  private:
    /* Toolbar buttons */
    QPushButton* m_btnContinue;
    QPushButton* m_btnStepIn;
    QPushButton* m_btnStepOver;
    QPushButton* m_btnStepOut;
    QPushButton* m_btnStop;

    /* Variable tree: Name | Type | Value */
    QTreeWidget* m_varTree;

    /* Call-stack tree: # | Function | Location */
    QTreeWidget* m_stackTree;

    void setupUi();
    void applyStyleSheet(bool dark);
};
