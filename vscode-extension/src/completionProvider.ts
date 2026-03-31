import * as vscode from 'vscode';

interface FuncInfo { name: string; detail: string; doc: string; snippet?: string; }

const keywords: FuncInfo[] = [
    { name: 'forge', detail: 'function', doc: 'Define a function', snippet: 'forge ${1:name}(${2:params}) -> ${3:type} {\n\t$0\n}' },
    { name: 'entity', detail: 'class', doc: 'Define an entity (class)', snippet: 'entity ${1:Name} {\n\t$0\n}' },
    { name: 'realm', detail: 'namespace', doc: 'Define a namespace', snippet: 'realm ${1:Name} {\n\t$0\n}' },
    { name: 'quest', detail: 'main', doc: 'Main entry point', snippet: 'quest() {\n\t$0\n}' },
    { name: 'morph', detail: 'variable', doc: 'Declare a mutable variable', snippet: 'morph ${1:name}: ${2:blade} = ${3:value}' },
    { name: 'eternal', detail: 'constant', doc: 'Declare a constant', snippet: 'eternal ${1:NAME}: ${2:blade} = ${3:value}' },
    { name: 'oracle', detail: 'if', doc: 'Conditional statement', snippet: 'oracle (${1:condition}) {\n\t$0\n}' },
    { name: 'otherwise', detail: 'else', doc: 'Else branch' },
    { name: 'cycle', detail: 'for', doc: 'For loop', snippet: 'cycle (morph ${1:i} = 0; ${1:i} < ${2:n}; ${1:i} = ${1:i} + 1) {\n\t$0\n}' },
    { name: 'while', detail: 'while loop', doc: 'While loop', snippet: 'while (${1:condition}) {\n\t$0\n}' },
    { name: 'unleash', detail: 'return', doc: 'Return a value' },
    { name: 'engrave', detail: 'print', doc: 'Print to console', snippet: 'engrave(${1:value})' },
    { name: 'summon', detail: 'import', doc: 'Import a module', snippet: 'summon ${1:Module}' },
    { name: 'conjure', detail: 'new', doc: 'Create new instance', snippet: 'conjure ${1:Entity}(${2:args})' },
    { name: 'shield', detail: 'try', doc: 'Try block', snippet: 'shield {\n\t$1\n} deflect (${2:err}) {\n\t$0\n}' },
    { name: 'spell', detail: 'lambda', doc: 'Anonymous function', snippet: 'spell(${1:params}) { ${0:body} }' },
    { name: 'shatter_cycle', detail: 'break', doc: 'Break out of loop' },
    { name: 'skip', detail: 'continue', doc: 'Continue to next iteration' },
    { name: 'shatter', detail: 'throw', doc: 'Throw an exception' },
    { name: '@gpu', detail: 'GPU block', doc: 'GPU compute block', snippet: '@gpu {\n\t$0\n}' },
    { name: '@ai', detail: 'AI block', doc: 'AI scripting block', snippet: '@ai {\n\t$0\n}' },
];

const types: FuncInfo[] = [
    { name: 'blade', detail: 'int64', doc: 'Integer type' },
    { name: 'spark', detail: 'double', doc: 'Float type' },
    { name: 'scroll', detail: 'string', doc: 'String type' },
    { name: 'rune', detail: 'char', doc: 'Character type' },
    { name: 'fate', detail: 'bool', doc: 'Boolean type' },
    { name: 'void', detail: 'void', doc: 'Void type' },
    { name: 'arsenal', detail: 'array', doc: 'Array type' },
    { name: 'truth', detail: 'true', doc: 'Boolean true' },
    { name: 'lies', detail: 'false', doc: 'Boolean false' },
    { name: 'abyss', detail: 'null', doc: 'Null value' },
    { name: 'self', detail: 'this', doc: 'Current instance' },
];

