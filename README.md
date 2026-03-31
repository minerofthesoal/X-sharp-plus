# X# (Xsharp) Programming Language

**A high-performance programming language built for game development and AI scripting.**

X# combines the power of systems programming with the expressiveness of modern scripting languages. It features a unique fantasy-themed syntax, built-in rendering engine, GPU compute support, neural network primitives, and a complete development ecosystem.

---

## Features

- **Custom Syntax** — Fantasy-themed keywords (`forge`, `entity`, `realm`, `quest`) that make game code read like an adventure
- **Built-in Rendering Engine** — Software rasterizer with 3D math, scene graphs, mesh loading, and lighting
- **GPU Compute** — `@gpu { }` blocks for parallel computation on the GPU via OpenCL
- **AI Primitives** — `@ai { }` blocks with built-in neural network creation, training, and inference
- **150+ Standard Library Functions** — Math, IO, graphics, AI, networking, audio, physics, crypto, threading, and more
- **Interactive Debugger** — Breakpoints, step execution, variable inspection, DAP protocol support
- **Native IDE** — GTK3-based desktop application with syntax highlighting, project management, and integrated debugger
- **VS Code Extension** — Full language support with syntax highlighting, auto-completion, debugging, and snippets
- **CLI with Shortcuts** — 25+ commands including REPL, formatter, linter, and quick-action `@shortcuts`
- **Custom File Formats** — `.Xssc` (source container) and `.Xscsc` (LZMA2 compressed) archive formats
- **Full Unix Support** — Linux and macOS, built with POSIX APIs
- **CPU & GPU Execution** — Runs on both CPU (stack-based VM) and GPU (OpenCL compute dispatch)

---

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/minerofthesoal/x-sharp-plus.git
cd x-sharp-plus

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Or use Make
make release

# Install system-wide (optional)
sudo make install
```

### Hello World

Create a file called `hello.xs`:

```xsharp
~~ My first X# program
quest() {
    engrave("Hello, World from X#!")
}
```

Run it:

```bash
xsharp run hello.xs
```

### Quick Start with Templates

```bash
# Create a new game project
xsharp new my_game --template game

# Create an AI project
xsharp new my_ai --template ai

# Create a simple console app
xsharp new my_app --template console
```

---

## Language Syntax

### Variables and Types

```xsharp
~~ Mutable variable
morph health: blade = 100
morph name: scroll = "Hero"
morph speed: spark = 3.14
morph alive: fate = truth
morph nothing = abyss

~~ Constant
eternal MAX_LEVEL: blade = 99
eternal PI: spark = 3.14159
```

### Functions

```xsharp
forge greet(name: scroll) -> scroll {
    unleash "Hello, #{name}!"
}

forge add(a: blade, b: blade) -> blade {
    unleash a + b
}

~~ Lambda / anonymous function
morph double = spell(x: blade) -> blade { unleash x * 2 }
```

### Entities (Classes)

```xsharp
entity Player {
    morph name: scroll
    morph health: blade
    morph x: spark
    morph y: spark

    forge create(name: scroll) -> Player {
        morph p = conjure Player
        p.name = name
        p.health = 100
        p.x = 0.0
        p.y = 0.0
        unleash p
    }

    forge takeDamage(self, amount: blade) {
        self.health = self.health - amount
        oracle (self.health <= 0) {
            engrave("#{self.name} has fallen!")
        }
    }

    forge move(self, dx: spark, dy: spark) {
        self.x = self.x + dx
        self.y = self.y + dy
    }
}

~~ Inheritance
entity Warrior : Player {
    morph armor: blade

    forge attack(self, target: Player) {
        target.takeDamage(25 - target.armor)
    }
}
```

### Control Flow

```xsharp
~~ Conditionals
oracle (score > 90) {
    engrave("Excellent!")
} otherwise oracle (score > 70) {
    engrave("Good!")
} otherwise {
    engrave("Keep trying!")
}

~~ For loop
cycle (morph i = 0; i < 10; i = i + 1) {
    engrave("Iteration #{i}")
}

~~ While loop
while (running) {
    update()
    render()
}

~~ Loop control
cycle (morph i = 0; i < 100; i = i + 1) {
    oracle (i % 2 == 0) { skip }     ~~ continue
    oracle (i > 50) { shatter_cycle } ~~ break
    engrave(i)
}
```

### Error Handling

```xsharp
shield {
    morph data = readFile("config.xs")
    processData(data)
} deflect (err) {
    engrave("Error: #{err}")
}
```

### Pipe Operator

```xsharp
morph result = getData()
    |> filter(spell(x) { unleash x > 0 })
    |> map(spell(x) { unleash x * 2 })
    |> reduce(0, spell(acc, x) { unleash acc + x })
