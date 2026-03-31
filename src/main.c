/*
 * X# (Xsharp) Programming Language
 * Main entry point - dispatches to CLI commands
 *
 * Copyright (c) 2024 X# Contributors
 * Licensed under the MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli/cli.h"
#include "runtime/runtime.h"

#define XS_VERSION_MAJOR 0
#define XS_VERSION_MINOR 1
#define XS_VERSION_PATCH 0
#define XS_VERSION_STRING "0.1.0"

static void print_banner(void) {
    printf("\033[36m");
    printf("  ██╗  ██╗ ██████╗ \n");
    printf("  ╚██╗██╔╝██╔════╝ \n");
    printf("   ╚███╔╝ ╚█████╗  \n");
    printf("   ██╔██╗  ╚════██╗\n");
    printf("  ██╔╝ ██╗██████╔╝\n");
    printf("  ╚═╝  ╚═╝╚═════╝ \n");
    printf("\033[0m");
    printf("  X# (Xsharp) v%s\n\n", XS_VERSION_STRING);
}

static void print_usage(void) {
    print_banner();
    printf("Usage: xsharp <command> [options] [arguments]\n\n");
    printf("Commands:\n");
    printf("  run <file>          Run an X# program\n");
    printf("  build <file>        Compile to bytecode\n");
    printf("  debug <file>        Run with debugger\n");
    printf("  repl                Interactive REPL\n");
    printf("  fmt <file>          Format source code\n");
    printf("  lint <file>         Lint and check code\n");
    printf("  test <dir>          Run tests\n");
    printf("  doc <file>          Generate documentation\n");
    printf("  new <name>          Create new project\n");
    printf("  init                Initialize project\n");
    printf("  pack <dir>          Package to .Xssc\n");
    printf("  unpack <file>       Extract .Xssc archive\n");
    printf("  compress <file>     Compress to .Xscsc (LZMA2)\n");
    printf("  decompress <file>   Decompress .Xscsc\n");
    printf("  version             Show version info\n");
    printf("  help                Show this help\n");
    printf("\nShortcuts (quick actions):\n");
    printf("  @run                Quick run main.xs\n");
    printf("  @build              Quick build all .xs files\n");
    printf("  @clean              Remove build artifacts\n");
    printf("  @test               Run all tests\n");
    printf("  @watch              Watch and rebuild on change\n");
    printf("  @check              Quick lint + type check\n");
    printf("  @info               Show project info\n");
    printf("\nRun 'xsharp help <command>' for more information.\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 0;
    }

    const char* command = argv[1];

    /* Version */
    if (strcmp(command, "version") == 0 || strcmp(command, "--version") == 0 ||
        strcmp(command, "-v") == 0) {
        printf("X# (Xsharp) version %s\n", XS_VERSION_STRING);
        printf("Platform: ");
#if defined(__linux__)
        printf("Linux");
#elif defined(__APPLE__)
        printf("macOS");
#else
        printf("Unknown");
#endif
        printf(" %s\n", sizeof(void*) == 8 ? "x86_64" : "x86");
        return 0;
    }

    /* Help */
    if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 ||
        strcmp(command, "-h") == 0) {
        print_usage();
        return 0;
    }

    /* Initialize runtime */
    xs_runtime_init();

    /* Dispatch to CLI handler */
    int result = xs_cli_dispatch(argc - 1, argv + 1);

    /* Cleanup */
    xs_runtime_shutdown();

    return result;
}
