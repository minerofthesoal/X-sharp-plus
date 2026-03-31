#!/usr/bin/env python3
"""
X# (Xsharp) Comprehensive Build System
========================================
Builds ALL components of the X# project:
  1. Core compiler/runtime (C)
  2. Test suite
  3. Qt6 IDE (C++/Qt6)
  4. VS Code extension (TypeScript)
  5. AppImage packaging

Usage:
    python3 build_all.py              # build core + tests
    python3 build_all.py --all        # build everything
    python3 build_all.py --core       # core compiler only
    python3 build_all.py --tests      # build + run tests
    python3 build_all.py --ide        # build Qt6 IDE
    python3 build_all.py --vscode     # build VS Code extension
    python3 build_all.py --appimage   # create AppImage
    python3 build_all.py --install-deps  # install all dependencies
    python3 build_all.py --clean      # clean build artifacts
    python3 build_all.py --release    # release build (optimized)
    python3 build_all.py --fix-only   # only apply source fixes
"""

import argparse
import glob
import os
import platform
import shutil
import subprocess
import sys
import urllib.request
from pathlib import Path

# ── Globals ──────────────────────────────────────────────────────────────────

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "src"
STDLIB = ROOT / "stdlib"
BUILD = ROOT / "build"
TESTS = ROOT / "tests"
IDE_QT6 = SRC / "ide" / "qt6"
VSCODE_EXT = ROOT / "vscode-extension"
ASSETS = ROOT / "assets"

CC = os.environ.get("CC", "gcc")
CXX = os.environ.get("CXX", "g++")
CFLAGS_BASE = ["-std=c11", "-Wall", "-Wextra", "-D_GNU_SOURCE",
               "-D_POSIX_C_SOURCE=200809L"]
CFLAGS_DEBUG = CFLAGS_BASE + ["-g", "-O0", "-DDEBUG"]
CFLAGS_RELEASE = CFLAGS_BASE + ["-O2", "-DNDEBUG"]
LDFLAGS = ["-lm", "-lpthread"]
INCLUDES = [f"-I{SRC}", f"-I{STDLIB}"]

GREEN = "\033[32m"
YELLOW = "\033[33m"
RED = "\033[31m"
CYAN = "\033[36m"
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

def section(title):
    print(f"\n{CYAN}{BOLD}{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}{RESET}\n")

def run(cmd, **kw):
    kw.setdefault("cwd", str(ROOT))
    kw.setdefault("capture_output", True)
    kw.setdefault("text", True)
    return subprocess.run(cmd, **kw)

def run_live(cmd, **kw):
    """Run a command with live output."""
    kw.setdefault("cwd", str(ROOT))
    return subprocess.run(cmd, **kw)

def file_read(path):
    return Path(path).read_text()

def file_write(path, content):
    Path(path).write_text(content)

def check_command(name):
    return shutil.which(name) is not None

# ── Dependency Management ────────────────────────────────────────────────────

def detect_distro():
    """Detect Linux distribution."""
    try:
        with open("/etc/os-release") as f:
            text = f.read()
        if "ubuntu" in text.lower() or "debian" in text.lower():
            return "debian"
        elif "fedora" in text.lower() or "rhel" in text.lower() or "centos" in text.lower():
            return "fedora"
        elif "arch" in text.lower() or "manjaro" in text.lower():
            return "arch"
    except FileNotFoundError:
        pass
    return "unknown"

def install_core_deps():
    """Install core build dependencies."""
    distro = detect_distro()
    log(f"Detected distro: {distro}")

    if distro == "debian":
        pkgs = ["gcc", "g++", "make", "cmake", "libreadline-dev"]
        run_live(["apt-get", "update", "-qq"])
        run_live(["apt-get", "install", "-y", "-qq"] + pkgs)
    elif distro == "fedora":
        pkgs = ["gcc", "gcc-c++", "make", "cmake", "readline-devel"]
        run_live(["dnf", "install", "-y"] + pkgs)
    elif distro == "arch":
        pkgs = ["gcc", "make", "cmake", "readline"]
        run_live(["pacman", "-S", "--noconfirm"] + pkgs)
    else:
        warn("Unknown distro. Please install gcc, g++, make, cmake manually.")

