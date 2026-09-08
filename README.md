# C-Hex: TUI Hex Editor

A lightweight, zero-dependency, cross-platform terminal hex editor and binary viewer written entirely in C. 

Built for reverse engineering, binary analysis, and low-level memory manipulation, this tool features direct-to-disk editing, custom VT sequence rendering, and overlapping buffer management for instantaneous navigation on both POSIX and Windows systems.

![HexEdit Screenshot](/media/index.png)

## Features

* **True Cross-Platform:** Runs flawlessly on Windows, macOS, and Linux. Uses native `<windows.h>` API for Windows and `<termios.h>` for POSIX systems.
* **Direct Disk I/O:** Edits are written directly to the physical disk via `fseek` and `fputc`. It does not load the entire file into RAM, allowing it to easily handle massive binary files.
* **Zero Dependencies:** Written in pure standard C. No third-party libraries like `ncurses` required.
* **Custom TUI Engine:** Hijacks the terminal's alternate buffer mode and disables canonical input buffering for instantaneous rendering and zero-latency keystrokes.
* **Three Viewing Modes:** Interactive Hex Editing, Hex Dumping, and Raw Binary Dumping.

## Compilation & Installation

Since there are no external dependencies, compiling from source is instantaneous. 

**For macOS / Linux (Clang or GCC):**
```bash
clang chex.c -o chex -O2
# or
gcc chex.c -o chex -O2

```

**For Windows (MSVC):**

```cmd
cl /Fe:chex.exe chex.c /O2

```

## Usage

Execute the binary from your terminal, passing the mode and the target file.

```bash
# Enter Interactive Edit Mode
./chex edit target_file.extension

# Dump formatted hex to the terminal
./chex hex target_file.extension

# Dump raw binary strings to the terminal
./chex binary target_file.extension

```

### Edit Mode Keybindings

| Key | Action |
| --- | --- |
| **Arrow Keys** | Navigate the hex grid |
| **Page Up / Page Down** | Scroll memory view by chunks |
| **Enter / Return** | Toggle Insert/Overwrite Mode |
| **0-9, A-F** | Type raw hex bytes (while in Insert Mode) |
| **Q** | Quit the editor safely |


## Roadmap

* [x] Cross-platform compatibility (Windows/POSIX)
* [x] Interactive TUI navigation
* [x] Direct-disk overwriting
* [ ] Search feature (Raw Hex & ASCII)

## License

This project is open source and available under the [MIT License](LICENSE).