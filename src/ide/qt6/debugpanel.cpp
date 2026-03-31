/*
 * X# IDE (Qt6) - Debug Panel Implementation
 * ===========================================
 */

#include "debugpanel.h"

XsDebugPanel::XsDebugPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    applyObsidianTheme();
    setDebugActive(false);
}

void XsDebugPanel::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    /* ---- Toolbar ---- */
    QWidget     *toolbar  = new QWidget(this);
    QHBoxLayout *toolHBox = new QHBoxLayout(toolbar);
    toolHBox->setContentsMargins(4, 4, 4, 4);
    toolHBox->setSpacing(4);

    auto makeBtn = [&](const QString &text, const QString &tooltip) -> QPushButton * {
        auto *btn = new QPushButton(text, toolbar);
        btn->setToolTip(tooltip);
        btn->setFixedHeight(26);
        btn->setFixedWidth(70);
        btn->setFlat(true);
        return btn;
    };

    m_btnContinue  = makeBtn(QStringLiteral("Continue"),  QStringLiteral("Continue execution (F5)"));
    m_btnStepOver  = makeBtn(QStringLiteral("Step Over"), QStringLiteral("Step over (F10)"));
    m_btnStepIn    = makeBtn(QStringLiteral("Step In"),   QStringLiteral("Step into (F11)"));
    m_btnStepOut   = makeBtn(QStringLiteral("Step Out"),  QStringLiteral("Step out (Shift+F11)"));
    m_btnStop      = makeBtn(QStringLiteral("Stop"),      QStringLiteral("Stop debugging (Shift+F5)"));

    toolHBox->addWidget(m_btnContinue);
    toolHBox->addWidget(m_btnStepOver);
    toolHBox->addWidget(m_btnStepIn);
    toolHBox->addWidget(m_btnStepOut);
    toolHBox->addWidget(m_btnStop);
    toolHBox->addStretch();

    connect(m_btnContinue, &QPushButton::clicked, this, &XsDebugPanel::continueRequested);
    connect(m_btnStepIn,   &QPushButton::clicked, this, &XsDebugPanel::stepInRequested);
    connect(m_btnStepOver, &QPushButton::clicked, this, &XsDebugPanel::stepOverRequested);
    connect(m_btnStepOut,  &QPushButton::clicked, this, &XsDebugPanel::stepOutRequested);
    connect(m_btnStop,     &QPushButton::clicked, this, &XsDebugPanel::stopRequested);

    mainLayout->addWidget(toolbar);

    /* ---- Splitter (variables | call stack) ---- */
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    splitter->setHandleWidth(2);

    /* Variables pane */
    QWidget     *varWidget = new QWidget(splitter);
    QVBoxLayout *varLayout = new QVBoxLayout(varWidget);
    varLayout->setContentsMargins(0, 0, 0, 0);
    varLayout->setSpacing(0);

    QLabel *varLabel = new QLabel(QStringLiteral("VARIABLES"), varWidget);
    varLabel->setContentsMargins(8, 4, 4, 4);

    m_varTree = new QTreeWidget(varWidget);
    m_varTree->setHeaderLabels({ QStringLiteral("Name"),
                                  QStringLiteral("Type"),
                                  QStringLiteral("Value") });
    m_varTree->setColumnWidth(0, 130);
    m_varTree->setColumnWidth(1, 100);
    m_varTree->setRootIsDecorated(false);
    m_varTree->setAlternatingRowColors(true);
    m_varTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

    varLayout->addWidget(varLabel);
    varLayout->addWidget(m_varTree);

    /* Call stack pane */
    QWidget     *stackWidget = new QWidget(splitter);
    QVBoxLayout *stackLayout = new QVBoxLayout(stackWidget);
    stackLayout->setContentsMargins(0, 0, 0, 0);
    stackLayout->setSpacing(0);

    QLabel *stackLabel = new QLabel(QStringLiteral("CALL STACK"), stackWidget);
    stackLabel->setContentsMargins(8, 4, 4, 4);

    m_stackTree = new QTreeWidget(stackWidget);
    m_stackTree->setHeaderLabels({ QStringLiteral("#"),
                                    QStringLiteral("Function"),
                                    QStringLiteral("Location") });
    m_stackTree->setColumnWidth(0,  30);
    m_stackTree->setColumnWidth(1, 150);
    m_stackTree->setRootIsDecorated(false);
    m_stackTree->setAlternatingRowColors(true);
    m_stackTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

    stackLayout->addWidget(stackLabel);
    stackLayout->addWidget(m_stackTree);

    splitter->addWidget(varWidget);
    splitter->addWidget(stackWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

/* ---- Data update ---- */

void XsDebugPanel::updateVariables(const QStringList &names,
                                   const QStringList &types,
                                   const QStringList &values)
{
    m_varTree->clear();
    int count = names.size();
    for (int i = 0; i < count; ++i) {
        auto *item = new QTreeWidgetItem(m_varTree);
        item->setText(0, i < names.size()  ? names.at(i)  : QString());
        item->setText(1, i < types.size()  ? types.at(i)  : QString());
        item->setText(2, i < values.size() ? values.at(i) : QString());
    }
}

void XsDebugPanel::updateCallStack(const QStringList &funcNames,
                                   const QStringList &locations)
{
    m_stackTree->clear();
    int count = funcNames.size();
    for (int i = 0; i < count; ++i) {
        auto *item = new QTreeWidgetItem(m_stackTree);
        item->setText(0, QString::number(i));
        item->setText(1, i < funcNames.size()  ? funcNames.at(i)  : QString());
        item->setText(2, i < locations.size()  ? locations.at(i)  : QString());
    }
}

void XsDebugPanel::clearAll()
{
    m_varTree->clear();
    m_stackTree->clear();
}

void XsDebugPanel::setDebugActive(bool active)
{
    m_btnContinue->setEnabled(active);
    m_btnStepIn->setEnabled(active);
    m_btnStepOver->setEnabled(active);
    m_btnStepOut->setEnabled(active);
    m_btnStop->setEnabled(active);
}

/* ---- Theming ---- */

void XsDebugPanel::applyObsidianTheme()
{
    applyStyleSheet(true);
}

void XsDebugPanel::applyRadianceTheme()
{
    applyStyleSheet(false);
}

void XsDebugPanel::applyStyleSheet(bool dark)
{
    if (dark) {
        setStyleSheet(
            QStringLiteral(
                "XsDebugPanel { background-color: #252526; }"
                "QWidget { background-color: #252526; color: #CCCCCC; }"
                "QLabel {"
                "  color: #BBBBBB;"
                "  font-size: 11px;"
                "  font-weight: bold;"
                "  background-color: #252526;"
                "}"
                "QPushButton {"
                "  color: #9D9D9D;"
                "  background: #3C3C3C;"
                "  border: 1px solid #555555;"
                "  border-radius: 3px;"
                "  font-size: 11px;"
                "  padding: 2px 4px;"
                "}"
                "QPushButton:hover:enabled  { background: #505050; color: #FFFFFF; }"
                "QPushButton:disabled { color: #555555; background: #2D2D2D; }"
                "QTreeWidget {"
                "  background-color: #1E1E1E;"
                "  color: #CCCCCC;"
                "  border: none;"
                "  alternate-background-color: #252526;"
                "}"
                "QTreeWidget::item:selected {"
                "  background-color: #094771;"
                "  color: #FFFFFF;"
                "}"
                "QHeaderView::section {"
                "  background-color: #252526;"
                "  color: #9D9D9D;"
                "  border: none;"
                "  border-right: 1px solid #3C3C3C;"
                "  padding: 3px 6px;"
                "  font-size: 11px;"
                "}"
                "QSplitter::handle { background-color: #1E1E1E; }"
            )
        );
    } else {
        setStyleSheet(
            QStringLiteral(
                "XsDebugPanel { background-color: #F3F3F3; }"
                "QWidget { background-color: #F3F3F3; color: #333333; }"
                "QLabel {"
                "  color: #424242;"
                "  font-size: 11px;"
                "  font-weight: bold;"
                "}"
                "QPushButton {"
                "  color: #333333;"
                "  background: #E8E8E8;"
                "  border: 1px solid #CCCCCC;"
                "  border-radius: 3px;"
                "  font-size: 11px;"
                "  padding: 2px 4px;"
                "}"
                "QPushButton:hover:enabled { background: #D0D0D0; }"
                "QPushButton:disabled { color: #AAAAAA; }"
                "QTreeWidget {"
                "  background-color: #FFFFFF;"
                "  color: #333333;"
                "  border: 1px solid #E0E0E0;"
                "  alternate-background-color: #F5F5F5;"
                "}"
                "QTreeWidget::item:selected {"
                "  background-color: #0060C0;"
                "  color: #FFFFFF;"
                "}"
                "QHeaderView::section {"
                "  background-color: #F3F3F3;"
                "  color: #424242;"
                "  border: none;"
                "  border-right: 1px solid #E0E0E0;"
                "  padding: 3px 6px;"
                "  font-size: 11px;"
                "}"
            )
        );
    }
}
