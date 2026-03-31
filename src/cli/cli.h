/*
 * X# (Xsharp) CLI - Command Line Interface
 * ==========================================
 */

#ifndef XSHARP_CLI_H
#define XSHARP_CLI_H

/* Dispatch a CLI command. Returns exit code. */
int xs_cli_dispatch(int argc, char **argv);

/* Runtime init/shutdown (declared in runtime.h, defined in runtime.c) */
void xs_runtime_init(void);
void xs_runtime_shutdown(void);

#endif /* XSHARP_CLI_H */
