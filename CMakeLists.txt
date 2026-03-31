cmake_minimum_required(VERSION 3.16)
project(xsharp VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(XS_ENABLE_GPU  "Enable OpenCL GPU compute" ON)
option(XS_ENABLE_IDE  "Build the native GTK3 IDE" ON)
option(XS_BUILD_TESTS "Build test suite" OFF)
option(XS_ENABLE_AUDIO "Enable audio subsystem" ON)

# Compiler flags
add_compile_options(-Wall -Wextra -Wpedantic)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
else()
    add_compile_options(-O2)
endif()

# Core source files
file(GLOB_RECURSE CORE_SOURCES
    src/lexer/*.c
    src/parser/*.c
    src/ast/*.c
    src/compiler/*.c
    src/codegen/*.c
    src/vm/*.c
    src/runtime/*.c
    src/debugger/*.c
    src/cli/*.c
    src/formats/*.c
    src/renderer/core/*.c
    src/renderer/gpu/*.c
    src/renderer/shaders/*.c
    src/renderer/scene/*.c
)

# Stdlib sources
file(GLOB_RECURSE STDLIB_SOURCES
    stdlib/**/*.c
)

# Main executable
add_executable(xsharp src/main.c ${CORE_SOURCES} ${STDLIB_SOURCES})
target_include_directories(xsharp PRIVATE src/ stdlib/)
target_link_libraries(xsharp m pthread)

# Readline (optional)
find_library(READLINE_LIB readline)
if(READLINE_LIB)
    target_link_libraries(xsharp ${READLINE_LIB})
    target_compile_definitions(xsharp PRIVATE XS_HAVE_READLINE)
endif()

# GPU support (OpenCL)
if(XS_ENABLE_GPU)
    find_package(OpenCL QUIET)
    if(OpenCL_FOUND)
        target_link_libraries(xsharp OpenCL::OpenCL)
        target_compile_definitions(xsharp PRIVATE XS_ENABLE_GPU)
    else()
        message(STATUS "OpenCL not found - GPU compute disabled")
    endif()
endif()

# IDE (GTK3)
if(XS_ENABLE_IDE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GTK3 gtk+-3.0)
        pkg_check_modules(GTKSV gtksourceview-3.0)
        if(GTK3_FOUND AND GTKSV_FOUND)
            file(GLOB_RECURSE IDE_SOURCES src/ide/**/*.c)
            add_executable(xsharp-ide src/ide/core/ide_app.c ${IDE_SOURCES}
                ${CORE_SOURCES} ${STDLIB_SOURCES})
            target_include_directories(xsharp-ide PRIVATE
                src/ stdlib/ ${GTK3_INCLUDE_DIRS} ${GTKSV_INCLUDE_DIRS})
            target_link_libraries(xsharp-ide
                ${GTK3_LIBRARIES} ${GTKSV_LIBRARIES} m pthread)
            target_compile_definitions(xsharp-ide PRIVATE XS_ENABLE_IDE)
        else()
            message(STATUS "GTK3/GtkSourceView not found - IDE disabled")
        endif()
    endif()
endif()

# Audio
if(XS_ENABLE_AUDIO)
    find_library(ALSA_LIB asound)
    if(ALSA_LIB)
        target_link_libraries(xsharp ${ALSA_LIB})
        target_compile_definitions(xsharp PRIVATE XS_ENABLE_AUDIO)
    endif()
endif()

# Tests
if(XS_BUILD_TESTS)
    enable_testing()
    file(GLOB_RECURSE TEST_SOURCES tests/**/*.c)
    add_executable(xsharp-tests tests/test_main.c ${TEST_SOURCES}
        ${CORE_SOURCES} ${STDLIB_SOURCES})
    target_include_directories(xsharp-tests PRIVATE src/ stdlib/ tests/)
    target_link_libraries(xsharp-tests m pthread)
    add_test(NAME xsharp-tests COMMAND xsharp-tests)
endif()

# Install
install(TARGETS xsharp DESTINATION bin)
install(DIRECTORY stdlib/ DESTINATION share/xsharp/stdlib)
install(DIRECTORY examples/ DESTINATION share/xsharp/examples)