def install_qt6_deps():
    """Install Qt6 development packages."""
    distro = detect_distro()
    log("Installing Qt6 dependencies...")

    if distro == "debian":
        pkgs = ["qt6-base-dev", "qt6-tools-dev", "qt6-tools-dev-tools",
                "libgl1-mesa-dev"]
        run_live(["apt-get", "install", "-y", "-qq"] + pkgs)
    elif distro == "fedora":
        pkgs = ["qt6-qtbase-devel", "qt6-qttools-devel"]
        run_live(["dnf", "install", "-y"] + pkgs)
    elif distro == "arch":
        pkgs = ["qt6-base", "qt6-tools"]
        run_live(["pacman", "-S", "--noconfirm"] + pkgs)
    else:
        warn("Unknown distro. Please install Qt6 dev packages manually.")

def install_node_deps():
    """Install Node.js and npm."""
    if check_command("node") and check_command("npm"):
        log("Node.js and npm already available.")
        return

    distro = detect_distro()
    log("Installing Node.js...")

    if distro == "debian":
        run_live(["apt-get", "install", "-y", "-qq", "nodejs", "npm"])
    elif distro == "fedora":
        run_live(["dnf", "install", "-y", "nodejs", "npm"])
    elif distro == "arch":
        run_live(["pacman", "-S", "--noconfirm", "nodejs", "npm"])
    else:
        warn("Unknown distro. Please install Node.js manually.")

def install_all_deps():
    """Install all dependencies."""
    section("Installing All Dependencies")
    install_core_deps()
    install_qt6_deps()
    install_node_deps()
    log("All dependencies installed.")

# ── Source Patches (from build.py) ───────────────────────────────────────────

def apply_patches():
    """Apply all known source-level fixes."""
    global ERRORS_FIXED
    section("Applying Source Patches")

    patches = [
        # cli.c / linter.c need <stdarg.h>
        (SRC / "cli" / "cli.c", '#include "cli.h"',
         '#include "cli.h"\n#include <stdarg.h>',
         "cli.c: add <stdarg.h>"),
        (SRC / "cli" / "linter.c", '#include "linter.h"',
         '#include "linter.h"\n#include <stdarg.h>',
         "linter.c: add <stdarg.h>"),
        # fs_lib.c PATH_MAX fallback
        (STDLIB / "fs" / "fs_lib.c", '#include "fs_lib.h"',
         '#include "fs_lib.h"\n#ifndef PATH_MAX\n#define PATH_MAX 4096\n#endif',
         "fs_lib.c: add PATH_MAX fallback"),
        # breakpoint.c needs debug_info.h
        (SRC / "debugger" / "breakpoint.c", '#include "breakpoint.h"',
         '#include "breakpoint.h"\n#include "debug_info.h"',
         "breakpoint.c: add debug_info.h include"),
    ]

    for filepath, old, new, desc in patches:
        if not filepath.exists():
            continue
        content = file_read(filepath)
        if old in content and new not in content:
            content = content.replace(old, new, 1)
            file_write(filepath, content)
            log(f"  Fixed: {desc}")
            ERRORS_FIXED += 1

    # Fix vm_register_native bridge
    vm_c = SRC / "vm" / "vm.c"
    if vm_c.exists():
        content = file_read(vm_c)
        if "vm_register_native" not in content or \
           ("vm_register_native_fn" in content and "void vm_register_native(" not in content):
            content += """
/* Bridge: stdlib modules call vm_register_native(VM*, ...) */
void vm_register_native(VM* vm, const char* name, XsNativeFn fn) {
    vm_register_native_fn((VMState*)vm, name, fn);
}
"""
            file_write(vm_c, content)
            log("  Fixed: vm.c bridge function")
            ERRORS_FIXED += 1

    # Fix runtime.h declaration
    runtime_h = SRC / "runtime" / "runtime.h"
    if runtime_h.exists():
        content = file_read(runtime_h)
        if "void vm_register_native(" not in content:
            # Add before the final #endif
            marker = "#endif"
            idx = content.rfind(marker)
            if idx >= 0:
                insert = "\n/* VM native registration bridge */\nvoid vm_register_native(VM* vm, const char* name, XsNativeFn fn);\n\n"
                content = content[:idx] + insert + content[idx:]
                file_write(runtime_h, content)
                log("  Fixed: runtime.h declaration")
                ERRORS_FIXED += 1

    # Fix LZMA2 carry propagation bug
    lzma2_c = SRC / "formats" / "lzma2.c"
    if lzma2_c.exists():
        content = file_read(lzma2_c)
        if "(uint8_t)(0xFF + 0)" in content:
            content = content.replace("(uint8_t)(0xFF + 0)", "(uint8_t)(0xFF + carry)")
            file_write(lzma2_c, content)
            log("  Fixed: LZMA2 range coder carry propagation")
            ERRORS_FIXED += 1

    log(f"Patches applied: {ERRORS_FIXED} fixes")

