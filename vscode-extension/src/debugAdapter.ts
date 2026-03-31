import * as vscode from 'vscode';

export class XsDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory {
    createDebugAdapterDescriptor(
        session: vscode.DebugSession,
        executable: vscode.DebugAdapterExecutable | undefined
    ): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
        const config = session.configuration;
        const program = config.program || '';
        const xsharpPath = vscode.workspace.getConfiguration('xsharp').get<string>('path', 'xsharp');
        return new vscode.DebugAdapterExecutable(xsharpPath, ['debug', '--dap', program]);
    }
}
