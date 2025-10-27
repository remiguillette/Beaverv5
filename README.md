# BeaverKiosk GTK

BeaverKiosk has been rebuilt as a native GTK 4 desktop application. The project now focuses on launching local tools from a modern, kiosk-friendly interface without relying on any embedded web server or remote ports.

## Overview

- **GTK 4 UI** – Responsive tiles built with `GtkFlowBox` and custom CSS styling.
- **Local-first interactions** – Clicking a tile prepares a local launch sequence instead of issuing HTTP calls.
- **Portable C++ codebase** – Minimal dependencies beyond GTK 4 and the standard library.

## Building

### Prerequisites

- A C++20 capable compiler (`g++` or `clang++`)
- `pkg-config`
- GTK 4 development packages (for example `libgtk-4-dev` on Debian/Ubuntu or `gtk4` on Fedora/Arch)

### Compile

```bash
make
```

The build system uses `pkg-config` to locate GTK 4 headers and libraries and emits optimized (`-O2 -pipe`) binaries by default. Override the `CXX` variable if you prefer a different compiler:

```bash
make CXX=clang++
```

### Run

```bash
./beaver_kiosk
```

A window titled **BeaverKiosk GTK** will open with local utility tiles. Selecting a tile updates the status banner with a message suitable for integrating real launch commands.

### Clean

```bash
make clean
```

## Project Structure

```
├── Makefile         # GTK-aware build script
├── README.md        # Project overview and instructions
└── src/
    └── main.cpp     # GTK 4 application entry point
```

## Customization

- Modify the `kApps` array in `src/main.cpp` to change the available tiles.
- Adapt the `on_tile_clicked` handler to integrate with local command execution.
- Update the CSS in `load_css()` for alternate branding.

## License

Demonstration project for educational purposes.
