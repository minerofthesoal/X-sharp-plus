/*
 * X# IDE (Qt6) - Main Window Implementation
 * ===========================================
 */

#include "mainwindow.h"
#include "editor.h"
#include "projectpanel.h"
#include "consolepanel.h"
#include "debugpanel.h"

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QCloseEvent>
#include <QFontDatabase>
#include <QVBoxLayout>

static const char *APP_TITLE   = "X# IDE";
static const char *APP_VERSION = "1.0.0";
static const char *APP_ORG     = "XSharp";
static const char *APP_NAME    = "xsharp-ide";

/* =========================================================================
 * Construction / destruction
 * ========================================================================= */

XsMainWindow::XsMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QString::fromLatin1(APP_TITLE));
    resize(1280, 800);

    /* Central editor */
    m_editor = new XsEditor(this);
    setCentralWidget(m_editor);

    /* Panels */
    m_projectPanel = new XsProjectPanel(this);
    m_consolePanel = new XsConsolePanel(this);
    m_debugPanel   = new XsDebugPanel(this);

    /* Build UI chrome */
    buildMenus();
    buildToolbar();
    buildStatusBar();
    buildDockPanels();

    /* Auto-save timer */
    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout,
            this, &XsMainWindow::onAutoSaveTick);

    /* Wire up editor signals */
    connect(m_editor, &XsEditor::cursorMoved,
            this, &XsMainWindow::onCursorMoved);
    connect(m_editor, &XsEditor::fileChanged,
            this, &XsMainWindow::onFileChanged);

    /* Wire up project panel */
    connect(m_projectPanel, &XsProjectPanel::fileOpenRequested,
            m_editor, &XsEditor::openFile);

    /* Wire up debug panel */
    connect(m_debugPanel, &XsDebugPanel::continueRequested,
            this, &XsMainWindow::onDebugContinue);
    connect(m_debugPanel, &XsDebugPanel::stepOverRequested,
            this, &XsMainWindow::onDebugStepOver);
    connect(m_debugPanel, &XsDebugPanel::stepInRequested,
            this, &XsMainWindow::onDebugStepIn);
    connect(m_debugPanel, &XsDebugPanel::stepOutRequested,
            this, &XsMainWindow::onDebugStepOut);
    connect(m_debugPanel, &XsDebugPanel::stopRequested,
            this, &XsMainWindow::onDebugStop);

    /* Load preferences then apply theme */
    loadPreferences();
    if (m_prefs.themeName == QStringLiteral("Radiance"))
        applyRadianceTheme();
    else
        applyObsidianTheme();

    /* Initial status */
    updateStatusBar(1, 1, QStringLiteral("Untitled"));

    /* Welcome message */
    m_consolePanel->append(
        QStringLiteral("X# IDE v") + QString::fromLatin1(APP_VERSION) +
        QStringLiteral(" ready.\nOpen a file or project to get started.\n"),
        XsConsolePanel::Info);
}

XsMainWindow::~XsMainWindow()
{
    savePreferences();
}

/* =========================================================================
 * UI Construction
 * ========================================================================= */

