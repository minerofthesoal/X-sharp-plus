##############################################################################
# X# IDE - Qt6 qmake Project File
##############################################################################
#
# Alternative build method using qmake:
#   qmake qt6_ide.pro
#   make -j$(nproc)
#
# Or with a specific Qt6 installation:
#   /path/to/qt6/bin/qmake qt6_ide.pro
#   make -j$(nproc)
#
##############################################################################

QT       += core gui widgets

# Require Qt 6
lessThan(QT_MAJOR_VERSION, 6): error("X# IDE requires Qt 6")

CONFIG   += c++17
CONFIG   -= app_bundle   # Remove for macOS bundle builds

# Silence some Qt deprecation warnings
DEFINES  += QT_DISABLE_DEPRECATED_BEFORE=0x060200

TARGET   = xsharp-ide
TEMPLATE = app

#------------------------------------------------------------------------------
# Source files
#------------------------------------------------------------------------------
SOURCES += \
    main.cpp           \
    mainwindow.cpp     \
    editor.cpp         \
    xsharp_highlighter.cpp \
    projectpanel.cpp   \
    consolepanel.cpp   \
    debugpanel.cpp

HEADERS += \
    mainwindow.h       \
    editor.h           \
    xsharp_highlighter.h \
    projectpanel.h     \
    consolepanel.h     \
    debugpanel.h

# Optional: resource file
# RESOURCES += resources.qrc

#------------------------------------------------------------------------------
# Compiler flags
#------------------------------------------------------------------------------
unix {
    QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic -Wno-unused-parameter
}
win32-msvc* {
    QMAKE_CXXFLAGS += /W4 /wd4100
}

#------------------------------------------------------------------------------
# Build output directories
#------------------------------------------------------------------------------
OBJECTS_DIR = .build/obj
MOC_DIR     = .build/moc
RCC_DIR     = .build/rcc
UI_DIR      = .build/ui
DESTDIR     = .

#------------------------------------------------------------------------------
# Default install path (Linux)
#------------------------------------------------------------------------------
unix:!macx {
    target.path    = /usr/local/bin
    INSTALLS       += target
}

#------------------------------------------------------------------------------
# macOS bundle settings
#------------------------------------------------------------------------------
macx {
    QMAKE_INFO_PLIST = Info.plist
    ICON             = ../../assets/xsharp-ide.icns
}

#------------------------------------------------------------------------------
# Windows RC file (version info / icon)
#------------------------------------------------------------------------------
win32 {
    RC_ICONS = ../../assets/xsharp-ide.ico
    VERSION  = 1.0.0.0
    QMAKE_TARGET_COMPANY     = "XSharp"
    QMAKE_TARGET_PRODUCT     = "X# IDE"
    QMAKE_TARGET_DESCRIPTION = "Native IDE for the X# programming language"
    QMAKE_TARGET_COPYRIGHT   = "Copyright (C) 2025 XSharp Project"
}