# ── Core Compiler Build ──────────────────────────────────────────────────────

def collect_c_sources(exclude_patterns=None):
    """Collect all .c source files for the core build."""
    exclude_patterns = exclude_patterns or []
    sources = []

    for d in [SRC, STDLIB]:
        for root, dirs, files in os.walk(d):
            # Skip IDE directory
            if "ide" in root:
                continue
            for f in files:
                if not f.endswith(".c"):
                    continue
                path = os.path.join(root, f)
                skip = False
                for pat in exclude_patterns:
                    if pat in path:
                        skip = True
                        break
                if not skip:
                    sources.append(path)

    return sources

def build_core(release=False):
    """Build the core X# compiler/runtime."""
    section("Building Core Compiler")

    BUILD.mkdir(exist_ok=True)
    cflags = CFLAGS_RELEASE if release else CFLAGS_DEBUG
    output = BUILD / "xsharp"

    # Collect sources, excluding test files
    sources = collect_c_sources(exclude_patterns=["test_", "tests/"])

    cmd = [CC] + cflags + INCLUDES + sources + LDFLAGS + ["-o", str(output)]

    log(f"Compiling {len(sources)} source files...")
    r = run(cmd)
    if r.returncode != 0:
        err("Core build failed!")
        if r.stderr:
            print(r.stderr[:2000])
        return False

    size = output.stat().st_size
    log(f"Built: {output} ({size // 1024}KB)")
    return True