void XsMainWindow::buildMenus()
{
    /* ---- File ---- */
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

    fileMenu->addAction(QStringLiteral("&New"),
        this, &XsMainWindow::onNewFile,
        QKeySequence::New);

    fileMenu->addAction(QStringLiteral("&Open..."),
        this, &XsMainWindow::onOpenFile,
        QKeySequence::Open);

    fileMenu->addAction(QStringLiteral("Open &Project..."),
        this, &XsMainWindow::onOpenProject);

    fileMenu->addSeparator();

    fileMenu->addAction(QStringLiteral("&Save"),
        this, &XsMainWindow::onSaveFile,
        QKeySequence::Save);

    fileMenu->addAction(QStringLiteral("Save &As..."),
        this, &XsMainWindow::onSaveFileAs,
        QKeySequence::SaveAs);

    fileMenu->addSeparator();

    /* Recent files sub-menu */
    m_recentMenu = fileMenu->addMenu(QStringLiteral("Recent &Files"));
    rebuildRecentMenu();

    fileMenu->addSeparator();

    fileMenu->addAction(QStringLiteral("&Close"),
        this, &XsMainWindow::onCloseFile,
        QKeySequence::Close);

    fileMenu->addSeparator();

    fileMenu->addAction(QStringLiteral("&Preferences..."),
        this, &XsMainWindow::onPreferences);

    fileMenu->addSeparator();

    fileMenu->addAction(QStringLiteral("E&xit"),
        qApp, &QApplication::quit,
        QKeySequence::Quit);

    /* ---- Edit ---- */
    QMenu *editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));

    editMenu->addAction(QStringLiteral("&Undo"),
        this, &XsMainWindow::onUndo,
        QKeySequence::Undo);

    editMenu->addAction(QStringLiteral("&Redo"),
        this, &XsMainWindow::onRedo,
        QKeySequence::Redo);

    editMenu->addSeparator();

    editMenu->addAction(QStringLiteral("Cu&t"),
        this, &XsMainWindow::onCut,
        QKeySequence::Cut);

    editMenu->addAction(QStringLiteral("&Copy"),
        this, &XsMainWindow::onCopy,
        QKeySequence::Copy);

    editMenu->addAction(QStringLiteral("&Paste"),
        this, &XsMainWindow::onPaste,
        QKeySequence::Paste);

    editMenu->addSeparator();

    editMenu->addAction(QStringLiteral("&Find / Replace..."),
        this, &XsMainWindow::onFind,
        QKeySequence::Find);

    /* ---- View ---- */
    QMenu *viewMenu = menuBar()->addMenu(QStringLiteral("&View"));

    viewMenu->addAction(QStringLiteral("Toggle &Theme (Obsidian/Radiance)"),
        this, &XsMainWindow::onToggleTheme);

    viewMenu->addSeparator();

    viewMenu->addAction(QStringLiteral("Show/Hide &Project Panel"),
        this, &XsMainWindow::onToggleProjectPanel,
        QKeySequence(Qt::CTRL | Qt::Key_1));

    viewMenu->addAction(QStringLiteral("Show/Hide &Console Panel"),
        this, &XsMainWindow::onToggleConsolePanel,
        QKeySequence(Qt::CTRL | Qt::Key_2));

    viewMenu->addAction(QStringLiteral("Show/Hide &Debug Panel"),
        this, &XsMainWindow::onToggleDebugPanel,
        QKeySequence(Qt::CTRL | Qt::Key_3));

    /* ---- Build ---- */
    QMenu *buildMenu = menuBar()->addMenu(QStringLiteral("&Build"));

    buildMenu->addAction(QStringLiteral("&Build Project"),
        this, &XsMainWindow::onBuild,
        QKeySequence(Qt::CTRL | Qt::Key_B));

    buildMenu->addAction(QStringLiteral("&Run"),
        this, &XsMainWindow::onRun,
        QKeySequence(Qt::CTRL | Qt::Key_R));

    buildMenu->addAction(QStringLiteral("&Stop"),
        this, &XsMainWindow::onStop);

    /* ---- Debug ---- */
    QMenu *debugMenu = menuBar()->addMenu(QStringLiteral("&Debug"));

    debugMenu->addAction(QStringLiteral("&Start Debugging"),
        this, &XsMainWindow::onDebugStart,
        QKeySequence(Qt::Key_F5));

    debugMenu->addAction(QStringLiteral("S&top Debugging"),
        this, &XsMainWindow::onDebugStop,
        QKeySequence(Qt::SHIFT | Qt::Key_F5));

    debugMenu->addSeparator();

    debugMenu->addAction(QStringLiteral("Step &Over"),
        this, &XsMainWindow::onDebugStepOver,
        QKeySequence(Qt::Key_F10));

    debugMenu->addAction(QStringLiteral("Step &Into"),
        this, &XsMainWindow::onDebugStepIn,
        QKeySequence(Qt::Key_F11));

    debugMenu->addAction(QStringLiteral("Step O&ut"),
        this, &XsMainWindow::onDebugStepOut,
        QKeySequence(Qt::SHIFT | Qt::Key_F11));

    /* ---- Help ---- */
    QMenu *helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));

    helpMenu->addAction(QStringLiteral("&About X# IDE"),
        this, &XsMainWindow::onAbout);

    helpMenu->addAction(QStringLiteral("About &Qt"),
        qApp, &QApplication::aboutQt);
}

void XsMainWindow::buildToolbar()
{
    QToolBar *tb = addToolBar(QStringLiteral("Main Toolbar"));
    tb->setObjectName(QStringLiteral("mainToolBar"));
    tb->setMovable(false);
    tb->setIconSize(QSize(18, 18));

    tb->addAction(QStringLiteral("New"),   this, &XsMainWindow::onNewFile);
    tb->addAction(QStringLiteral("Open"),  this, &XsMainWindow::onOpenFile);
    tb->addAction(QStringLiteral("Save"),  this, &XsMainWindow::onSaveFile);
    tb->addSeparator();
    tb->addAction(QStringLiteral("Build"), this, &XsMainWindow::onBuild);
    tb->addAction(QStringLiteral("Run"),   this, &XsMainWindow::onRun);
    tb->addSeparator();
    tb->addAction(QStringLiteral("Debug"), this, &XsMainWindow::onDebugStart);
    tb->addAction(QStringLiteral("Stop"),  this, &XsMainWindow::onDebugStop);
}

void XsMainWindow::buildStatusBar()
{
    m_statusCursor   = new QLabel(QStringLiteral("Ln 1, Col 1"), this);
    m_statusFile     = new QLabel(QStringLiteral("Untitled"),     this);
    m_statusLanguage = new QLabel(QStringLiteral("X#"),           this);
    m_statusEncoding = new QLabel(QStringLiteral("UTF-8"),        this);

    for (QLabel *lbl : { m_statusCursor, m_statusFile,
                          m_statusLanguage, m_statusEncoding }) {
        lbl->setContentsMargins(8, 0, 8, 0);
    }

    statusBar()->addWidget(m_statusFile);
    statusBar()->addWidget(m_statusCursor);
    statusBar()->addPermanentWidget(m_statusLanguage);
    statusBar()->addPermanentWidget(m_statusEncoding);
}