const stdlib: FuncInfo[] = [
    // Math
    { name: 'abs', detail: 'Math', doc: 'Absolute value', snippet: 'abs(${1:x})' },
    { name: 'ceil', detail: 'Math', doc: 'Ceiling', snippet: 'ceil(${1:x})' },
    { name: 'floor', detail: 'Math', doc: 'Floor', snippet: 'floor(${1:x})' },
    { name: 'round', detail: 'Math', doc: 'Round', snippet: 'round(${1:x})' },
    { name: 'sqrt', detail: 'Math', doc: 'Square root', snippet: 'sqrt(${1:x})' },
    { name: 'pow', detail: 'Math', doc: 'Power', snippet: 'pow(${1:base}, ${2:exp})' },
    { name: 'sin', detail: 'Math', doc: 'Sine', snippet: 'sin(${1:x})' },
    { name: 'cos', detail: 'Math', doc: 'Cosine', snippet: 'cos(${1:x})' },
    { name: 'tan', detail: 'Math', doc: 'Tangent', snippet: 'tan(${1:x})' },
    { name: 'asin', detail: 'Math', doc: 'Arc sine' },
    { name: 'acos', detail: 'Math', doc: 'Arc cosine' },
    { name: 'atan', detail: 'Math', doc: 'Arc tangent' },
    { name: 'atan2', detail: 'Math', doc: 'Two-argument arc tangent', snippet: 'atan2(${1:y}, ${2:x})' },
    { name: 'log', detail: 'Math', doc: 'Natural logarithm' },
    { name: 'log2', detail: 'Math', doc: 'Base-2 logarithm' },
    { name: 'log10', detail: 'Math', doc: 'Base-10 logarithm' },
    { name: 'exp', detail: 'Math', doc: 'Exponential (e^x)' },
    { name: 'min', detail: 'Math', doc: 'Minimum of two values', snippet: 'min(${1:a}, ${2:b})' },
    { name: 'max', detail: 'Math', doc: 'Maximum of two values', snippet: 'max(${1:a}, ${2:b})' },
    { name: 'clamp', detail: 'Math', doc: 'Clamp value to range', snippet: 'clamp(${1:x}, ${2:min}, ${3:max})' },
    { name: 'lerp', detail: 'Math', doc: 'Linear interpolation', snippet: 'lerp(${1:a}, ${2:b}, ${3:t})' },
    { name: 'smoothstep', detail: 'Math', doc: 'Smooth Hermite interpolation' },
    { name: 'random', detail: 'Math', doc: 'Random float 0..1' },
    { name: 'randomRange', detail: 'Math', doc: 'Random float in range', snippet: 'randomRange(${1:min}, ${2:max})' },
    { name: 'randomInt', detail: 'Math', doc: 'Random integer in range' },
    { name: 'degToRad', detail: 'Math', doc: 'Degrees to radians' },
    { name: 'radToDeg', detail: 'Math', doc: 'Radians to degrees' },
    { name: 'sign', detail: 'Math', doc: 'Sign of number (-1, 0, 1)' },
    { name: 'fract', detail: 'Math', doc: 'Fractional part' },
    // String
    { name: 'length', detail: 'String', doc: 'String length', snippet: 'length(${1:str})' },
    { name: 'charAt', detail: 'String', doc: 'Character at index', snippet: 'charAt(${1:str}, ${2:index})' },
    { name: 'substring', detail: 'String', doc: 'Extract substring', snippet: 'substring(${1:str}, ${2:start}, ${3:end})' },
    { name: 'indexOf', detail: 'String', doc: 'Find first occurrence' },
    { name: 'contains', detail: 'String', doc: 'Check if contains substring' },
    { name: 'startsWith', detail: 'String', doc: 'Check prefix' },
    { name: 'endsWith', detail: 'String', doc: 'Check suffix' },
    { name: 'toUpper', detail: 'String', doc: 'Convert to uppercase' },
    { name: 'toLower', detail: 'String', doc: 'Convert to lowercase' },
    { name: 'trim', detail: 'String', doc: 'Trim whitespace' },
    { name: 'split', detail: 'String', doc: 'Split string', snippet: 'split(${1:str}, ${2:delim})' },
    { name: 'join', detail: 'String', doc: 'Join array to string', snippet: 'join(${1:arr}, ${2:sep})' },
    { name: 'replace', detail: 'String', doc: 'Replace first occurrence' },
    { name: 'replaceAll', detail: 'String', doc: 'Replace all occurrences' },
    { name: 'repeat', detail: 'String', doc: 'Repeat string N times' },
    // Collections
    { name: 'push', detail: 'Collections', doc: 'Push to array', snippet: 'push(${1:arr}, ${2:val})' },
    { name: 'pop', detail: 'Collections', doc: 'Pop from array' },
    { name: 'shift', detail: 'Collections', doc: 'Remove first element' },
    { name: 'unshift', detail: 'Collections', doc: 'Add to front' },
    { name: 'slice', detail: 'Collections', doc: 'Slice array' },
    { name: 'map', detail: 'Collections', doc: 'Map over array', snippet: 'map(${1:arr}, spell(${2:x}) { $0 })' },
    { name: 'filter', detail: 'Collections', doc: 'Filter array' },
    { name: 'reduce', detail: 'Collections', doc: 'Reduce array' },
    { name: 'forEach', detail: 'Collections', doc: 'Iterate array' },
    { name: 'sort', detail: 'Collections', doc: 'Sort array' },
    { name: 'len', detail: 'Collections', doc: 'Get length' },
    { name: 'range', detail: 'Collections', doc: 'Generate range array' },
    // IO
    { name: 'readLine', detail: 'IO', doc: 'Read line from stdin' },
    { name: 'readFile', detail: 'IO', doc: 'Read file contents', snippet: 'readFile(${1:path})' },
    { name: 'writeFile', detail: 'IO', doc: 'Write to file', snippet: 'writeFile(${1:path}, ${2:data})' },
    { name: 'appendFile', detail: 'IO', doc: 'Append to file' },
    { name: 'fileExists', detail: 'IO', doc: 'Check if file exists' },
    { name: 'deleteFile', detail: 'IO', doc: 'Delete a file' },
    { name: 'listDir', detail: 'IO', doc: 'List directory contents' },
    { name: 'mkdir', detail: 'IO', doc: 'Create directory' },
    // Graphics
    { name: 'createWindow', detail: 'Graphics', doc: 'Create a window', snippet: 'createWindow(${1:title}, ${2:width}, ${3:height})' },
    { name: 'destroyWindow', detail: 'Graphics', doc: 'Destroy window' },
    { name: 'clearScreen', detail: 'Graphics', doc: 'Clear screen', snippet: 'clearScreen(${1:r}, ${2:g}, ${3:b})' },
    { name: 'setColor', detail: 'Graphics', doc: 'Set draw color', snippet: 'setColor(${1:r}, ${2:g}, ${3:b}, ${4:a})' },
    { name: 'drawPixel', detail: 'Graphics', doc: 'Draw a pixel' },
    { name: 'drawLine', detail: 'Graphics', doc: 'Draw a line' },
    { name: 'drawRect', detail: 'Graphics', doc: 'Draw rectangle outline' },
    { name: 'fillRect', detail: 'Graphics', doc: 'Fill rectangle', snippet: 'fillRect(${1:x}, ${2:y}, ${3:w}, ${4:h})' },
    { name: 'drawCircle', detail: 'Graphics', doc: 'Draw circle outline' },
    { name: 'fillCircle', detail: 'Graphics', doc: 'Fill circle' },
    { name: 'drawText', detail: 'Graphics', doc: 'Draw text' },
    { name: 'pollEvents', detail: 'Graphics', doc: 'Poll window events' },
    { name: 'swapBuffers', detail: 'Graphics', doc: 'Swap display buffers' },
    // AI
    { name: 'createNeuralNet', detail: 'AI', doc: 'Create neural network', snippet: 'createNeuralNet([${1:layers}])' },
    { name: 'train', detail: 'AI', doc: 'Train neural network', snippet: 'train(${1:net}, ${2:inputs}, ${3:targets}, ${4:epochs})' },
    { name: 'predict', detail: 'AI', doc: 'Run prediction', snippet: 'predict(${1:net}, ${2:input})' },
    { name: 'sigmoid', detail: 'AI', doc: 'Sigmoid activation' },
    { name: 'relu', detail: 'AI', doc: 'ReLU activation' },
    { name: 'softmax', detail: 'AI', doc: 'Softmax activation' },
    // Crypto
    { name: 'sha256', detail: 'Crypto', doc: 'SHA-256 hash' },
    { name: 'base64Encode', detail: 'Crypto', doc: 'Base64 encode' },
    { name: 'base64Decode', detail: 'Crypto', doc: 'Base64 decode' },
    // Thread
    { name: 'spawn', detail: 'Thread', doc: 'Spawn a new thread' },
    { name: 'mutex_new', detail: 'Thread', doc: 'Create a mutex' },
    { name: 'channel_new', detail: 'Thread', doc: 'Create a channel' },
    // System
    { name: 'exec', detail: 'System', doc: 'Execute shell command' },
    { name: 'getEnv', detail: 'System', doc: 'Get environment variable' },
    { name: 'platform', detail: 'System', doc: 'Get platform name' },
    { name: 'cpuCount', detail: 'System', doc: 'Get CPU core count' },
    // Time
    { name: 'now', detail: 'Time', doc: 'Current time' },
    { name: 'timestamp', detail: 'Time', doc: 'Unix timestamp' },
    { name: 'startTimer', detail: 'Time', doc: 'Start a timer' },
    { name: 'elapsed', detail: 'Time', doc: 'Get elapsed time' },
    // Physics
    { name: 'createWorld', detail: 'Physics', doc: 'Create physics world' },
    { name: 'addBody', detail: 'Physics', doc: 'Add physics body' },
    { name: 'applyForce', detail: 'Physics', doc: 'Apply force to body' },
    { name: 'stepSimulation', detail: 'Physics', doc: 'Step physics simulation' },
    // Audio
    { name: 'initAudio', detail: 'Audio', doc: 'Initialize audio system' },
    { name: 'loadSound', detail: 'Audio', doc: 'Load sound file' },
    { name: 'playSound', detail: 'Audio', doc: 'Play a sound' },
    // Net
    { name: 'httpGet', detail: 'Net', doc: 'HTTP GET request', snippet: 'httpGet(${1:url})' },
    { name: 'httpPost', detail: 'Net', doc: 'HTTP POST request' },
    { name: 'tcpConnect', detail: 'Net', doc: 'TCP connect' },
    // Regex
    { name: 'regexMatch', detail: 'Regex', doc: 'Match regex pattern' },
    { name: 'regexReplace', detail: 'Regex', doc: 'Replace with regex' },
    { name: 'regexTest', detail: 'Regex', doc: 'Test if pattern matches' },
    // Filesystem
    { name: 'fsGlob', detail: 'Filesystem', doc: 'Glob file patterns' },
    { name: 'fsJoinPath', detail: 'Filesystem', doc: 'Join path components' },
    { name: 'fsBasename', detail: 'Filesystem', doc: 'Get file basename' },
];

export function getCompletionItems(): vscode.CompletionItem[] {
    const items: vscode.CompletionItem[] = [];

    for (const kw of keywords) {
        const item = new vscode.CompletionItem(kw.name, vscode.CompletionItemKind.Keyword);
        item.detail = `X# keyword (${kw.detail})`;
        item.documentation = new vscode.MarkdownString(kw.doc);
        if (kw.snippet) item.insertText = new vscode.SnippetString(kw.snippet);
        items.push(item);
    }

    for (const t of types) {
        const item = new vscode.CompletionItem(t.name, vscode.CompletionItemKind.TypeParameter);
        item.detail = `X# type (${t.detail})`;
        item.documentation = new vscode.MarkdownString(t.doc);
        items.push(item);
    }

    for (const fn of stdlib) {
        const item = new vscode.CompletionItem(fn.name, vscode.CompletionItemKind.Function);
        item.detail = `X# stdlib - ${fn.detail}`;
        item.documentation = new vscode.MarkdownString(fn.doc);
        if (fn.snippet) item.insertText = new vscode.SnippetString(fn.snippet);
        items.push(item);
    }

    return items;
}
