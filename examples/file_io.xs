~~ File I/O Example in X#
~~ Demonstrates reading, writing, and manipulating files

summon File from "stdlib/file"
summon Path from "stdlib/path"

~~ Write a text file
forge write_example() {
    engrave("--- Writing Files ---")

    ~~ Write a simple text file
    morph content: scroll = "Hello from X#!\nThis is line 2.\nAnd line 3.\n"
    File.write("output/hello.txt", content)
    engrave("  Wrote output/hello.txt")

    ~~ Write with append
    File.append("output/hello.txt", "Appended line 4!\n")
    engrave("  Appended to output/hello.txt")

    ~~ Write binary data
    morph bytes: arsenal[blade] = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]
    File.write_bytes("output/header.bin", bytes)
    engrave("  Wrote output/header.bin (#{bytes.length()} bytes)")
}

~~ Read a text file
forge read_example() {
    engrave("")
    engrave("--- Reading Files ---")

    ~~ Read entire file as string
    morph content: scroll = File.read("output/hello.txt")
    engrave("  Contents of hello.txt:")
    engrave("  ----")
    engrave("  #{content}")
    engrave("  ----")

    ~~ Read file line by line
    morph lines: arsenal[scroll] = File.read_lines("output/hello.txt")
    engrave("  Line count: #{lines.length()}")
    cycle (morph i: blade = 0; i < lines.length(); i++) {
        engrave("  Line #{i + 1}: #{lines[i]}")
    }

    ~~ Read binary data
    morph bytes: arsenal[blade] = File.read_bytes("output/header.bin")
    engrave("  Binary file size: #{bytes.length()} bytes")
    morph hex: scroll = ""
    cycle (morph i: blade = 0; i < bytes.length(); i++) {
        hex = hex + "0x" + bytes[i].to_hex() + " "
    }
    engrave("  Hex: #{hex}")
}

~~ CSV file handling
forge csv_example() {
    engrave("")
    engrave("--- CSV File Handling ---")

    ~~ Write a CSV file
    morph csv_content: scroll = "Name,Age,Score\n"
    csv_content = csv_content + "Alice,25,95.5\n"
    csv_content = csv_content + "Bob,30,87.3\n"
    csv_content = csv_content + "Charlie,22,91.8\n"
    csv_content = csv_content + "Diana,28,99.1\n"

    File.write("output/data.csv", csv_content)
    engrave("  Wrote output/data.csv")

    ~~ Read and parse CSV
    morph lines: arsenal[scroll] = File.read_lines("output/data.csv")
    morph header: arsenal[scroll] = lines[0].split(",")
    engrave("  Columns: #{header[0]}, #{header[1]}, #{header[2]}")

    morph total_score: spark = 0.0
    cycle (morph i: blade = 1; i < lines.length(); i++) {
        morph fields: arsenal[scroll] = lines[i].split(",")
        oracle (fields.length() >= 3) {
            morph name: scroll = fields[0]
            morph age: scroll = fields[1]
            morph score: spark = fields[2] as spark
            total_score = total_score + score
            engrave("  #{name} (age #{age}): score #{score}")
        }
    }

    morph avg: spark = total_score / (lines.length() - 1) as spark
    engrave("  Average score: #{avg}")
}

~~ File system operations
forge filesystem_example() {
    engrave("")
    engrave("--- File System Operations ---")

    ~~ Check existence
    engrave("  hello.txt exists: #{File.exists("output/hello.txt")}")
    engrave("  ghost.txt exists: #{File.exists("output/ghost.txt")}")

    ~~ File size
    morph size: blade = File.size("output/hello.txt")
    engrave("  hello.txt size: #{size} bytes")

    ~~ Path operations
    morph full_path: scroll = "output/subdir/myfile.xs"
    engrave("  Path: #{full_path}")
    engrave("    Directory: #{Path.dirname(full_path)}")
    engrave("    Filename:  #{Path.basename(full_path)}")
    engrave("    Extension: #{Path.extension(full_path)}")
    engrave("    Stem:      #{Path.stem(full_path)}")

    ~~ Directory listing
    engrave("  Files in output/:")
    morph files: arsenal[scroll] = File.list_dir("output")
    cycle (morph i: blade = 0; i < files.length(); i++) {
        morph fpath: scroll = "output/" + files[i]
        morph fsize: blade = File.size(fpath)
        engrave("    #{files[i]} (#{fsize} bytes)")
    }

    ~~ Copy and rename
    File.copy("output/hello.txt", "output/hello_copy.txt")
    engrave("  Copied hello.txt -> hello_copy.txt")

    ~~ Delete a file
    File.delete("output/hello_copy.txt")
    engrave("  Deleted hello_copy.txt")
}

~~ Structured data file
forge structured_data_example() {
    engrave("")
    engrave("--- Structured Data (Config File) ---")

    ~~ Write a simple key=value config file
    morph config: scroll = ""
    config = config + "# X# Application Config\n"
    config = config + "app.name=MyApp\n"
    config = config + "app.version=1.0.0\n"
    config = config + "window.width=800\n"
    config = config + "window.height=600\n"
    config = config + "render.fps=60\n"
    config = config + "debug.enabled=false\n"

    File.write("output/app.cfg", config)
    engrave("  Wrote output/app.cfg")

    ~~ Read and parse config
    morph lines: arsenal[scroll] = File.read_lines("output/app.cfg")
    engrave("  Configuration:")
    cycle (morph i: blade = 0; i < lines.length(); i++) {
        morph line: scroll = lines[i].trim()
        ~~ Skip comments and empty lines
        oracle (line.length() == 0 || line.startsWith("#")) {
            skip
        }
        morph parts: arsenal[scroll] = line.split("=")
        oracle (parts.length() >= 2) {
            engrave("    #{parts[0]} = #{parts[1]}")
        }
    }
}

quest() {
    engrave("=== X# File I/O Examples ===")
    engrave("")

    ~~ Create output directory
    File.mkdir("output")

    write_example()
    read_example()
    csv_example()
    filesystem_example()
    structured_data_example()

    engrave("")
    engrave("File I/O demo complete!")
}