void XsMainWindow::buildDockPanels()
{
    /* Project panel – left */
    m_projectDock = new QDockWidget(QStringLiteral("Project"), this);
    m_projectDock->setObjectName(QStringLiteral("projectDock"));
    m_projectDock->setWidget(m_projectPanel);
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_projectDock->setMinimumWidth(180);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock);

    /* Console panel – bottom */
    m_consoleDock = new QDockWidget(QStringLiteral("Output"), this);
    m_consoleDock->setObjectName(QStringLiteral("consoleDock"));
    m_consoleDock->setWidget(m_consolePanel);
    m_consoleDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    m_consoleDock->setMinimumHeight(120);
    addDockWidget(Qt::BottomDockWidgetArea, m_consoleDock);

    /* Debug panel – bottom (tabbed with console) */
    m_debugDock = new QDockWidget(QStringLiteral("Debug"), this);
    m_debugDock->setObjectName(QStringLiteral("debugDock"));
    m_debugDock->setWidget(m_debugPanel);
    m_debugDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    tabifyDockWidget(m_consoleDock, m_debugDock);

    /* Make console the active tab */
    m_consoleDock->raise();
}

/* =========================================================================
 * File actions
 * ========================================================================= */

void XsMainWindow::onNewFile()
{
    m_editor->newTab();
    updateWindowTitle(QStringLiteral("Untitled"));
}

void XsMainWindow::onOpenFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open File"),
        m_projectPanel->rootPath(),
        QStringLiteral("X# Files (*.xs);;All Files (*)"));

    if (!path.isEmpty()) {
        m_editor->openFile(path);
        addRecentFile(path);
        updateWindowTitle(QFileInfo(path).fileName());
    }
}

void XsMainWindow::onSaveFile()
{
    if (m_editor->saveCurrent()) {
        const QString path = m_editor->currentFilePath();
        if (!path.isEmpty()) {
            addRecentFile(path);
            updateWindowTitle(QFileInfo(path).fileName());
            m_consolePanel->append(
                QStringLiteral("Saved: ") + path + QLatin1Char('\n'),
                XsConsolePanel::Info);
        }
    }
}

void XsMainWindow::onSaveFileAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save File As"),
        m_projectPanel->rootPath(),
        QStringLiteral("X# Files (*.xs);;All Files (*)"));

    if (!path.isEmpty()) {
        m_editor->saveAs(path);
        addRecentFile(path);
        updateWindowTitle(QFileInfo(path).fileName());
    }
}

void XsMainWindow::onOpenProject()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Open Project Directory"),
        QDir::homePath());

    if (!dir.isEmpty()) {
        m_projectPanel->loadDirectory(dir);
        updateWindowTitle(QFileInfo(dir).fileName());
        m_consolePanel->append(
            QStringLiteral("Project loaded: ") + dir + QLatin1Char('\n'),
            XsConsolePanel::Info);
    }
}

void XsMainWindow::onCloseFile()
{
    m_editor->closeCurrent();
}

/* =========================================================================
 * Edit actions
 * ========================================================================= */

void XsMainWindow::onUndo()  { m_editor->undo(); }
void XsMainWindow::onRedo()  { m_editor->redo(); }
void XsMainWindow::onFind()  { m_editor->showFindDialog(); }

void XsMainWindow::onCut()
{
    if (auto *ed = m_editor->tabWidget()->currentWidget())
        QMetaObject::invokeMethod(ed, "cut");
}

void XsMainWindow::onCopy()
{
    if (auto *ed = m_editor->tabWidget()->currentWidget())
        QMetaObject::invokeMethod(ed, "copy");
}

void XsMainWindow::onPaste()
{
    if (auto *ed = m_editor->tabWidget()->currentWidget())
        QMetaObject::invokeMethod(ed, "paste");
}

/* =========================================================================
 * View actions
 * ========================================================================= */

void XsMainWindow::onToggleTheme()
{
    if (m_prefs.themeName == QStringLiteral("Obsidian")) {
        applyRadianceTheme();
        m_prefs.themeName = QStringLiteral("Radiance");
    } else {
        applyObsidianTheme();
        m_prefs.themeName = QStringLiteral("Obsidian");
    }
}

void XsMainWindow::onToggleProjectPanel()
{
    m_projectDock->setVisible(!m_projectDock->isVisible());
}

void XsMainWindow::onToggleConsolePanel()
{
    m_consoleDock->setVisible(!m_consoleDock->isVisible());
}

void XsMainWindow::onToggleDebugPanel()
{
    m_debugDock->setVisible(!m_debugDock->isVisible());
}

void XsMainWindow::onPreferences()
{
    showPreferencesDialog();
}

/* =========================================================================
 * Build actions
 * ========================================================================= */

