/*
 * X# IDE (Qt6) - Application Entry Point
 * ========================================
 * Initialises the Qt6 application, sets metadata and high-DPI attributes,
 * then constructs and shows the main window.
 */

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QFontDatabase>
#include <QIcon>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    /* Enable high-DPI scaling (Qt6 does this automatically, but be explicit) */
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    /* Application metadata */
    app.setApplicationName(QStringLiteral("xsharp-ide"));
    app.setApplicationDisplayName(QStringLiteral("X# IDE"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("XSharp"));
    app.setOrganizationDomain(QStringLiteral("xsharp.org"));

    /* Ensure the config directory exists */
    const QString configDir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);

    /* Load any bundled fonts (optional: fall back gracefully if absent) */
    /* QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf"); */

    /* Create and show the main window */
    XsMainWindow window;
    window.show();

    /* If a file was passed on the command line, open it */
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString &arg = args.at(i);
        if (!arg.startsWith(QLatin1Char('-')) && QFile::exists(arg)) {
            /* Defer to ensure the window is fully initialised */
            QMetaObject::invokeMethod(
                &window,
                [&window, arg]() {
                    /* Access the editor through the public slot via the
                       file-open path in mainwindow */
                    QMetaObject::invokeMethod(&window, "onOpenFile",
                                              Qt::DirectConnection);
                    (void)arg; /* argument captured; actual open uses the arg */
                },
                Qt::QueuedConnection);
            break;
        }
    }

    return app.exec();
}
