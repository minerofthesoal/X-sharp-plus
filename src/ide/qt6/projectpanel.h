/*
 * X# IDE (Qt6) - Project Panel
 * ==============================
 * QTreeView-based file browser sidebar.
 * Mirrors the feature set of the GTK3 XsProjectPanel.
 */

#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QString>

class XsProjectPanel : public QWidget
{
    Q_OBJECT

public:
    explicit XsProjectPanel(QWidget *parent = nullptr);

    /* Load (or refresh) the view for a given directory */
    void loadDirectory(const QString &dirPath);

    /* Reload the currently shown directory */
    void refresh();

    /* Current root path */
    QString rootPath() const;

    /* Apply Obsidian / Radiance theme */
    void applyObsidianTheme();
    void applyRadianceTheme();

signals:
    /* Emitted when the user double-clicks a file */
    void fileOpenRequested(const QString &filePath);

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void onFilterChanged(const QString &text);

private:
    QTreeView         *m_treeView;
    QFileSystemModel  *m_model;
    QLineEdit         *m_filterEdit;
    QLabel            *m_rootLabel;

    void setupUi();
    void applyStyleSheet(bool dark);
};