void XsMainWindow::onBuild()
{
    /* Save current file first */
    m_editor->saveCurrent();

    m_consolePanel->clear();
    m_consolePanel->append(QStringLiteral("Building project...\n"), XsConsolePanel::Info);
    m_consoleDock->raise();

    if (m_buildProcess) {
        m_buildProcess->kill();
        m_buildProcess->deleteLater();
    }
    m_buildProcess = new QProcess(this);
    connect(m_buildProcess, &QProcess::readyReadStandardOutput,
            this, &XsMainWindow::onBuildProcessOutput);
    connect(m_buildProcess, &QProcess::readyReadStandardError,
            this, &XsMainWindow::onBuildProcessOutput);
    connect(m_buildProcess,
            static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &XsMainWindow::onBuildProcessFinished);

    m_buildProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_buildProcess->start(QStringLiteral("/bin/sh"),
        QStringList{ QStringLiteral("-c"), m_prefs.buildCommand });
}

void XsMainWindow::onRun()
{
    m_consolePanel->clear();
    m_consolePanel->append(
        QStringLiteral("Running: ") + m_prefs.runCommand + QLatin1Char('\n'),
        XsConsolePanel::Command);
    m_consoleDock->raise();

    QProcess *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc]() {
        m_consolePanel->append(
            QString::fromLocal8Bit(proc->readAllStandardOutput()),
            XsConsolePanel::Output);
    });
    connect(proc,
            static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this, proc](int code, QProcess::ExitStatus) {
                m_consolePanel->append(
                    QStringLiteral("\nProcess exited with code ") +
                    QString::number(code) + QLatin1Char('\n'),
                    code == 0 ? XsConsolePanel::Success : XsConsolePanel::Error);
                proc->deleteLater();
            });

    proc->start(QStringLiteral("/bin/sh"),
        QStringList{ QStringLiteral("-c"), m_prefs.runCommand });
}

void XsMainWindow::onStop()
{
    if (m_buildProcess && m_buildProcess->state() != QProcess::NotRunning) {
        m_buildProcess->kill();
        m_consolePanel->append(QStringLiteral("\nBuild stopped.\n"), XsConsolePanel::Warning);
    }
}

void XsMainWindow::onBuildProcessOutput()
{
    if (!m_buildProcess) return;
    const QString output = QString::fromLocal8Bit(m_buildProcess->readAll());
    m_consolePanel->append(output, XsConsolePanel::Output);
}

void XsMainWindow::onBuildProcessFinished(int exitCode, QProcess::ExitStatus /*status*/)
{
    if (exitCode == 0) {
        m_consolePanel->append(QStringLiteral("\nBuild succeeded.\n"), XsConsolePanel::Success);
    } else {
        m_consolePanel->append(
            QStringLiteral("\nBuild failed (exit code ") +
            QString::number(exitCode) + QStringLiteral(").\n"),
            XsConsolePanel::Error);
    }
    m_buildProcess->deleteLater();
    m_buildProcess = nullptr;
}

/* =========================================================================
 * Debug actions
 * ========================================================================= */

void XsMainWindow::onDebugStart()
{
    m_isDebugging = true;
    m_debugPanel->setDebugActive(true);
    m_debugDock->raise();
    m_consolePanel->append(QStringLiteral("Starting debugger...\n"), XsConsolePanel::Info);
    /* Full DAP integration would launch a debug adapter here */
}

void XsMainWindow::onDebugStop()
{
    m_isDebugging = false;
    m_debugPanel->setDebugActive(false);
    m_debugPanel->clearAll();
    m_consolePanel->append(QStringLiteral("Debugger stopped.\n"), XsConsolePanel::Info);
}

void XsMainWindow::onDebugContinue()
{
    m_consolePanel->append(QStringLiteral("[dbg] continue\n"), XsConsolePanel::Info);
}

void XsMainWindow::onDebugStepOver()
{
    m_consolePanel->append(QStringLiteral("[dbg] step over\n"), XsConsolePanel::Info);
}

void XsMainWindow::onDebugStepIn()
{
    m_consolePanel->append(QStringLiteral("[dbg] step in\n"), XsConsolePanel::Info);
}

void XsMainWindow::onDebugStepOut()
{
    m_consolePanel->append(QStringLiteral("[dbg] step out\n"), XsConsolePanel::Info);
}

/* =========================================================================
 * Help
 * ========================================================================= */

void XsMainWindow::onAbout()
{
    QMessageBox::about(
        this,
        QStringLiteral("About X# IDE"),
        QStringLiteral(
            "<h2>X# IDE v") + QString::fromLatin1(APP_VERSION) + QStringLiteral("</h2>"
            "<p>A native IDE for the <b>X#</b> programming language.</p>"
            "<p>Built with <b>Qt6</b> and C++.</p>"
            "<p>Features:<br>"
            "&nbsp;&bull; Syntax highlighting for X# keywords<br>"
            "&nbsp;&bull; Multi-tab editor with line numbers<br>"
            "&nbsp;&bull; Project file browser<br>"
            "&nbsp;&bull; Build &amp; run integration<br>"
            "&nbsp;&bull; Debug panel with variable inspector<br>"
            "&nbsp;&bull; Obsidian (dark) and Radiance (light) themes</p>"
        )
    );
}