def build_tests(release=False):
    """Build and run the test suite."""
    section("Building Test Suite")

    BUILD.mkdir(exist_ok=True)
    cflags = CFLAGS_RELEASE if release else CFLAGS_DEBUG
    output = BUILD / "xsharp-tests"

    # Collect all sources + test sources
    sources = collect_c_sources()

    # Add test sources
    for root, dirs, files in os.walk(TESTS):
        for f in files:
            if f.endswith(".c"):
                sources.append(os.path.join(root, f))

    # Deduplicate
    sources = list(set(sources))

    # Exclude main.c from src (test_main.c has its own main)
    sources = [s for s in sources if not s.endswith("src/main.c")]

    cmd = [CC] + cflags + INCLUDES + [f"-I{TESTS}"] + sources + LDFLAGS + ["-o", str(output)]

    log(f"Compiling {len(sources)} source files (with tests)...")
    r = run(cmd)
    if r.returncode != 0:
        err("Test build failed!")
        if r.stderr:
            print(r.stderr[:3000])
        return False

    size = output.stat().st_size
    log(f"Built: {output} ({size // 1024}KB)")

    # Run tests
    log("Running tests...")
    r = run([str(output)])
    if r.stdout:
        print(r.stdout)
    if r.stderr:
        print(r.stderr)

    # Count results
    pass_count = r.stdout.count("[PASS]") if r.stdout else 0
    fail_count = r.stdout.count("[FAIL]") if r.stdout else 0
    log(f"Results: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        warn(f"{fail_count} tests failed!")
        return False

    log("All tests passed!")
    return True

# ── Qt6 IDE Build ────────────────────────────────────────────────────────────

def build_ide():
    """Build the Qt6 IDE."""
    section("Building Qt6 IDE")

    if not IDE_QT6.exists():
        err(f"Qt6 IDE source not found at {IDE_QT6}")
        return False

    # Check for cmake and Qt6
    if not check_command("cmake"):
        err("cmake not found. Run with --install-deps first.")
        return False

    # Create build directory
    ide_build = BUILD / "ide"
    ide_build.mkdir(parents=True, exist_ok=True)

    # Try cmake build
    cmake_file = IDE_QT6 / "CMakeLists.txt"
    if cmake_file.exists():
        log("Configuring with CMake...")
        r = run(["cmake", str(IDE_QT6), "-DCMAKE_BUILD_TYPE=Release"],
                cwd=str(ide_build))
        if r.returncode != 0:
            if "Qt6" in (r.stderr or ""):
                warn("Qt6 not found. Install with: --install-deps")
                warn("Skipping IDE build (Qt6 not available)")
                return False
            err(f"CMake configure failed: {r.stderr[:1000] if r.stderr else 'unknown error'}")
            return False

        log("Building IDE...")
        r = run(["cmake", "--build", ".", "--parallel"], cwd=str(ide_build))
        if r.returncode != 0:
            err(f"IDE build failed: {r.stderr[:1000] if r.stderr else 'unknown error'}")
            return False

        # Find the built binary
        ide_bin = ide_build / "xsharp-ide"
        if ide_bin.exists():
            log(f"Built: {ide_bin} ({ide_bin.stat().st_size // 1024}KB)")
            return True

    # Fallback: try qmake
    pro_file = IDE_QT6 / "qt6_ide.pro"
    if pro_file.exists() and check_command("qmake6"):
        log("Trying qmake6 build...")
        r = run(["qmake6", str(pro_file)], cwd=str(ide_build))
        if r.returncode == 0:
            r = run(["make", "-j4"], cwd=str(ide_build))
            if r.returncode == 0:
                log("IDE built successfully with qmake6")
                return True

    warn("IDE build failed or Qt6 not available")
    return False

# ── VS Code Extension Build ─────────────────────────────────────────────────

def build_vscode():
    """Build the VS Code extension."""
    section("Building VS Code Extension")

    if not VSCODE_EXT.exists():
        err(f"VS Code extension source not found at {VSCODE_EXT}")
        return False

    if not check_command("npm"):
        err("npm not found. Run with --install-deps first.")
        return False

    # Install dependencies
    log("Installing npm dependencies...")
    r = run(["npm", "install"], cwd=str(VSCODE_EXT))
    if r.returncode != 0:
        err(f"npm install failed: {r.stderr[:500] if r.stderr else ''}")
        return False
    log("npm dependencies installed.")

    # Compile TypeScript
    log("Compiling TypeScript...")
    npx = shutil.which("npx")
    if not npx:
        err("npx not found")
        return False

    r = run(["npx", "tsc", "-p", "."], cwd=str(VSCODE_EXT))
    if r.returncode != 0:
        err(f"TypeScript compilation failed:")
        if r.stdout:
            print(r.stdout[:2000])
        if r.stderr:
            print(r.stderr[:2000])
        return False
    log("TypeScript compiled successfully.")

    # Check if out/ was created
    out_dir = VSCODE_EXT / "out"
    if out_dir.exists():
        js_files = list(out_dir.glob("**/*.js"))
        log(f"Generated {len(js_files)} JavaScript files in out/")

    # Package as .vsix
    log("Packaging extension as .vsix...")
    r = run(["npx", "@vscode/vsce", "package", "--no-dependencies", "--allow-missing-repository"],
            cwd=str(VSCODE_EXT))
    if r.returncode != 0:
        warn(f"VSIX packaging failed (non-critical): {r.stderr[:300] if r.stderr else ''}")
        # Not fatal - the extension source is compiled
    else:
        vsix_files = list(VSCODE_EXT.glob("*.vsix"))
        if vsix_files:
            log(f"Packaged: {vsix_files[0]}")

    log("VS Code extension built successfully.")
    return True

# ── AppImage Build ───────────────────────────────────────────────────────────

def download_file(url, dest):
    """Download a file with progress."""
    log(f"Downloading {url}...")
    try:
        urllib.request.urlretrieve(url, dest)
        os.chmod(dest, 0o755)
        return True
    except Exception as e:
        err(f"Download failed: {e}")
        return False

def build_appimage():
    """Create an AppImage."""
    section("Creating AppImage")

    tools_dir = BUILD / "tools"
    tools_dir.mkdir(parents=True, exist_ok=True)

    # Check prerequisites
    xsharp_bin = BUILD / "xsharp"
    ide_bin = BUILD / "ide" / "xsharp-ide"

    if not xsharp_bin.exists():
        warn("xsharp binary not found. Building core first...")
        if not build_core(release=True):
            return False

    # Download linuxdeploy if needed
    linuxdeploy = tools_dir / "linuxdeploy-x86_64.AppImage"
    if not linuxdeploy.exists():
        url = "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
        if not download_file(url, str(linuxdeploy)):
            warn("Could not download linuxdeploy. Creating manual AppDir instead.")
            return create_manual_appdir()

    # Create AppDir structure
    appdir = BUILD / "AppDir"
    if appdir.exists():
        shutil.rmtree(appdir)

    (appdir / "usr" / "bin").mkdir(parents=True)
    (appdir / "usr" / "share" / "applications").mkdir(parents=True)
    (appdir / "usr" / "share" / "icons" / "hicolor" / "256x256" / "apps").mkdir(parents=True)

    # Copy binaries
    shutil.copy2(xsharp_bin, appdir / "usr" / "bin" / "xsharp")
    if ide_bin.exists():
        shutil.copy2(ide_bin, appdir / "usr" / "bin" / "xsharp-ide")

    # Copy desktop file
    desktop_src = ASSETS / "xsharp-ide.desktop"
    if desktop_src.exists():
        shutil.copy2(desktop_src, appdir / "usr" / "share" / "applications" / "xsharp-ide.desktop")

    # Create icon (simple PNG from SVG or placeholder)
    icon_dest = appdir / "usr" / "share" / "icons" / "hicolor" / "256x256" / "apps" / "xsharp-ide.png"
    svg_src = ASSETS / "xsharp-ide.svg"
    if svg_src.exists() and check_command("rsvg-convert"):
        run(["rsvg-convert", "-w", "256", "-h", "256", str(svg_src), "-o", str(icon_dest)])
    else:
        # Create a simple placeholder PNG (1x1 pixel)
        create_placeholder_png(str(icon_dest))

    # Run linuxdeploy
    log("Running linuxdeploy...")
    env = os.environ.copy()
    env["ARCH"] = "x86_64"

    r = run([str(linuxdeploy),
             "--appdir", str(appdir),
             "--desktop-file", str(appdir / "usr" / "share" / "applications" / "xsharp-ide.desktop"),
             "--icon-file", str(icon_dest),
             "--output", "appimage"],
            cwd=str(BUILD), env=env)

    if r.returncode != 0:
        warn("linuxdeploy failed. Creating manual AppDir archive instead.")
        return create_manual_appdir()

    appimage_files = list(BUILD.glob("*.AppImage"))
    if appimage_files:
        log(f"Created: {appimage_files[0]}")
        return True

    return create_manual_appdir()

def create_placeholder_png(path):
    """Create a minimal valid PNG file (1x1 red pixel)."""
    import struct
    import zlib

    def chunk(chunk_type, data):
        c = chunk_type + data
        crc = struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)
        return struct.pack(">I", len(data)) + c + crc

    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = chunk(b'IHDR', struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0))
    raw = b'\x00\xe9\x45\x60'  # filter byte + RGB
    idat = chunk(b'IDAT', zlib.compress(raw))
    iend = chunk(b'IEND', b'')

    with open(path, 'wb') as f:
        f.write(sig + ihdr + idat + iend)

