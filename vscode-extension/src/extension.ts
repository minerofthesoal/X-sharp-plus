import * as vscode from 'vscode';
import { getCompletionItems } from './completionProvider';
import { XsDebugAdapterFactory } from './debugAdapter';

export function activate(context: vscode.ExtensionContext) {
    console.log('X# (Xsharp) extension activated');

    // Commands
    context.subscriptions.push(
        vscode.commands.registerCommand('xsharp.run', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) return;
            const terminal = vscode.window.createTerminal('X# Run');
            terminal.show();
            terminal.sendText(`xsharp run "${editor.document.fileName}"`);
        }),
        vscode.commands.registerCommand('xsharp.build', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) return;
            const terminal = vscode.window.createTerminal('X# Build');
            terminal.show();
            terminal.sendText(`xsharp build "${editor.document.fileName}"`);
        }),
        vscode.commands.registerCommand('xsharp.debug', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) return;
            vscode.debug.startDebugging(undefined, {
                type: 'xsharp', request: 'launch',
                name: 'Debug X#', program: editor.document.fileName
            });
        }),
        vscode.commands.registerCommand('xsharp.format', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) return;
            const terminal = vscode.window.createTerminal('X# Format');
            terminal.sendText(`xsharp fmt "${editor.document.fileName}"`);
        }),
        vscode.commands.registerCommand('xsharp.lint', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor) return;
            const terminal = vscode.window.createTerminal('X# Lint');
            terminal.show();
            terminal.sendText(`xsharp lint "${editor.document.fileName}"`);
        })
    );

    // Completion Provider
    const completionProvider = vscode.languages.registerCompletionItemProvider(
        { language: 'xsharp', scheme: 'file' },
        {
            provideCompletionItems(document: vscode.TextDocument, position: vscode.Position) {
                return getCompletionItems();
            }
        },
        '.', '@'
    );
    context.subscriptions.push(completionProvider);

    // Hover Provider
    const hoverProvider = vscode.languages.registerHoverProvider(
        { language: 'xsharp' },
        {
            provideHover(document, position) {
                const range = document.getWordRangeAtPosition(position);
                if (!range) return;
                const word = document.getText(range);
                const docs: Record<string, string> = {
                    'forge': '**forge** - Define a function\n```\nforge name(params) -> ReturnType { ... }\n```',
                    'entity': '**entity** - Define a class\n```\nentity Name : Parent { ... }\n```',
                    'realm': '**realm** - Define a namespace\n```\nrealm Name { ... }\n```',
                    'quest': '**quest** - Main entry point\n```\nquest() { ... }\n```',
                    'morph': '**morph** - Declare a mutable variable\n```\nmorph x: blade = 42\n```',
                    'eternal': '**eternal** - Declare a constant\n```\neternal PI: spark = 3.14159\n```',
                    'oracle': '**oracle** - Conditional (if)\n```\noracle (condition) { ... }\n```',
                    'otherwise': '**otherwise** - Else branch',
                    'cycle': '**cycle** - For loop\n```\ncycle (morph i = 0; i < 10; i = i + 1) { ... }\n```',
                    'unleash': '**unleash** - Return a value',
                    'engrave': '**engrave** - Print to console',
                    'summon': '**summon** - Import a module',
                    'conjure': '**conjure** - Create new instance',
                    'shield': '**shield/deflect** - Try/catch block',
                    'spell': '**spell** - Lambda/anonymous function',
                    'blade': '**blade** - Integer type (int64)',
                    'spark': '**spark** - Float type (double)',
                    'scroll': '**scroll** - String type',
                    'fate': '**fate** - Boolean type',
                    'truth': '**truth** - Boolean true',
                    'lies': '**lies** - Boolean false',
                    'abyss': '**abyss** - Null value',
                    'arsenal': '**arsenal** - Array type',
                };
                if (docs[word]) return new vscode.Hover(new vscode.MarkdownString(docs[word]));
                return undefined;
            }
        }
    );
    context.subscriptions.push(hoverProvider);

    // Document Symbol Provider
    const symbolProvider = vscode.languages.registerDocumentSymbolProvider(
        { language: 'xsharp' },
        {
            provideDocumentSymbols(document) {
                const symbols: vscode.DocumentSymbol[] = [];
                const text = document.getText();
                const patterns: [RegExp, vscode.SymbolKind, string][] = [
                    [/\bforge\s+(\w+)\s*\(/g, vscode.SymbolKind.Function, 'forge'],
                    [/\bentity\s+(\w+)/g, vscode.SymbolKind.Class, 'entity'],
                    [/\brealm\s+(\w+)/g, vscode.SymbolKind.Namespace, 'realm'],
                    [/\bquest\s*\(\)/g, vscode.SymbolKind.Function, 'quest'],
                ];
                for (const [re, kind, prefix] of patterns) {
                    let m;
                    while ((m = re.exec(text)) !== null) {
                        const pos = document.positionAt(m.index);
                        const endPos = document.positionAt(m.index + m[0].length);
                        const range = new vscode.Range(pos, endPos);
                        const name = m[1] || prefix;
                        symbols.push(new vscode.DocumentSymbol(
                            name, prefix, kind, range, range
                        ));
                    }
                }
                return symbols;
            }
        }
    );
    context.subscriptions.push(symbolProvider);

    // Document Formatting Provider
    const formatProvider = vscode.languages.registerDocumentFormattingEditProvider(
        { language: 'xsharp' },
        {
            provideDocumentFormattingEdits(document) {
                // Basic formatting: ensure consistent indentation
                const edits: vscode.TextEdit[] = [];
                let indent = 0;
                for (let i = 0; i < document.lineCount; i++) {
                    const line = document.lineAt(i);
                    const trimmed = line.text.trim();
                    if (trimmed.startsWith('}') || trimmed.startsWith(']')) indent = Math.max(0, indent - 1);
                    const expected = '    '.repeat(indent) + trimmed;
                    if (line.text !== expected && trimmed.length > 0) {
                        edits.push(vscode.TextEdit.replace(line.range, expected));
                    }
                    if (trimmed.endsWith('{') || trimmed.endsWith('[')) indent++;
                }
                return edits;
            }
        }
    );
    context.subscriptions.push(formatProvider);

    // Diagnostics
    const diagnostics = vscode.languages.createDiagnosticCollection('xsharp');
    context.subscriptions.push(diagnostics);

    // Watch for file saves to run linting
    context.subscriptions.push(
        vscode.workspace.onDidSaveTextDocument((doc) => {
            if (doc.languageId !== 'xsharp') return;
            // Clear old diagnostics
            diagnostics.set(doc.uri, []);
        })
    );

    // Debug Adapter
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory('xsharp', new XsDebugAdapterFactory())
    );

    // Status Bar
    const statusBar = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    statusBar.text = '$(zap) X# v0.1.0';
    statusBar.tooltip = 'X# (Xsharp) Language';
    statusBar.command = 'xsharp.run';
    statusBar.show();
    context.subscriptions.push(statusBar);
}

export function deactivate() {}
