/*
 * X# IDE (Qt6) - Project Panel Implementation
 * =============================================
 */

#include "projectpanel.h"

#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QSortFilterProxyModel>

XsProjectPanel::XsProjectPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    applyObsidianTheme();
}

void XsProjectPanel::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    /* Header label */
    m_rootLabel = new QLabel(QStringLiteral("EXPLORER"), this);
    m_rootLabel->setContentsMargins(8, 6, 4, 4);

    /* Filter edit */
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(QStringLiteral("Filter files..."));
    m_filterEdit->setContentsMargins(4, 0, 4, 0);
    connect(m_filterEdit, &QLineEdit::textChanged,
            this, &XsProjectPanel::onFilterChanged);

    /* File system model */
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setRootPath(QString());

    /* Tree view */
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setRootIndex(m_model->index(QDir::homePath()));
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);

    /* Hide columns other than Name */
    m_treeView->hideColumn(1);
    m_treeView->hideColumn(2);
    m_treeView->hideColumn(3);

    connect(m_treeView, &QTreeView::doubleClicked,
            this, &XsProjectPanel::onItemDoubleClicked);

    layout->addWidget(m_rootLabel);
    layout->addWidget(m_filterEdit);
    layout->addWidget(m_treeView);
}

void XsProjectPanel::loadDirectory(const QString &dirPath)
{
    if (dirPath.isEmpty()) return;

    m_model->setRootPath(dirPath);
    m_treeView->setRootIndex(m_model->index(dirPath));

    QFileInfo info(dirPath);
    m_rootLabel->setText(info.fileName().toUpper());
}

void XsProjectPanel::refresh()
{
    /* QFileSystemModel auto-refreshes via the OS watcher; force a reset */
    QString current = m_model->rootPath();
    if (!current.isEmpty()) loadDirectory(current);
}

QString XsProjectPanel::rootPath() const
{
    return m_model->rootPath();
}

void XsProjectPanel::onItemDoubleClicked(const QModelIndex &index)
{
    QString path = m_model->filePath(index);
    QFileInfo info(path);
    if (info.isFile())
        emit fileOpenRequested(path);
}

void XsProjectPanel::onFilterChanged(const QString &text)
{
    m_model->setNameFilters(
        text.isEmpty()
        ? QStringList()
        : QStringList{ QStringLiteral("*") + text + QStringLiteral("*") });
    m_model->setNameFilterDisables(!text.isEmpty());
}

void XsProjectPanel::applyObsidianTheme()
{
    applyStyleSheet(true);
}

void XsProjectPanel::applyRadianceTheme()
{
    applyStyleSheet(false);
}

void XsProjectPanel::applyStyleSheet(bool dark)
{
    if (dark) {
        setStyleSheet(
            QStringLiteral(
                "XsProjectPanel { background-color: #252526; }"
                "QLabel {"
                "  color: #BBBBBB;"
                "  font-size: 11px;"
                "  font-weight: bold;"
                "  background-color: transparent;"
                "}"
                "QLineEdit {"
                "  background-color: #3C3C3C;"
                "  color: #D4D4D4;"
                "  border: 1px solid #555555;"
                "  padding: 3px 6px;"
                "  margin: 2px 4px;"
                "}"
                "QTreeView {"
                "  background-color: #252526;"
                "  color: #CCCCCC;"
                "  border: none;"
                "  outline: none;"
                "}"
                "QTreeView::item:hover    { background-color: #2A2D2E; }"
                "QTreeView::item:selected { background-color: #094771; color: #FFFFFF; }"
                "QTreeView::branch:has-children:!has-siblings:closed,"
                "QTreeView::branch:closed:has-children:has-siblings {"
                "  border-image: none;"
                "  image: none;"
                "}"
            )
        );
    } else {
        setStyleSheet(
            QStringLiteral(
                "XsProjectPanel { background-color: #F3F3F3; }"
                "QLabel {"
                "  color: #424242;"
                "  font-size: 11px;"
                "  font-weight: bold;"
                "  background-color: transparent;"
                "}"
                "QLineEdit {"
                "  background-color: #FFFFFF;"
                "  color: #333333;"
                "  border: 1px solid #CCCCCC;"
                "  padding: 3px 6px;"
                "  margin: 2px 4px;"
                "}"
                "QTreeView {"
                "  background-color: #F3F3F3;"
                "  color: #333333;"
                "  border: none;"
                "  outline: none;"
                "}"
                "QTreeView::item:hover    { background-color: #E8E8E8; }"
                "QTreeView::item:selected { background-color: #0060C0; color: #FFFFFF; }"
            )
        );
    }
}
