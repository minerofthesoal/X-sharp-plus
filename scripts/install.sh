#!/bin/bash
# X# (Xsharp) Installation Script
# Supports Linux and macOS

set -e

VERSION="0.1.0"
PREFIX="${PREFIX:-/usr/local}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

print_banner() {
    echo -e "${CYAN}"
    echo "  ██╗  ██╗ ██████╗"
    echo "  ╚██╗██╔╝██╔════╝"
    echo "   ╚███╔╝ ╚█████╗ "
    echo "   ██╔██╗  ╚════██╗"
    echo "  ██╔╝ ██╗██████╔╝"
    echo "  ╚═╝  ╚═╝╚═════╝ "
    echo -e "  X# (Xsharp) v${VERSION}${NC}"
    echo ""
}

info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

detect_os() {
    case "$(uname -s)" in
        Linux*)  OS="linux";;
        Darwin*) OS="macos";;
        *)       error "Unsupported operating system: $(uname -s)";;
    esac
    info "Detected OS: $OS"
}

check_dependencies() {
    info "Checking dependencies..."

    command -v cmake >/dev/null 2>&1 || error "cmake is required. Install it first."
    command -v cc >/dev/null 2>&1 || error "A C compiler is required. Install gcc or clang."
    command -v make >/dev/null 2>&1 || error "make is required."

    if [ "$OS" = "linux" ]; then
        pkg-config --exists gtk+-3.0 2>/dev/null || warn "GTK3 not found - IDE will not be built"
        pkg-config --exists gtksourceview-3.0 2>/dev/null || warn "GtkSourceView not found - IDE will not be built"
    fi

    info "Dependencies OK"
}

configure() {
    info "Configuring build..."

    local cmake_args="-DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${PREFIX}"

    # Check for optional dependencies
    if pkg-config --exists gtk+-3.0 2>/dev/null && pkg-config --exists gtksourceview-3.0 2>/dev/null; then
        cmake_args="$cmake_args -DXS_ENABLE_IDE=ON"
    else
        cmake_args="$cmake_args -DXS_ENABLE_IDE=OFF"
    fi

    if pkg-config --exists OpenCL 2>/dev/null || [ -f /usr/include/CL/cl.h ]; then
        cmake_args="$cmake_args -DXS_ENABLE_GPU=ON"
    else
        cmake_args="$cmake_args -DXS_ENABLE_GPU=OFF"
        warn "OpenCL not found - GPU compute disabled"
    fi

    cmake -B build $cmake_args
}

build() {
    info "Building X#..."
    local nprocs
    if [ "$OS" = "macos" ]; then
        nprocs=$(sysctl -n hw.ncpu)
    else
        nprocs=$(nproc)
    fi
    cmake --build build --config "${BUILD_TYPE}" -j"$nprocs"
    info "Build complete!"
}

install() {
    info "Installing to ${PREFIX}..."
    sudo cmake --install build
    info "X# installed successfully!"
    echo ""
    echo -e "${GREEN}Run 'xsharp version' to verify the installation.${NC}"
    echo -e "${GREEN}Run 'xsharp help' to get started.${NC}"
}

run_tests() {
    info "Running tests..."
    cd build && ctest --output-on-failure && cd ..
    info "All tests passed!"
}

# Main
print_banner
detect_os
check_dependencies
configure
build

if [ "${SKIP_TESTS}" != "1" ]; then
    run_tests
fi

if [ "${SKIP_INSTALL}" != "1" ]; then
    install
fi