```

### GPU Compute

```xsharp
@gpu {
    forge parallelAdd(a: arsenal[spark], b: arsenal[spark], out: arsenal[spark], n: blade) {
        morph idx = gpu_thread_id()
        oracle (idx < n) {
            out[idx] = a[idx] + b[idx]
        }
    }
}
```

### AI Blocks

```xsharp
@ai {
    morph net = createNeuralNet([2, 4, 1])

    morph inputs = [[0,0], [0,1], [1,0], [1,1]]
    morph targets = [[0], [1], [1], [0]]

    train(net, inputs, targets, epochs: 1000, lr: 0.1)

    morph prediction = predict(net, [1, 0])
    engrave("Prediction: #{prediction}")
}
```

### Namespaces

```xsharp
realm GameEngine {
    entity Scene { ... }
    entity Camera { ... }

    forge init() { ... }
}

summon GameEngine.Scene
summon GameEngine.Camera
```

### Comments

```xsharp
~~ This is a single-line comment

~*
  This is a
  multi-line comment
*~
```

---

## Standard Library

### Math (35 functions)
`abs`, `ceil`, `floor`, `round`, `sqrt`, `pow`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `log`, `log2`, `log10`, `exp`, `min`, `max`, `clamp`, `lerp`, `inverseLerp`, `remap`, `sign`, `fract`, `step`, `smoothstep`, `random`, `randomRange`, `randomInt`, `degToRad`, `radToDeg`, `PI`, `E`, `TAU`

### String (22 functions)
`length`, `charAt`, `substring`, `indexOf`, `lastIndexOf`, `contains`, `startsWith`, `endsWith`, `toUpper`, `toLower`, `trim`, `trimStart`, `trimEnd`, `split`, `join`, `replace`, `replaceAll`, `repeat`, `reverse`, `padStart`, `padEnd`, `format`

### Collections (25 functions)
`push`, `pop`, `shift`, `unshift`, `slice`, `splice`, `concat`, `map`, `filter`, `reduce`, `forEach`, `find`, `findIndex`, `sort`, `reverse`, `includes`, `indexOf`, `flat`, `zip`, `enumerate`, `range`, `len`, `hashmap_new`, `hashmap_set`, `hashmap_get`

### IO (16 functions)
`engrave`, `engraveLn`, `readLine`, `readChar`, `readFile`, `writeFile`, `appendFile`, `fileExists`, `deleteFile`, `listDir`, `mkdir`, `rmdir`, `copyFile`, `moveFile`, `fileSize`, `cwd`

### Graphics (19 functions)
`createWindow`, `destroyWindow`, `clearScreen`, `setColor`, `drawPixel`, `drawLine`, `drawRect`, `fillRect`, `drawCircle`, `fillCircle`, `drawTriangle`, `fillTriangle`, `drawText`, `loadImage`, `drawImage`, `getScreenWidth`, `getScreenHeight`, `pollEvents`, `swapBuffers`

### AI (17 functions)
`createNeuralNet`, `addLayer`, `train`, `predict`, `backpropagate`, `setLearningRate`, `saveModel`, `loadModel`, `createMatrix`, `matMul`, `matAdd`, `matTranspose`, `sigmoid`, `relu`, `softmax`, `crossEntropy`, `mse`

### Networking (11 functions)
`httpGet`, `httpPost`, `tcpConnect`, `tcpListen`, `tcpSend`, `tcpRecv`, `tcpClose`, `udpSend`, `udpRecv`, `urlEncode`, `urlDecode`

### System (11 functions)
`exec`, `getEnv`, `setEnv`, `exit`, `sleep`, `time`, `clock`, `platform`, `arch`, `cpuCount`, `memoryUsage`

### Audio (10 functions)
`initAudio`, `loadSound`, `playSound`, `stopSound`, `pauseSound`, `setVolume`, `setPan`, `createOscillator`, `setFrequency`, `closeAudio`

### Physics (12 functions)
`createWorld`, `addBody`, `removeBody`, `applyForce`, `applyImpulse`, `setGravity`, `stepSimulation`, `raycast`, `checkCollision`, `setMass`, `setFriction`, `setBounce`

### Crypto (9 functions)
`sha256`, `sha512`, `md5`, `hmac`, `randomBytes`, `base64Encode`, `base64Decode`, `aesEncrypt`, `aesDecrypt`

### Threading (11 functions)
`spawn`, `join`, `detach`, `mutex_new`, `mutex_lock`, `mutex_unlock`, `channel_new`, `channel_send`, `channel_recv`, `atomic_inc`, `atomic_dec`

### Time (8 functions)
`now`, `timestamp`, `format`, `parse`, `addDuration`, `diffTime`, `startTimer`, `elapsed`

### Regex (6 functions)
`compile`, `match`, `matchAll`, `replace`, `split`, `test`

### Filesystem (10 functions)
`watch`, `glob`, `realpath`, `basename`, `dirname`, `extname`, `joinPath`, `isAbsolute`, `tempDir`, `tempFile`

**Total: 212 functions**

---

## CLI Commands

```bash
xsharp run <file.xs>        # Run a program
xsharp build <file.xs>      # Compile to bytecode
xsharp debug <file.xs>      # Run with debugger attached
xsharp repl                 # Interactive REPL
xsharp fmt <file.xs>        # Format source code
xsharp lint <file.xs>       # Lint and check code
xsharp test <dir>           # Run test files
xsharp doc <file.xs>        # Generate documentation
xsharp new <name>           # Create new project
xsharp init                 # Initialize project in current directory
xsharp pack <file.xs>       # Package to .Xssc
xsharp unpack <file.Xssc>   # Extract .Xssc archive
xsharp compress <file>      # Compress to .Xscsc (LZMA2)
xsharp decompress <file>    # Decompress .Xscsc
xsharp version              # Show version info
xsharp help                 # Show help
```

### Shortcuts (Quick Actions)

Shortcuts are prefixed with `@` for rapid actions:

```bash
xsharp @run          # Quick run main.xs in current directory
xsharp @build        # Quick build all .xs files
xsharp @clean        # Remove build artifacts
xsharp @test         # Run all tests in ./tests/
xsharp @bench        # Run benchmarks
xsharp @watch        # Watch files and rebuild on change
xsharp @serve        # Dev server with hot reload
xsharp @profile      # Run with profiler
xsharp @size         # Show compiled binary size
xsharp @deps         # Show dependency tree
xsharp @info         # Show project info
xsharp @check        # Quick lint + type check
```

Custom shortcuts can be defined in `.xsharprc`:

```ini
[shortcuts]
deploy = build && pack && upload
my_test = test ./tests/ --verbose
```

---

## File Formats

### .Xssc (X# Source Container)

A custom binary+text hybrid archive format for packaging X# projects:

- **Not JSON** — Custom binary format with magic bytes, entry tables, and CRC32 checksums
- Stores source files, assets, config, and compiled bytecode
- Metadata key-value pairs for project information
- Integrity verification via CRC32 checksums

```bash
# Create archive
xsharp pack my_project/ -o game.Xssc