def create_manual_appdir():
    """Create a simple tar.gz distribution instead of AppImage."""
    appdir = BUILD / "AppDir"
    if not appdir.exists():
        warn("No AppDir to package")
        return False

    import tarfile
    archive = BUILD / "xsharp-linux-x86_64.tar.gz"
    log(f"Creating archive: {archive}")
    with tarfile.open(archive, "w:gz") as tar:
        tar.add(appdir, arcname="xsharp")

    log(f"Created: {archive} ({archive.stat().st_size // 1024}KB)")
    return True

# ── Clean ────────────────────────────────────────────────────────────────────

def clean():
    """Remove all build artifacts."""
    section("Cleaning Build Artifacts")
    dirs_to_clean = [
        BUILD,
        VSCODE_EXT / "out",
        VSCODE_EXT / "node_modules",
    ]
    files_to_clean = list(VSCODE_EXT.glob("*.vsix"))

    for d in dirs_to_clean:
        if d.exists():
            shutil.rmtree(d)
            log(f"Removed: {d}")
    for f in files_to_clean:
        f.unlink()
        log(f"Removed: {f}")

    log("Clean complete.")

# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="X# (Xsharp) Build System")
    parser.add_argument("--all", action="store_true", help="Build everything")
    parser.add_argument("--core", action="store_true", help="Build core compiler")
    parser.add_argument("--tests", action="store_true", help="Build and run tests")
    parser.add_argument("--ide", action="store_true", help="Build Qt6 IDE")
    parser.add_argument("--vscode", action="store_true", help="Build VS Code extension")
    parser.add_argument("--appimage", action="store_true", help="Create AppImage")
    parser.add_argument("--install-deps", action="store_true", help="Install all dependencies")
    parser.add_argument("--clean", action="store_true", help="Clean build artifacts")
    parser.add_argument("--release", action="store_true", help="Release build")
    parser.add_argument("--fix-only", action="store_true", help="Only apply source fixes")
    args = parser.parse_args()

    print(f"{CYAN}{BOLD}")
    print("╔══════════════════════════════════════════╗")
    print("║     X# (Xsharp) Build System v0.2.0     ║")
    print("╚══════════════════════════════════════════╝")
    print(f"{RESET}")

    if args.clean:
        clean()
        return 0

    if args.install_deps:
        install_all_deps()
        if not args.all and not args.core and not args.tests and not args.ide and not args.vscode:
            return 0

    # Always apply patches first
    apply_patches()

    if args.fix_only:
        log("Fix-only mode: patches applied, skipping compilation.")
        return 0

    results = {}

    # Determine what to build
    build_core_flag = args.core or args.all or (not any([args.ide, args.vscode, args.appimage, args.tests]))
    build_tests_flag = args.tests or args.all
    build_ide_flag = args.ide or args.all
    build_vscode_flag = args.vscode or args.all
    build_appimage_flag = args.appimage or args.all

    # Build core
    if build_core_flag:
        results["Core Compiler"] = build_core(release=args.release)

    # Build tests
    if build_tests_flag:
        results["Test Suite"] = build_tests(release=args.release)

    # Build IDE
    if build_ide_flag:
        results["Qt6 IDE"] = build_ide()

    # Build VS Code extension
    if build_vscode_flag:
        results["VS Code Extension"] = build_vscode()

    # Build AppImage
    if build_appimage_flag:
        results["AppImage"] = build_appimage()

    # Summary
    section("Build Summary")
    all_ok = True
    for component, success in results.items():
        status = f"{GREEN}OK{RESET}" if success else f"{RED}FAILED{RESET}"
        if not success:
            all_ok = False
        print(f"  {component:.<40s} [{status}]")

    print()
    if all_ok:
        log("All builds completed successfully!")
    else:
        warn("Some builds failed. Check output above for details.")

    if (BUILD / "xsharp").exists():
        print(f"\n  {BOLD}Binary:{RESET}  {BUILD / 'xsharp'}")
    if (BUILD / "xsharp-tests").exists():
        print(f"  {BOLD}Tests:{RESET}   {BUILD / 'xsharp-tests'}")
    if (BUILD / "ide" / "xsharp-ide").exists():
        print(f"  {BOLD}IDE:{RESET}     {BUILD / 'ide' / 'xsharp-ide'}")
    vsix = list(VSCODE_EXT.glob("*.vsix"))
    if vsix:
        print(f"  {BOLD}VS Code:{RESET} {vsix[0]}")
    appimage = list(BUILD.glob("*.AppImage"))
    if appimage:
        print(f"  {BOLD}AppImage:{RESET} {appimage[0]}")
    print()

    return 0 if all_ok else 1

if __name__ == "__main__":
    sys.exit(main())