/* =========================================================================
 * Internal signal handlers
 * ========================================================================= */

void XsMainWindow::onCursorMoved(int line, int col)
{
    updateStatusBar(line, col, m_editor->currentFilePath());
}

void XsMainWindow::onFileChanged(const QString &path)
{
    const QString name = path.isEmpty()
                         ? QStringLiteral("Untitled")
                         : QFileInfo(path).fileName();
    updateWindowTitle(name);
    m_statusFile->setText(name);
}

void XsMainWindow::onAutoSaveTick()
{
    if (m_prefs.autoSave) m_editor->saveCurrent();
}

/* =========================================================================
 * Close event
 * ========================================================================= */

void XsMainWindow::closeEvent(QCloseEvent *event)
{
    savePreferences();
    event->accept();
}

/* =========================================================================
 * Helpers
 * ========================================================================= */

void XsMainWindow::updateWindowTitle(const QString &filename)
{
    const QString name = filename.isEmpty()
                         ? QStringLiteral("Untitled")
                         : filename;
    setWindowTitle(name + QStringLiteral(" - ") + QString::fromLatin1(APP_TITLE));
}

void XsMainWindow::updateStatusBar(int line, int col,
                                   const QString &file,
                                   const QString &encoding)
{
    m_statusCursor->setText(
        QStringLiteral("Ln ") + QString::number(line) +
        QStringLiteral(", Col ") + QString::number(col));

    const QString name = file.isEmpty()
                         ? QStringLiteral("Untitled")
                         : QFileInfo(file).fileName();
    m_statusFile->setText(name);
    m_statusEncoding->setText(encoding);
}

/* =========================================================================
 * Themes
 * ========================================================================= */

void XsMainWindow::applyObsidianTheme()
{
    qApp->setStyleSheet(obsidianStyleSheet());
    m_editor->applyObsidianTheme();
    m_projectPanel->applyObsidianTheme();
    m_consolePanel->applyObsidianTheme();
    m_debugPanel->applyObsidianTheme();
}

void XsMainWindow::applyRadianceTheme()
{
    qApp->setStyleSheet(radianceStyleSheet());
    m_editor->applyRadianceTheme();
    m_projectPanel->applyRadianceTheme();
    m_consolePanel->applyRadianceTheme();
    m_debugPanel->applyRadianceTheme();
}

QString XsMainWindow::obsidianStyleSheet() const
{
    return QStringLiteral(
        /* ---- Main window ---- */
        "QMainWindow { background-color: #1E1E1E; }"
        "QMainWindow::separator {"
        "  background: #3C3C3C;"
        "  width: 2px; height: 2px;"
        "}"

        /* ---- Menu bar ---- */
        "QMenuBar {"
        "  background-color: #3C3C3C;"
        "  color: #CCCCCC;"
        "  border-bottom: 1px solid #1E1E1E;"
        "}"
        "QMenuBar::item:selected { background-color: #505050; }"
        "QMenuBar::item:pressed  { background-color: #007ACC; }"

        /* ---- Menu ---- */
        "QMenu {"
        "  background-color: #252526;"
        "  color: #CCCCCC;"
        "  border: 1px solid #454545;"
        "}"
        "QMenu::item:selected { background-color: #094771; color: #FFFFFF; }"
        "QMenu::separator { background: #454545; height: 1px; margin: 3px 0; }"

        /* ---- Toolbar ---- */
        "QToolBar {"
        "  background-color: #3C3C3C;"
        "  border: none;"
        "  spacing: 3px;"
        "  padding: 2px 4px;"
        "}"
        "QToolButton {"
        "  color: #CCCCCC;"
        "  background: transparent;"
        "  border: 1px solid transparent;"
        "  border-radius: 3px;"
        "  padding: 3px 8px;"
        "  font-size: 12px;"
        "}"
        "QToolButton:hover   { background-color: #505050; border-color: #606060; }"
        "QToolButton:pressed { background-color: #007ACC; color: #FFFFFF; }"

        /* ---- Status bar ---- */
        "QStatusBar {"
        "  background-color: #007ACC;"
        "  color: #FFFFFF;"
        "  font-size: 12px;"
        "}"
        "QStatusBar QLabel { color: #FFFFFF; background: transparent; }"

        /* ---- Dock widgets ---- */
        "QDockWidget {"
        "  titlebar-close-icon: url(none);"
        "  color: #CCCCCC;"
        "}"
        "QDockWidget::title {"
        "  background-color: #252526;"
        "  color: #CCCCCC;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "  font-weight: bold;"
        "  border-bottom: 1px solid #1E1E1E;"
        "}"

        /* ---- Splitter handles ---- */
        "QSplitter::handle { background-color: #3C3C3C; }"

        /* ---- Scroll bars ---- */
        "QScrollBar:vertical, QScrollBar:horizontal {"
        "  background: #1E1E1E;"
        "  width: 10px; height: 10px;"
        "  border: none;"
        "}"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "  background: #424242;"
        "  border-radius: 4px;"
        "  min-height: 20px; min-width: 20px;"
        "}"
        "QScrollBar::handle:hover { background: #686868; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"

        /* ---- Message box ---- */
        "QMessageBox { background-color: #252526; color: #CCCCCC; }"
        "QMessageBox QPushButton {"
        "  background-color: #0E639C;"
        "  color: #FFFFFF;"
        "  border: none;"
        "  padding: 5px 16px;"
        "  border-radius: 3px;"
        "}"
        "QMessageBox QPushButton:hover { background-color: #1177BB; }"

        /* ---- Dialog ---- */
        "QDialog { background-color: #252526; color: #CCCCCC; }"
        "QLabel { color: #CCCCCC; background: transparent; }"
        "QLineEdit, QSpinBox, QComboBox {"
        "  background-color: #3C3C3C;"
        "  color: #CCCCCC;"
        "  border: 1px solid #555555;"
        "  padding: 3px 6px;"
        "  border-radius: 2px;"
        "}"
        "QCheckBox { color: #CCCCCC; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid #555555;"
        "  background: #3C3C3C;"
        "}"
        "QCheckBox::indicator:checked { background: #007ACC; }"
        "QDialogButtonBox QPushButton {"
        "  background-color: #0E639C;"
        "  color: #FFFFFF;"
        "  border: none;"
        "  padding: 5px 16px;"
        "  border-radius: 3px;"
        "}"
        "QDialogButtonBox QPushButton:hover { background-color: #1177BB; }"
    );
}

