/*
 * X# IDE (Qt6) - Main Window
 * ============================
 * QMainWindow-based IDE shell with:
 *   - Menu bar  : File | Edit | View | Build | Debug | Help
 *   - Toolbar   : New | Open | Save | --- | Build | Run | Debug
 *   - Status bar: cursor position, file name, language, encoding
 *   - Dockable panels: Project (left), Console & Debug (bottom)
 *   - Central editor with multi-tab support
 *   - Preferences loading/saving (~/.config/xsharp-ide/preferences.ini)
 *   - Recent files tracking (up to 20 entries)
 *   - Obsidian (dark) and Radiance (light) themes
 */

#pragma once

#include <QAction>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QProcess>
#include <QSettings>
#include <QString>
#include <QTimer>

class XsEditor;
class XsProjectPanel;
class XsConsolePanel;
class XsDebugPanel;

/* ===================================================================
 * IDE Preferences
 * =================================================================== */
struct XsIdePrefs {
    QString themeName = QStringLiteral("Obsidian");
    int fontSize = 12;
    QString fontFamily = QStringLiteral("Monospace");
    bool showLineNumbers = true;
    bool showMinimap = false;
    bool autoIndent = true;
    bool highlightCurrentLine = true;
    bool bracketMatching = true;
    bool wordWrap = false;
    int tabWidth = 4;
    bool useSpaces = true;
    bool autoSave = false;
    int autoSaveIntervalSec = 30;
    QString buildCommand = QStringLiteral("xsharp build");
    QString runCommand = QStringLiteral("xsharp run");
};

/* ===================================================================
 * XsMainWindow
 * =================================================================== */
class XsMainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit XsMainWindow(QWidget* parent = nullptr);
    ~XsMainWindow() override;

  protected:
    void closeEvent(QCloseEvent* event) override;

  private slots:
    /* File menu */
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveFileAs();
    void onOpenProject();
    void onCloseFile();

    /* Edit menu */
    void onUndo();
    void onRedo();
    void onFind();
    void onCut();
    void onCopy();
    void onPaste();

    /* View menu */
    void onToggleTheme();
    void onToggleProjectPanel();
    void onToggleConsolePanel();
    void onToggleDebugPanel();
    void onPreferences();

    /* Build menu */
    void onBuild();
    void onRun();
    void onStop();

    /* Debug menu */
    void onDebugStart();
    void onDebugStop();
    void onDebugContinue();
    void onDebugStepOver();
    void onDebugStepIn();
    void onDebugStepOut();

    /* Help menu */
    void onAbout();

    /* Internal */
    void onCursorMoved(int line, int col);
    void onFileChanged(const QString& path);
    void onBuildProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onBuildProcessOutput();
    void onAutoSaveTick();

  private:
    /* Components */
    XsEditor* m_editor;
    XsProjectPanel* m_projectPanel;
    XsConsolePanel* m_consolePanel;
    XsDebugPanel* m_debugPanel;

    /* Dock widgets */
    QDockWidget* m_projectDock;
    QDockWidget* m_consoleDock;
    QDockWidget* m_debugDock;

    /* Status bar labels */
    QLabel* m_statusCursor;
    QLabel* m_statusFile;
    QLabel* m_statusLanguage;
    QLabel* m_statusEncoding;

    /* State */
    XsIdePrefs m_prefs;
    bool m_isDebugging = false;
    QProcess* m_buildProcess = nullptr;

    /* Auto-save timer */
    QTimer* m_autoSaveTimer;

    /* Recent files */
    QStringList m_recentFiles;
    QMenu* m_recentMenu = nullptr;
    static constexpr int MaxRecentFiles = 20;

    /* Build helpers */
    void buildMenus();
    void buildToolbar();
    void buildStatusBar();
    void buildDockPanels();

    void updateWindowTitle(const QString& filename = QString());
    void updateStatusBar(int line, int col, const QString& file,
                         const QString& encoding = QStringLiteral("UTF-8"));

    /* Theme application */
    void applyObsidianTheme();
    void applyRadianceTheme();
    QString obsidianStyleSheet() const;
    QString radianceStyleSheet() const;

    /* Preferences */
    void loadPreferences();
    void savePreferences();
    void showPreferencesDialog();

    /* Recent files */
    void addRecentFile(const QString& path);
    void rebuildRecentMenu();
};
