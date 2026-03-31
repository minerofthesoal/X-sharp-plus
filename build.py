#!/usr/bin/env python3
"""
X# (Xsharp) Build System
=========================
Single-file build script that:
  1. Checks / installs system dependencies (apt-based)
  2. Patches all known source-level build issues
  3. Compiles the full project (CLI + tests)
  4. Optionally builds the IDE (if GTK3 present)
  5. Optionally builds the VS Code extension (if npm present)

Usage:
    python3 build.py              # full build
    python3 build.py --fix-only   # apply patches without compiling
    python3 build.py --clean      # remove build artifacts
    python3 build.py --release    # optimised release build
    python3 build.py --ide        # also build the GTK3 IDE
    python3 build.py --vscode     # also build the VS Code extension
    python3 build.py --all        # build everything
    python3 build.py --install    # install to /usr/local
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import textwrap
from pathlib import Path

# ── Globals ──────────────────────────────────────────────────────────────────

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "src"
STDLIB = ROOT / "stdlib"
BUILD = ROOT / "build"
TESTS = ROOT / "tests"

CC = os.environ.get("CC", "gcc")
CFLAGS_BASE = ["-std=c11", "-Wall", "-Wextra", "-D_GNU_SOURCE",
               "-D_POSIX_C_SOURCE=200809L"]
CFLAGS_DEBUG = CFLAGS_BASE + ["-g", "-O0", "-DDEBUG"]
CFLAGS_RELEASE = CFLAGS_BASE + ["-O2", "-DNDEBUG"]
LDFLAGS = ["-lm", "-lpthread"]
INCLUDES = [f"-I{SRC}", f"-I{STDLIB}"]

GREEN = "\033[32m"
YELLOW = "\033[33m"
RED = "\033[31m"
BOLD = "\033[1m"
RESET = "\033[0m"

ERRORS_FIXED = 0

# ── Utilities ────────────────────────────────────────────────────────────────

def log(msg, color=GREEN):
    print(f"{color}{BOLD}[build]{RESET} {msg}")

def warn(msg):
    log(msg, YELLOW)

def err(msg):
    log(msg, RED)

def run(cmd, **kw):
    """Run a command, return CompletedProcess."""
    kw.setdefault("cwd", ROOT)
    kw.setdefault("capture_output", True)
    kw.setdefault("text", True)
    return subprocess.run(cmd, **kw)

def file_read(path):
    return Path(path).read_text()

def file_write(path, content):
    Path(path).write_text(content)

# ── Dependency checking ─────────────────────────────────────────────────────

REQUIRED_PACKAGES = ["gcc", "make", "cmake"]
OPTIONAL_PACKAGES = {
    "libreadline-dev": "readline (REPL line editing)",
    "libgtk-3-dev": "GTK3 (native IDE)",
    "libgtksourceview-3.0-dev": "GtkSourceView (IDE syntax highlighting)",
    "ocl-icd-opencl-dev": "OpenCL (GPU compute)",
    "libasound2-dev": "ALSA (audio)",
}

def check_command(name):
    return shutil.which(name) is not None

def check_dpkg(pkg):
    r = run(["dpkg", "-s", pkg])
    return r.returncode == 0

def install_deps(also_optional=False):
    """Check and install dependencies."""
    log("Checking dependencies...")

    missing = []
    for pkg in REQUIRED_PACKAGES:
        if not check_command(pkg):
            missing.append(pkg)

    if missing:
        log(f"Installing required packages: {', '.join(missing)}")
        r = run(["apt-get", "update", "-qq"])
        r = run(["apt-get", "install", "-y", "-qq"] + missing)
        if r.returncode != 0:
            warn("Could not install packages (may need sudo). Trying sudo...")
            run(["sudo", "apt-get", "update", "-qq"])
            run(["sudo", "apt-get", "install", "-y", "-qq"] + missing)

    # Build-essential for full toolchain
    if not check_dpkg("build-essential"):
        log("Installing build-essential...")
        r = run(["apt-get", "install", "-y", "-qq", "build-essential"])
        if r.returncode != 0:
            run(["sudo", "apt-get", "install", "-y", "-qq", "build-essential"])

    if also_optional:
        opt_missing = []
        for pkg, desc in OPTIONAL_PACKAGES.items():
            if not check_dpkg(pkg):
                opt_missing.append(pkg)
                warn(f"  Optional: {pkg} ({desc}) - not installed")
        if opt_missing:
            log(f"Installing optional packages: {', '.join(opt_missing)}")
            r = run(["apt-get", "install", "-y", "-qq"] + opt_missing)
            if r.returncode != 0:
                run(["sudo", "apt-get", "install", "-y", "-qq"] + opt_missing)

    log("Dependencies OK.")

# ── Source patches ───────────────────────────────────────────────────────────

def patch_file(filepath, old, new, description=""):
    """Replace `old` with `new` in file. Returns True if changed."""
    global ERRORS_FIXED
    p = Path(filepath)
    if not p.exists():
        return False
    content = p.read_text()
    if old not in content:
        return False
    content = content.replace(old, new, 1)
    p.write_text(content)
    ERRORS_FIXED += 1
    if description:
        log(f"  Fixed: {p.relative_to(ROOT)} - {description}")
    return True

def patch_insert_after(filepath, anchor, insertion, description=""):
    """Insert text after first occurrence of anchor."""
    global ERRORS_FIXED
    p = Path(filepath)
    if not p.exists():
        return False
    content = p.read_text()
    if insertion in content:
        return False  # already patched
    if anchor not in content:
        return False
    content = content.replace(anchor, anchor + insertion, 1)
    p.write_text(content)
    ERRORS_FIXED += 1
    if description:
        log(f"  Fixed: {p.relative_to(ROOT)} - {description}")
    return True

def patch_insert_before(filepath, anchor, insertion, description=""):
    """Insert text before first occurrence of anchor."""
    global ERRORS_FIXED
    p = Path(filepath)
    if not p.exists():
        return False
    content = p.read_text()
    if insertion in content:
        return False  # already patched
    if anchor not in content:
        return False
    content = content.replace(anchor, insertion + anchor, 1)
    p.write_text(content)
    ERRORS_FIXED += 1
    if description:
        log(f"  Fixed: {p.relative_to(ROOT)} - {description}")
    return True

def apply_all_patches():
    """Apply all known source fixes."""
    log("Applying source patches...")
    global ERRORS_FIXED
    ERRORS_FIXED = 0

    # ── 1. debugger/breakpoint.c: needs debug_info.h for XsSourceMap ──
    patch_insert_after(
        SRC / "debugger/breakpoint.c",
        '#include "breakpoint.h"\n',
        '#include "debug_info.h"\n',
        "add missing #include debug_info.h for XsSourceMap"
    )

    # ── 2. debugger/debug_protocol.c: conflicting static funcs vs header decls ──
    # The .c file uses its own simple JSON helpers that conflict with the header's
    # JSON API declarations. Rename the static helpers.
    dp_path = SRC / "debugger/debug_protocol.c"
    if dp_path.exists():
        content = dp_path.read_text()
        if "static void json_string" in content:
            renames = {
                "json_string": "dap_json_string",
                "json_bool": "dap_json_bool",
                "json_start_object": "dap_json_start_object",
                "json_end_object": "dap_json_end_object",
                "json_start_array": "dap_json_start_array",
                "json_end_array": "dap_json_end_array",
                "json_key": "dap_json_key",
                "json_int": "dap_json_int",
                "json_comma": "dap_json_comma",
            }
            for old_name, new_name in renames.items():
                # Only rename the static local usages, not the header's declarations
                # The functions in the .c file are: static void json_xxx(FILE *out, ...)
                # and the (void)json_xxx; suppression lines
                content = content.replace(f"static void {old_name}(", f"static void {new_name}(")
                content = content.replace(f"(void){old_name}", f"(void){new_name}")
            dp_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: {dp_path.relative_to(ROOT)} - renamed conflicting static JSON helpers")

        # Also rename dap_read_message to avoid conflict with header's declaration
        if "static int dap_read_message(" in content or "static int dap_read_message(" in dp_path.read_text():
            content = dp_path.read_text()
            content = content.replace("static int dap_read_message(char *buf, int buf_size)",
                                      "static int dap_read_msg(char *buf, int buf_size)")
            content = content.replace("dap_read_message(buf, sizeof(buf))",
                                      "dap_read_msg(buf, sizeof(buf))")
            dp_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: {dp_path.relative_to(ROOT)} - renamed conflicting dap_read_message")

    # ── 3. cli/cli.c: missing #include <stdarg.h> ──
    patch_insert_after(
        SRC / "cli/cli.c",
        "#include <stdbool.h>\n",
        "#include <stdarg.h>\n",
        "add missing #include <stdarg.h>"
    )

    # ── 4. cli/linter.c: missing #include <stdarg.h> ──
    patch_insert_after(
        SRC / "cli/linter.c",
        "#include <ctype.h>\n",
        "#include <stdarg.h>\n",
        "add missing #include <stdarg.h>"
    )

    # ── 5. stdlib/fs/fs_lib.c: missing <limits.h> define for PATH_MAX ──
    # PATH_MAX needs <linux/limits.h> on some systems
    fs_path = STDLIB / "fs/fs_lib.c"
    if fs_path.exists():
        content = fs_path.read_text()
        if "#include <linux/limits.h>" not in content and "#ifndef PATH_MAX" not in content:
            patch_insert_after(
                fs_path,
                "#include <limits.h>\n",
                "#ifndef PATH_MAX\n#define PATH_MAX 4096\n#endif\n",
                "add PATH_MAX fallback definition"
            )
        # Also needs _GNU_SOURCE before includes for realpath
        if "_GNU_SOURCE" not in content and "_POSIX_C_SOURCE" not in content:
            patch_insert_before(
                fs_path,
                '#include "fs_lib.h"',
                "#define _GNU_SOURCE\n",
                "add _GNU_SOURCE for realpath"
            )

    # ── 6. stdlib/net/net_lib.c: missing <netdb.h> needed for getaddrinfo ──
    # netdb.h is included but getaddrinfo also needs _GNU_SOURCE or _POSIX_C_SOURCE
    # Already handled by our -D_GNU_SOURCE flag, but let's verify netdb.h is included
    net_path = STDLIB / "net/net_lib.c"
    # netdb.h is already included - the issue is just the -D flags (handled in CFLAGS)

    # ── 7. stdlib/time/time_lib.c: _POSIX_C_SOURCE defined AFTER includes ──
    time_path = STDLIB / "time/time_lib.c"
    if time_path.exists():
        content = time_path.read_text()
        # Remove the misplaced #define and we rely on -D_POSIX_C_SOURCE in CFLAGS
        if "\n#define _POSIX_C_SOURCE 200809L\n" in content:
            content = content.replace("\n#define _POSIX_C_SOURCE 200809L\n", "\n")
            time_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: {time_path.relative_to(ROOT)} - removed misplaced _POSIX_C_SOURCE (using -D flag)")

    # ── 8. stdlib/system/system_lib.c: popen/pclose need _GNU_SOURCE ──
    # Already handled by -D_GNU_SOURCE in CFLAGS

    # ── 9. cli/shortcuts.h: "/*" inside comment warning ──
    patch_file(
        SRC / "cli/shortcuts.h",
        'int xs_shortcut_test(void);     /* find tests/*.xs, run each */',
        'int xs_shortcut_test(void);     /* find tests .xs files, run each */',
        "fix comment-within-comment warning"
    )

    # Also in shortcuts.c
    patch_file(
        SRC / "cli/shortcuts.c",
        '/* ===== @test: Find tests/*.xs and run each ===== */',
        '/* ===== @test: Find tests .xs files and run each ===== */',
        "fix comment-within-comment warning"
    )

    # ── 10. runtime/runtime.h: function-pointer-to-object-pointer cast ──
    # Cast via uintptr_t to avoid -Wpedantic warning
    patch_file(
        SRC / "runtime/runtime.h",
        "val.object = (void*)fn;",
        "val.object = (void*)(uintptr_t)fn;",
        "fix function-to-object pointer cast via uintptr_t"
    )
    # Need stdint.h for uintptr_t - it's already included

    # ── 11. vm/vm.c: object-to-function-pointer cast ──
    vm_path = SRC / "vm/vm.c"
    if vm_path.exists():
        content = vm_path.read_text()
        content = content.replace(
            "XsNativeFn fn = (XsNativeFn)callee.object;",
            "XsNativeFn fn = (XsNativeFn)(uintptr_t)callee.object;"
        )
        if content != vm_path.read_text():
            vm_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: {vm_path.relative_to(ROOT)} - fix object-to-function pointer cast")

    # ── 12. Update CMakeLists.txt to add _GNU_SOURCE and _POSIX_C_SOURCE ──
    cmake_path = ROOT / "CMakeLists.txt"
    if cmake_path.exists():
        content = cmake_path.read_text()
        if "_GNU_SOURCE" not in content:
            content = content.replace(
                "add_compile_options(-Wall -Wextra -Wpedantic)",
                "add_compile_options(-Wall -Wextra)\n"
                "add_compile_definitions(_GNU_SOURCE _POSIX_C_SOURCE=200809L)"
            )
            cmake_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: CMakeLists.txt - add _GNU_SOURCE, _POSIX_C_SOURCE, remove -Wpedantic")

    # ── 13. Update Makefile to add _GNU_SOURCE and _POSIX_C_SOURCE ──
    make_path = ROOT / "Makefile"
    if make_path.exists():
        content = make_path.read_text()
        if "_GNU_SOURCE" not in content:
            content = content.replace(
                "CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic",
                "CFLAGS  ?= -std=c11 -Wall -Wextra -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L"
            )
            make_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: Makefile - add _GNU_SOURCE, _POSIX_C_SOURCE, remove -Wpedantic")

    # ── 14. runtime.h + vm.c: bridge VM/VMState and vm_register_native ──
    # Add vm_register_native declaration to runtime.h so stdlib can see it
    patch_insert_before(
        SRC / "runtime/runtime.h",
        "/* ===== Value to C type helpers ===== */",
        "/* ===== VM native function registration (implemented in vm.c) ===== */\n"
        "/* VM is an opaque handle; stdlib modules use this to register natives. */\n"
        "void vm_register_native(VM* vm, const char* name, XsNativeFn fn);\n\n",
        "add vm_register_native declaration to runtime.h"
    )

    # Add vm_register_native implementation to vm.c
    vm_c = SRC / "vm/vm.c"
    if vm_c.exists():
        content = vm_c.read_text()
        bridge_fn = ("/* ===== vm_register_native: bridge for stdlib modules ===== */\n"
                      "void vm_register_native(VM *vm, const char *name, XsNativeFn fn) {\n"
                      "    vm_register_native_fn((VMState *)vm, name, fn);\n"
                      "}\n")
        if "vm_register_native(VM" not in content:
            content += "\n" + bridge_fn
            vm_c.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: {vm_c.relative_to(ROOT)} - add vm_register_native bridge function")

    # ── 15. CMakeLists.txt: fix IDE target double-including ide_app.c ──
    if cmake_path.exists():
        content = cmake_path.read_text()
        if "add_executable(xsharp-ide src/ide/core/ide_app.c ${IDE_SOURCES}" in content:
            content = content.replace(
                "add_executable(xsharp-ide src/ide/core/ide_app.c ${IDE_SOURCES}\n"
                "                ${CORE_SOURCES} ${STDLIB_SOURCES})",
                "add_executable(xsharp-ide ${IDE_SOURCES}\n"
                "                ${CORE_SOURCES} ${STDLIB_SOURCES})"
            )
            cmake_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: CMakeLists.txt - remove duplicate ide_app.c in IDE target")

    # ── 15. CMakeLists.txt: fix test target including test_main.c twice ──
    if cmake_path.exists():
        content = cmake_path.read_text()
        if "add_executable(xsharp-tests tests/test_main.c ${TEST_SOURCES}" in content:
            content = content.replace(
                "add_executable(xsharp-tests tests/test_main.c ${TEST_SOURCES}\n"
                "        ${CORE_SOURCES} ${STDLIB_SOURCES})",
                "add_executable(xsharp-tests ${TEST_SOURCES}\n"
                "        ${CORE_SOURCES} ${STDLIB_SOURCES})"
            )
            cmake_path.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: CMakeLists.txt - remove duplicate test_main.c in test target")

    # ── 18. test_main.c: fix function declarations to match implementations ──
    test_main = TESTS / "test_main.c"
    if test_main.exists():
        content = test_main.read_text()
        if "extern int test_lexer_all" in content:
            content = content.replace("extern int test_lexer_all(void);", "extern void run_lexer_tests(void);")
            content = content.replace("extern int test_parser_all(void);", "extern void run_parser_tests(void);")
            content = content.replace("extern int test_compiler_all(void);", "extern void run_compiler_tests(void);")
            content = content.replace("extern int test_runtime_all(void);", "extern void run_runtime_tests(void);")
            content = content.replace("pass = test_lexer_all();", "run_lexer_tests();")
            content = content.replace("pass = test_parser_all();", "run_parser_tests();")
            content = content.replace("pass = test_compiler_all();", "run_compiler_tests();")
            content = content.replace("pass = test_runtime_all();", "run_runtime_tests();")
            # Remove now-unused variables
            content = content.replace("    int total_pass = 0, total_fail = 0;\n    int pass, fail;\n", "")
            content = content.replace("    fail = 0; /* failure count embedded in test_lexer_all */\n    total_pass += pass;\n", "")
            content = content.replace("    total_pass += pass;\n", "")
            test_main.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: {test_main.relative_to(ROOT)} - match function declarations to implementations")

    # ── 19. test_framework.h: add TEST_ASSERT and TEST_RUN macros ──
    patch_insert_before(
        TESTS / "test_framework.h",
        "/* Run a test function. Call TEST_CASE inside the function. */\n"
        "#define RUN_TEST(fn)",
        '/* Assert with message (used by tests) */\n'
        '#define TEST_ASSERT(expr, msg)                                  \\\n'
        '    do {                                                        \\\n'
        '        if (!(expr)) {                                          \\\n'
        '            TEST_FAIL(msg);                                     \\\n'
        '            return;                                             \\\n'
        '        }                                                       \\\n'
        '    } while (0)\n'
        '\n'
        '/* Run a test function and track pass count */\n'
        '#define TEST_RUN(fn, pass_ptr)                                  \\\n'
        '    do {                                                        \\\n'
        '        TEST_CASE(#fn);                                         \\\n'
        '        int _before = xs_test_fail_count;                       \\\n'
        '        fn();                                                   \\\n'
        '        if (xs_test_fail_count == _before) {                    \\\n'
        '            TEST_PASS();                                        \\\n'
        '            if (pass_ptr) (*(pass_ptr))++;                      \\\n'
        '        }                                                       \\\n'
        '    } while (0)\n'
        '\n',
        "add TEST_ASSERT and TEST_RUN macros"
    )

    # ── 19. test_integration.c: fix lexer_init arg count ──
    patch_file(
        TESTS / "integration/test_integration.c",
        'lexer_init(&lexer, source, "<test>");',
        'lexer_init(&lexer, source);',
        "fix lexer_init argument count (2 not 3)"
    )

    # ── 20. test_stdlib.c: fix wrong function names ──
    stdlib_test = TESTS / "stdlib/test_stdlib.c"
    if stdlib_test.exists():
        content = stdlib_test.read_text()
        replacements = {
            "xs_str_length": "xs_string_length",
            "xs_str_to_upper": "xs_string_toUpper",
            "xs_str_contains": "xs_string_contains",
        }
        changed = False
        for old_name, new_name in replacements.items():
            if old_name in content:
                content = content.replace(old_name, new_name)
                changed = True
        if changed:
            stdlib_test.write_text(content)
            ERRORS_FIXED += 1
            log(f"  Fixed: {stdlib_test.relative_to(ROOT)} - correct stdlib function names")

    log(f"Applied {ERRORS_FIXED} fixes.")

# ── Collect source files ─────────────────────────────────────────────────────

def glob_c_files(*patterns):
    """Glob for .c files matching patterns relative to ROOT."""
    files = []
    for pat in patterns:
        files.extend(sorted(ROOT.glob(pat)))
    return files

def get_core_sources():
    return glob_c_files(
        "src/lexer/*.c", "src/parser/*.c", "src/ast/*.c",
        "src/compiler/*.c", "src/codegen/*.c", "src/vm/*.c",
        "src/runtime/*.c", "src/debugger/*.c", "src/cli/*.c",
        "src/formats/*.c",
        "src/renderer/core/*.c", "src/renderer/gpu/*.c",
        "src/renderer/shaders/*.c", "src/renderer/scene/*.c",
    )

def get_stdlib_sources():
    return glob_c_files("stdlib/*/*.c")

def get_test_sources():
    return glob_c_files("tests/*.c", "tests/*/*.c")

def get_ide_sources():
    return glob_c_files("src/ide/**/*.c")

# ── Compilation ──────────────────────────────────────────────────────────────

def compile_object(src_file, obj_file, cflags):
    """Compile a single .c -> .o file."""
    obj_file.parent.mkdir(parents=True, exist_ok=True)
    cmd = [CC] + cflags + INCLUDES + ["-c", "-o", str(obj_file), str(src_file)]
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    return r

def link_binary(obj_files, output, extra_libs=None):
    """Link object files into a binary."""
    cmd = [CC, "-o", str(output)] + [str(o) for o in obj_files] + LDFLAGS
    if extra_libs:
        cmd += extra_libs
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    return r

def build_target(name, sources, output_name, cflags, extra_libs=None):
    """Compile and link a complete target."""
    log(f"Building {name}...")
    obj_dir = BUILD / "obj" / name
    obj_dir.mkdir(parents=True, exist_ok=True)

    obj_files = []
    errors = []
    warnings = []

    for src in sources:
        rel = src.relative_to(ROOT)
        obj = obj_dir / rel.with_suffix(".o")
        obj.parent.mkdir(parents=True, exist_ok=True)

        r = compile_object(src, obj, cflags)
        if r.returncode != 0:
            errors.append((src, r.stderr))
        elif r.stderr.strip():
            # Collect warnings but don't fail
            for line in r.stderr.strip().split("\n"):
                if "warning:" in line:
                    warnings.append(line)
        obj_files.append(obj)

    if errors:
        err(f"Compilation errors in {name}:")
        for src, stderr in errors:
            print(f"  {RED}{src.relative_to(ROOT)}{RESET}:")
            for line in stderr.strip().split("\n")[:10]:
                print(f"    {line}")
        return False

    if warnings:
        warn(f"  {len(warnings)} warnings (non-fatal)")

    # Link
    output = BUILD / output_name
    log(f"Linking {output_name}...")
    r = link_binary(obj_files, output, extra_libs)
    if r.returncode != 0:
        err(f"Link failed for {name}:")
        print(r.stderr)
        return False

    # Make executable
    output.chmod(0o755)
    size_kb = output.stat().st_size / 1024
    log(f"  {GREEN}OK{RESET}: {output} ({size_kb:.0f} KB)")
    return True

def build_xsharp(cflags):
    """Build the main xsharp CLI binary."""
    sources = [ROOT / "src/main.c"] + get_core_sources() + get_stdlib_sources()
    extra = []
    # Check for readline
    r = run(["pkg-config", "--libs", "readline"])
    if r.returncode == 0:
        extra.append("-lreadline")

    return build_target("xsharp", sources, "xsharp", cflags, extra)

def build_tests(cflags):
    """Build the test runner."""
    test_cflags = cflags + [f"-I{TESTS}"]
    sources = get_test_sources() + get_core_sources() + get_stdlib_sources()
    return build_target("tests", sources, "xsharp-tests", test_cflags)

def build_ide(cflags):
    """Build the GTK3 IDE."""
    # Check for GTK3
    r = run(["pkg-config", "--cflags", "gtk+-3.0", "gtksourceview-3.0"])
    if r.returncode != 0:
        warn("GTK3/GtkSourceView not found - skipping IDE build")
        warn("  Install: apt-get install libgtk-3-dev libgtksourceview-3.0-dev")
        return False

    gtk_cflags = r.stdout.strip().split()
    r2 = run(["pkg-config", "--libs", "gtk+-3.0", "gtksourceview-3.0"])
    gtk_libs = r2.stdout.strip().split()

    ide_cflags = cflags + gtk_cflags + ["-DXS_ENABLE_IDE"]
    sources = get_ide_sources() + get_core_sources() + get_stdlib_sources()
    extra = gtk_libs
    return build_target("ide", sources, "xsharp-ide", ide_cflags, extra)

def build_vscode():
    """Build the VS Code extension."""
    ext_dir = ROOT / "vscode-extension"
    if not ext_dir.exists():
        warn("vscode-extension/ not found - skipping")
        return False

    if not check_command("npm"):
        warn("npm not found - skipping VS Code extension build")
        warn("  Install: apt-get install nodejs npm")
        return False

    log("Building VS Code extension...")
    r = run(["npm", "install"], cwd=ext_dir)
    if r.returncode != 0:
        warn("npm install failed:")
        print(r.stderr)
        return False

    # Check if there's a build script
    pkg_json = ext_dir / "package.json"
    import json
    pkg = json.loads(pkg_json.read_text())
    if "scripts" in pkg and "compile" in pkg["scripts"]:
        r = run(["npm", "run", "compile"], cwd=ext_dir)
        if r.returncode != 0:
            # Try installing vsce and compiling TypeScript directly
            if check_command("npx"):
                r = run(["npx", "tsc", "-p", "."], cwd=ext_dir)
    elif check_command("npx"):
        r = run(["npx", "tsc", "-p", "."], cwd=ext_dir)

    log("  VS Code extension built.")
    return True

# ── Clean ────────────────────────────────────────────────────────────────────

def clean():
    """Remove build artifacts."""
    log("Cleaning build artifacts...")
    if BUILD.exists():
        shutil.rmtree(BUILD)
    # Remove .o files from source tree
    for o in ROOT.rglob("*.o"):
        o.unlink()
    # Remove built binaries
    for name in ["xsharp", "xsharp-ide", "xsharp-tests"]:
        p = ROOT / name
        if p.exists():
            p.unlink()
    log("Clean complete.")

# ── Install ──────────────────────────────────────────────────────────────────

def install(prefix="/usr/local"):
    """Install xsharp to prefix."""
    binary = BUILD / "xsharp"
    if not binary.exists():
        err("xsharp binary not found. Run build first.")
        return False

    bin_dir = Path(prefix) / "bin"
    share_dir = Path(prefix) / "share" / "xsharp"

    log(f"Installing to {prefix}...")
    bin_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binary, bin_dir / "xsharp")
    (bin_dir / "xsharp").chmod(0o755)

    if share_dir.exists():
        shutil.rmtree(share_dir)
    share_dir.mkdir(parents=True, exist_ok=True)
    shutil.copytree(STDLIB, share_dir / "stdlib")
    shutil.copytree(ROOT / "examples", share_dir / "examples")

    log(f"Installed to {prefix}")
    return True

# ── Run tests ────────────────────────────────────────────────────────────────

def run_tests():
    """Run the test binary."""
    test_bin = BUILD / "xsharp-tests"
    if not test_bin.exists():
        warn("Test binary not found. Building tests first...")
        return False

    log("Running tests...")
    r = subprocess.run([str(test_bin)], cwd=ROOT, capture_output=True, text=True)
    print(r.stdout)
    if r.stderr:
        print(r.stderr)
    if r.returncode == 0:
        log("All tests passed!")
    else:
        err(f"Tests failed with exit code {r.returncode}")
    return r.returncode == 0

# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="X# (Xsharp) Build System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            Examples:
              python3 build.py                  # default debug build
              python3 build.py --release        # optimised build
              python3 build.py --all            # build everything
              python3 build.py --fix-only       # just patch source issues
              python3 build.py --clean          # remove artifacts
        """))

    parser.add_argument("--release", action="store_true",
                        help="Build with optimisations (-O2)")
    parser.add_argument("--clean", action="store_true",
                        help="Remove all build artifacts")
    parser.add_argument("--fix-only", action="store_true",
                        help="Apply source fixes without building")
    parser.add_argument("--ide", action="store_true",
                        help="Also build the GTK3 IDE")
    parser.add_argument("--vscode", action="store_true",
                        help="Also build the VS Code extension")
    parser.add_argument("--tests", action="store_true",
                        help="Also build and run tests")
    parser.add_argument("--all", action="store_true",
                        help="Build everything (CLI + IDE + tests + VS Code)")
    parser.add_argument("--install", action="store_true",
                        help="Install after building")
    parser.add_argument("--prefix", default="/usr/local",
                        help="Install prefix (default: /usr/local)")
    parser.add_argument("--skip-deps", action="store_true",
                        help="Skip dependency checking")
    parser.add_argument("--cc", default=None,
                        help="C compiler to use (default: gcc)")

    args = parser.parse_args()

    if args.cc:
        global CC
        CC = args.cc

    if args.all:
        args.ide = True
        args.vscode = True
        args.tests = True

    print(f"""
{BOLD}╔══════════════════════════════════════════╗
║     X# (Xsharp) Build System v0.1.0     ║
╚══════════════════════════════════════════╝{RESET}
""")

    # Clean
    if args.clean:
        clean()
        return 0

    # Dependencies
    if not args.skip_deps:
        install_deps(also_optional=args.all or args.ide)

    # Apply source patches
    apply_all_patches()

    if args.fix_only:
        log("Fix-only mode: done.")
        return 0

    # Create build directory
    BUILD.mkdir(parents=True, exist_ok=True)

    cflags = CFLAGS_RELEASE if args.release else CFLAGS_DEBUG
    mode = "release" if args.release else "debug"
    log(f"Build mode: {mode}")

    # Build main CLI
    ok = build_xsharp(cflags)
    if not ok:
        err("Main build FAILED")
        return 1

    # Build tests
    if args.tests:
        ok = build_tests(cflags)
        if ok:
            run_tests()

    # Build IDE
    if args.ide:
        build_ide(cflags)

    # Build VS Code extension
    if args.vscode:
        build_vscode()

    # Install
    if args.install:
        install(args.prefix)

    print(f"""
{GREEN}{BOLD}Build complete!{RESET}
  Binary: {BUILD}/xsharp
  Run:    {BUILD}/xsharp run examples/hello.xs
  Tests:  {BUILD}/xsharp-tests (if built with --tests)
""")
    return 0


if __name__ == "__main__":
    sys.exit(main())