# List contents
xsharp pack --list game.Xssc

# Extract
xsharp unpack game.Xssc -o output/
```

### .Xscsc (X# Compressed Source Container)

LZMA2-compressed version of .Xssc for distribution:

- Uses **LZMA2 compression** (LZ77 + range coding)
- Configurable dictionary size and compression level
- Significantly smaller than .Xssc for distribution

```bash
# Compress
xsharp compress game.Xssc -o game.Xscsc --level 6

# Decompress
xsharp decompress game.Xscsc -o game.Xssc
```

---

## Rendering Engine

X# includes a built-in software rendering engine:

- **Software Rasterizer** — Scanline triangle rasterization with Z-buffering
- **3D Math** — Vectors, matrices, quaternions, transforms
- **Scene Graph** — Hierarchical node system with transform inheritance
- **Mesh System** — Primitives (cube, sphere, plane), OBJ loading
- **Lighting** — Directional, point, and spot lights with Phong shading
- **Textures** — Texture mapping with bilinear filtering and mipmaps
- **Camera** — Perspective and orthographic projection
- **Shaders** — Programmable vertex/fragment shaders (as function pointers)
- **GPU Dispatch** — Optional OpenCL compute backend

```xsharp
summon Graphics

quest() {
    morph win = createWindow("My Game", 800, 600)

    while (truth) {
        clearScreen(0, 0, 0)

        setColor(255, 0, 0, 255)
        fillRect(100, 100, 200, 150)

        setColor(0, 255, 0, 255)
        drawCircle(400, 300, 50)

        swapBuffers(win)
        pollEvents(win)
    }

    destroyWindow(win)
}
```

---

## Debugger

Full interactive debugging with DAP (Debug Adapter Protocol) support:

- **Breakpoints** — Line, conditional, and hit-count breakpoints
- **Stepping** — Step in, step over, step out, continue, run to cursor
- **Inspection** — Variables, call stack, watch expressions, memory
- **IDE Integration** — Works with the native IDE and VS Code extension

```bash
# Start debugger
xsharp debug my_program.xs

