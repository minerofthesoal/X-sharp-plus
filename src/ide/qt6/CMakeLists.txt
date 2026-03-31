##############################################################################
# X# IDE - Qt6 CMake Build File
##############################################################################
#
# Requirements:
#   - CMake >= 3.22
#   - Qt 6.2+ (Core, Gui, Widgets)
#   - A C++17-capable compiler
#
# Build (out-of-source):
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j$(nproc)
#   ./build/xsharp-ide
#
##############################################################################

cmake_minimum_required(VERSION 3.22)

project(XSharpIDE
    VERSION     1.0.0
    DESCRIPTION "Native Qt6 IDE for the X# programming language"
    LANGUAGES   CXX
)

#------------------------------------------------------------------------------
# C++ Standard
#------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

#------------------------------------------------------------------------------
# Qt6 Configuration
#------------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)   # auto-run moc on Q_OBJECT classes
set(CMAKE_AUTORCC ON)   # auto-process .qrc resource files
set(CMAKE_AUTOUIC ON)   # auto-run uic on .ui files (if any)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

#------------------------------------------------------------------------------
# Source Files
#------------------------------------------------------------------------------
set(IDE_SOURCES
    main.cpp

    mainwindow.cpp
    mainwindow.h

    editor.cpp
    editor.h

    xsharp_highlighter.cpp
    xsharp_highlighter.h

    projectpanel.cpp
    projectpanel.h

    consolepanel.cpp
    consolepanel.h

    debugpanel.cpp
    debugpanel.h
)

# Optional: resource file for icons / fonts
# list(APPEND IDE_SOURCES resources.qrc)

#------------------------------------------------------------------------------
# Executable
#------------------------------------------------------------------------------
if(WIN32)
    # Windows: GUI subsystem (no console window)
    qt_add_executable(xsharp-ide WIN32 ${IDE_SOURCES})
elseif(APPLE)
    # macOS: bundle
    qt_add_executable(xsharp-ide MACOSX_BUNDLE ${IDE_SOURCES})
    set_target_properties(xsharp-ide PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER  "org.xsharp.ide"
        MACOSX_BUNDLE_BUNDLE_VERSION  "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
else()
    qt_add_executable(xsharp-ide ${IDE_SOURCES})
endif()

#------------------------------------------------------------------------------
# Link Qt6 modules
#------------------------------------------------------------------------------
target_link_libraries(xsharp-ide
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

#------------------------------------------------------------------------------
# Include paths
#------------------------------------------------------------------------------
target_include_directories(xsharp-ide PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#------------------------------------------------------------------------------
# Compiler warnings
#------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(xsharp-ide PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
elseif(MSVC)
    target_compile_options(xsharp-ide PRIVATE /W4 /wd4100)
endif()

#------------------------------------------------------------------------------
# Install rules (Linux / macOS)
#------------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS xsharp-ide
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Optional desktop integration (Linux)
if(UNIX AND NOT APPLE)
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../assets/xsharp-ide.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
        OPTIONAL
    )
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../assets/xsharp-ide.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        OPTIONAL
    )
endif()

#------------------------------------------------------------------------------
# CPack (optional packaging)
#------------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME              "xsharp-ide")
set(CPACK_PACKAGE_VERSION           "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SHORT "X# IDE – Qt6 edition")
set(CPACK_PACKAGE_CONTACT           "dev@xsharp.org")
include(CPack)

#------------------------------------------------------------------------------
# Status summary
#------------------------------------------------------------------------------
message(STATUS "----------------------------------------------------")
message(STATUS "  Project     : ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Qt version  : ${Qt6_VERSION}")
message(STATUS "  Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install dir : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "----------------------------------------------------")