QString XsMainWindow::radianceStyleSheet() const
{
    return QStringLiteral(
        "QMainWindow { background-color: #F5F5F5; }"
        "QMenuBar { background-color: #F3F3F3; color: #333333; border-bottom: 1px solid #DCDCDC; }"
        "QMenuBar::item:selected { background-color: #E8E8E8; }"
        "QMenu { background-color: #FFFFFF; color: #333333; border: 1px solid #DCDCDC; }"
        "QMenu::item:selected { background-color: #0060C0; color: #FFFFFF; }"
        "QToolBar { background-color: #F3F3F3; border: none; spacing: 3px; padding: 2px 4px; }"
        "QToolButton { color: #333333; background: transparent; border: 1px solid transparent; border-radius: 3px; padding: 3px 8px; }"
        "QToolButton:hover { background-color: #E8E8E8; }"
        "QStatusBar { background-color: #007ACC; color: #FFFFFF; }"
        "QStatusBar QLabel { color: #FFFFFF; }"
        "QDockWidget::title { background-color: #F3F3F3; color: #424242; padding: 4px 8px; border-bottom: 1px solid #DCDCDC; }"
        "QScrollBar:vertical, QScrollBar:horizontal { background: #F5F5F5; width: 10px; height: 10px; }"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal { background: #C0C0C0; border-radius: 4px; }"
        "QDialog { background-color: #FFFFFF; color: #333333; }"
        "QLabel { color: #333333; }"
        "QLineEdit, QSpinBox, QComboBox { background-color: #FFFFFF; color: #333333; border: 1px solid #CCCCCC; padding: 3px 6px; }"
        "QCheckBox { color: #333333; }"
        "QDialogButtonBox QPushButton { background-color: #0060C0; color: #FFFFFF; border: none; padding: 5px 16px; border-radius: 3px; }"
    );
}

/* =========================================================================
 * Preferences
 * ========================================================================= */

void XsMainWindow::loadPreferences()
{
    QSettings s(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                + QStringLiteral("/preferences.ini"),
                QSettings::IniFormat);

    m_prefs.themeName            = s.value(QStringLiteral("theme"),         m_prefs.themeName).toString();
    m_prefs.fontSize             = s.value(QStringLiteral("font_size"),     m_prefs.fontSize).toInt();
    m_prefs.fontFamily           = s.value(QStringLiteral("font_family"),   m_prefs.fontFamily).toString();
    m_prefs.tabWidth             = s.value(QStringLiteral("tab_width"),     m_prefs.tabWidth).toInt();
    m_prefs.showLineNumbers      = s.value(QStringLiteral("line_numbers"),  m_prefs.showLineNumbers).toBool();
    m_prefs.autoIndent           = s.value(QStringLiteral("auto_indent"),   m_prefs.autoIndent).toBool();
    m_prefs.bracketMatching      = s.value(QStringLiteral("bracket_match"), m_prefs.bracketMatching).toBool();
    m_prefs.wordWrap             = s.value(QStringLiteral("word_wrap"),     m_prefs.wordWrap).toBool();
    m_prefs.useSpaces            = s.value(QStringLiteral("use_spaces"),    m_prefs.useSpaces).toBool();
    m_prefs.autoSave             = s.value(QStringLiteral("auto_save"),     m_prefs.autoSave).toBool();
    m_prefs.autoSaveIntervalSec  = s.value(QStringLiteral("auto_save_sec"), m_prefs.autoSaveIntervalSec).toInt();
    m_prefs.buildCommand         = s.value(QStringLiteral("build_cmd"),     m_prefs.buildCommand).toString();
    m_prefs.runCommand           = s.value(QStringLiteral("run_cmd"),       m_prefs.runCommand).toString();

    /* Apply loaded preferences to editor */
    m_editor->setTabWidth(m_prefs.tabWidth);
    m_editor->setAutoIndent(m_prefs.autoIndent);
    m_editor->setBracketMatching(m_prefs.bracketMatching);
    m_editor->setHighlightCurrentLine(m_prefs.highlightCurrentLine);

    if (m_prefs.autoSave && m_prefs.autoSaveIntervalSec > 0) {
        m_autoSaveTimer->start(m_prefs.autoSaveIntervalSec * 1000);
    }

    /* Recent files */
    m_recentFiles = s.value(QStringLiteral("recent_files")).toStringList();
    rebuildRecentMenu();

    /* Window geometry */
    if (s.contains(QStringLiteral("geometry")))
        restoreGeometry(s.value(QStringLiteral("geometry")).toByteArray());
    if (s.contains(QStringLiteral("state")))
        restoreState(s.value(QStringLiteral("state")).toByteArray());
}

void XsMainWindow::savePreferences()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);

    QSettings s(dir + QStringLiteral("/preferences.ini"), QSettings::IniFormat);

    s.setValue(QStringLiteral("theme"),         m_prefs.themeName);
    s.setValue(QStringLiteral("font_size"),     m_prefs.fontSize);
    s.setValue(QStringLiteral("font_family"),   m_prefs.fontFamily);
    s.setValue(QStringLiteral("tab_width"),     m_prefs.tabWidth);
    s.setValue(QStringLiteral("line_numbers"),  m_prefs.showLineNumbers);
    s.setValue(QStringLiteral("auto_indent"),   m_prefs.autoIndent);
    s.setValue(QStringLiteral("bracket_match"), m_prefs.bracketMatching);
    s.setValue(QStringLiteral("word_wrap"),     m_prefs.wordWrap);
    s.setValue(QStringLiteral("use_spaces"),    m_prefs.useSpaces);
    s.setValue(QStringLiteral("auto_save"),     m_prefs.autoSave);
    s.setValue(QStringLiteral("auto_save_sec"), m_prefs.autoSaveIntervalSec);
    s.setValue(QStringLiteral("build_cmd"),     m_prefs.buildCommand);
    s.setValue(QStringLiteral("run_cmd"),       m_prefs.runCommand);
    s.setValue(QStringLiteral("recent_files"),  m_recentFiles);
    s.setValue(QStringLiteral("geometry"),      saveGeometry());
    s.setValue(QStringLiteral("state"),         saveState());
}