# Debugger commands
(xdb) break main.xs:10        # Set breakpoint
(xdb) break main.xs:15 if x>5 # Conditional breakpoint
(xdb) continue                 # Run until breakpoint
(xdb) step                     # Step into
(xdb) next                     # Step over
(xdb) out                      # Step out
(xdb) print x                  # Print variable
(xdb) watch x + y              # Watch expression
(xdb) stack                    # Show call stack
(xdb) locals                   # Show local variables
(xdb) quit                     # Exit debugger
```

---

## IDE

### Native IDE (X# Forge)

A GTK3-based desktop application:

- **Code Editor** — Syntax highlighting, line numbers, code folding, bracket matching
- **Project Explorer** — File tree with create/delete/rename
- **Console** — Build output, program output, error display
- **Debug Panel** — Variables, call stack, breakpoints
- **Themes** — "Obsidian" (dark) and "Radiance" (light)
- **Build Integration** — One-click build, run, and debug

```bash
# Launch the IDE
xsharp-ide

# Open a project
xsharp-ide my_project/
```

### VS Code Extension

Install from the VS Code marketplace or from a `.vsix` file:

- Syntax highlighting for `.xs` files
- Auto-completion for all 212 standard library functions
- 30+ code snippets
- Integrated debugging
- Build tasks
- "X# Obsidian" color theme
- Hover documentation
- Go to definition
- Format document
- Diagnostics/linting

---

## Building from Source

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install build-essential cmake pkg-config \
  libgtk-3-dev libgtksourceview-3.0-dev libreadline-dev \
  ocl-icd-opencl-dev libasound2-dev
```

**macOS:**
```bash
brew install cmake pkg-config gtk+3 gtksourceview3 readline
```

### Build Options

| CMake Option       | Default | Description                    |
|--------------------|---------|--------------------------------|
| `XS_ENABLE_GPU`    | ON      | Enable OpenCL GPU compute      |
| `XS_ENABLE_IDE`    | ON      | Build the native IDE           |
| `XS_BUILD_TESTS`   | OFF     | Build test suite               |
| `XS_ENABLE_AUDIO`  | ON      | Enable audio subsystem         |

```bash
# Full build with all features
cmake -B build -DCMAKE_BUILD_TYPE=Release -DXS_BUILD_TESTS=ON
cmake --build build -j$(nproc)

# Minimal build (no GPU, no IDE)
cmake -B build -DXS_ENABLE_GPU=OFF -DXS_ENABLE_IDE=OFF
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure
```

---

## Project Structure

```
x-sharp-plus/
├── src/
│   ├── lexer/          # Tokenizer
│   ├── parser/         # Recursive descent parser
│   ├── ast/            # Abstract syntax tree
│   ├── compiler/       # Bytecode compiler
│   ├── codegen/        # Code generation utilities
│   ├── vm/             # Stack-based virtual machine
│   ├── runtime/        # Runtime system (values, GC, types)
│   ├── debugger/       # Interactive debugger + DAP
│   ├── cli/            # CLI commands and shortcuts
│   ├── renderer/       # Rendering engine
│   │   ├── core/       # Rasterizer, textures, math, pipeline
│   │   ├── gpu/        # GPU compute backend
│   │   ├── shaders/    # Software shader system
│   │   └── scene/      # Scene graph
│   ├── formats/        # .Xssc and .Xscsc file formats
│   └── ide/            # Native GTK3 IDE
│       ├── core/       # Application, project management
│       ├── editor/     # Code editor, syntax
│       ├── panels/     # Project explorer, console, debug
│       └── themes/     # Obsidian, Radiance themes
├── stdlib/             # Standard library (150+ functions)
│   ├── math/           ├── string/       ├── collections/
│   ├── io/             ├── graphics/     ├── ai/
│   ├── net/            ├── system/       ├── audio/
│   ├── physics/        ├── crypto/       ├── thread/
│   ├── time/           ├── regex/        └── fs/
├── vscode-extension/   # VS Code extension
├── examples/           # Example programs
├── tests/              # Test suite
├── docs/               # Documentation
├── scripts/            # Build and utility scripts
├── .github/workflows/  # CI/CD pipelines
├── CMakeLists.txt      # CMake build system
├── Makefile            # Alternative Make build
└── README.md
```

---

## Examples

See the `examples/` directory for complete programs:

| File                | Description                              |
|---------------------|------------------------------------------|
| `hello.xs`          | Hello World                              |
| `game_loop.xs`      | Game loop with rendering                 |
| `neural_net.xs`     | Neural network training                  |
| `fibonacci.xs`      | Fibonacci (recursive + iterative)        |
| `entity_demo.xs`    | Classes, inheritance, polymorphism       |
| `gpu_compute.xs`    | GPU parallel computation                 |
| `file_io.xs`        | File reading and writing                 |
| `physics_demo.xs`   | Physics simulation                       |

---

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m "Add my feature"`
4. Push to the branch: `git push origin feature/my-feature`
5. Open a Pull Request

Please ensure:
- Code compiles without warnings (`-Wall -Wextra -Werror`)
- All tests pass
- New features include tests
- Code follows the existing style

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Acknowledgments

X# is built with passion for game development and AI. Special thanks to the open-source community for the tools and libraries that make this possible.

**Happy forging!**