void XsMainWindow::showPreferencesDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Preferences"));
    dlg.setMinimumWidth(440);

    QVBoxLayout *mainVL = new QVBoxLayout(&dlg);

    /* ---- Editor group ---- */
    QGroupBox   *editorBox = new QGroupBox(QStringLiteral("Editor"), &dlg);
    QFormLayout *editorFL  = new QFormLayout(editorBox);

    QSpinBox *fontSizeSpin = new QSpinBox(&dlg);
    fontSizeSpin->setRange(6, 36);
    fontSizeSpin->setValue(m_prefs.fontSize);

    QLineEdit *fontFamilyEdit = new QLineEdit(m_prefs.fontFamily, &dlg);

    QSpinBox *tabWidthSpin = new QSpinBox(&dlg);
    tabWidthSpin->setRange(1, 16);
    tabWidthSpin->setValue(m_prefs.tabWidth);

    QCheckBox *autoIndentCheck  = new QCheckBox(QStringLiteral("Auto-indent"),       &dlg);
    QCheckBox *bracketCheck     = new QCheckBox(QStringLiteral("Bracket matching"),   &dlg);
    QCheckBox *hlLineCheck      = new QCheckBox(QStringLiteral("Highlight current line"), &dlg);
    QCheckBox *wordWrapCheck    = new QCheckBox(QStringLiteral("Word wrap"),           &dlg);
    QCheckBox *useSpacesCheck   = new QCheckBox(QStringLiteral("Use spaces for tab"), &dlg);

    autoIndentCheck->setChecked(m_prefs.autoIndent);
    bracketCheck->setChecked(m_prefs.bracketMatching);
    hlLineCheck->setChecked(m_prefs.highlightCurrentLine);
    wordWrapCheck->setChecked(m_prefs.wordWrap);
    useSpacesCheck->setChecked(m_prefs.useSpaces);

    editorFL->addRow(QStringLiteral("Font family:"), fontFamilyEdit);
    editorFL->addRow(QStringLiteral("Font size:"),   fontSizeSpin);
    editorFL->addRow(QStringLiteral("Tab width:"),   tabWidthSpin);
    editorFL->addRow(autoIndentCheck);
    editorFL->addRow(bracketCheck);
    editorFL->addRow(hlLineCheck);
    editorFL->addRow(wordWrapCheck);
    editorFL->addRow(useSpacesCheck);

    /* ---- Build group ---- */
    QGroupBox   *buildBox = new QGroupBox(QStringLiteral("Build && Run"), &dlg);
    QFormLayout *buildFL  = new QFormLayout(buildBox);

    QLineEdit *buildCmdEdit = new QLineEdit(m_prefs.buildCommand, &dlg);
    QLineEdit *runCmdEdit   = new QLineEdit(m_prefs.runCommand,   &dlg);

    QCheckBox *autoSaveCheck = new QCheckBox(QStringLiteral("Auto-save before build"), &dlg);
    autoSaveCheck->setChecked(m_prefs.autoSave);

    QSpinBox *autoSaveSpin = new QSpinBox(&dlg);
    autoSaveSpin->setRange(5, 600);
    autoSaveSpin->setSuffix(QStringLiteral(" s"));
    autoSaveSpin->setValue(m_prefs.autoSaveIntervalSec);

    buildFL->addRow(QStringLiteral("Build command:"), buildCmdEdit);
    buildFL->addRow(QStringLiteral("Run command:"),   runCmdEdit);
    buildFL->addRow(autoSaveCheck);
    buildFL->addRow(QStringLiteral("Auto-save interval:"), autoSaveSpin);

    /* ---- Theme group ---- */
    QGroupBox   *themeBox = new QGroupBox(QStringLiteral("Appearance"), &dlg);
    QFormLayout *themeFL  = new QFormLayout(themeBox);

    QComboBox *themeCombo = new QComboBox(&dlg);
    themeCombo->addItems({ QStringLiteral("Obsidian"), QStringLiteral("Radiance") });
    themeCombo->setCurrentText(m_prefs.themeName);
    themeFL->addRow(QStringLiteral("Theme:"), themeCombo);

    /* ---- Buttons ---- */
    QDialogButtonBox *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    mainVL->addWidget(editorBox);
    mainVL->addWidget(buildBox);
    mainVL->addWidget(themeBox);
    mainVL->addWidget(btns);

    if (dlg.exec() == QDialog::Accepted) {
        m_prefs.fontSize             = fontSizeSpin->value();
        m_prefs.fontFamily           = fontFamilyEdit->text();
        m_prefs.tabWidth             = tabWidthSpin->value();
        m_prefs.autoIndent           = autoIndentCheck->isChecked();
        m_prefs.bracketMatching      = bracketCheck->isChecked();
        m_prefs.highlightCurrentLine = hlLineCheck->isChecked();
        m_prefs.wordWrap             = wordWrapCheck->isChecked();
        m_prefs.useSpaces            = useSpacesCheck->isChecked();
        m_prefs.buildCommand         = buildCmdEdit->text();
        m_prefs.runCommand           = runCmdEdit->text();
        m_prefs.autoSave             = autoSaveCheck->isChecked();
        m_prefs.autoSaveIntervalSec  = autoSaveSpin->value();

        const QString newTheme = themeCombo->currentText();
        if (newTheme != m_prefs.themeName) {
            m_prefs.themeName = newTheme;
            if (newTheme == QStringLiteral("Radiance"))
                applyRadianceTheme();
            else
                applyObsidianTheme();
        }

        /* Forward settings to editor */
        m_editor->setTabWidth(m_prefs.tabWidth);
        m_editor->setAutoIndent(m_prefs.autoIndent);
        m_editor->setBracketMatching(m_prefs.bracketMatching);
        m_editor->setHighlightCurrentLine(m_prefs.highlightCurrentLine);

        /* Auto-save timer */
        if (m_prefs.autoSave && m_prefs.autoSaveIntervalSec > 0)
            m_autoSaveTimer->start(m_prefs.autoSaveIntervalSec * 1000);
        else
            m_autoSaveTimer->stop();

        savePreferences();
    }
}

/* =========================================================================
 * Recent Files
 * ========================================================================= */

void XsMainWindow::addRecentFile(const QString &path)
{
    m_recentFiles.removeAll(path);
    m_recentFiles.prepend(path);
    while (m_recentFiles.size() > MaxRecentFiles)
        m_recentFiles.removeLast();
    rebuildRecentMenu();
}

void XsMainWindow::rebuildRecentMenu()
{
    if (!m_recentMenu) return;
    m_recentMenu->clear();

    if (m_recentFiles.isEmpty()) {
        m_recentMenu->addAction(QStringLiteral("(no recent files)"))->setEnabled(false);
        return;
    }

    for (const QString &path : m_recentFiles) {
        QAction *act = m_recentMenu->addAction(QFileInfo(path).fileName());
        act->setToolTip(path);
        connect(act, &QAction::triggered, this, [this, path]() {
            m_editor->openFile(path);
            updateWindowTitle(QFileInfo(path).fileName());
        });
    }

    m_recentMenu->addSeparator();
    QAction *clearAct = m_recentMenu->addAction(QStringLiteral("Clear Recent Files"));
    connect(clearAct, &QAction::triggered, this, [this]() {
        m_recentFiles.clear();
        rebuildRecentMenu();
    });
}
