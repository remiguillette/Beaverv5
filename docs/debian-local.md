# BeaverKiosk – Debian Local Guide

This document replaces the previous Replit-oriented notes. It explains how to build and run BeaverKiosk locally on a Debian-based distribution using the new middleware-driven architecture that powers both the HTTP server and the GTK 4 desktop experience.

## 1. Prerequisites

Install the required toolchain and libraries:

```bash
sudo apt update
sudo apt install -y clang make pkg-config libgtk-4-dev
```

> **Why clang?** BeaverKiosk is now tuned for clang++ and builds cleanly with C++20. GCC will still work, but the default Makefile targets clang++ to match the middleware-first toolchain.

## 2. Project Layout

```
├── include/
│   ├── core/
│   │   └── app_manager.h       # Middleware: shared app catalogue & serializers
│   └── ui/
│       ├── gtk/gtk_app.h       # GTK 4 presentation layer
│       └── http/
│           ├── http_server.h   # HTTP façade using the middleware
│           └── http_utils.h    # Parsing & response helpers
├── src/
│   ├── core/app_manager.cpp
│   ├── main.cpp                # Entry point (switch between HTTP / GTK)
│   └── ui/
│       ├── gtk/gtk_app.cpp
│       └── http/
│           ├── http_server.cpp
│           └── http_utils.cpp
├── public/css/styles.css       # Shared styling for the web UI
├── Makefile                    # clang + GTK aware build script
└── docs/debian-local.md        # You are here
```

The **AppManager** in `src/core` is the “middle layer” that feeds both the native GTK UI and the HTTP server. Any future front-end (CLI, REST, etc.) plugs into this layer without duplicating logic.

## 3. Building the Project

```bash
make clean
make
```

The `make` target discovers every `.cpp` file recursively, applies the GTK 4 compiler flags via `pkg-config`, and links a single executable named `beaver_kiosk`.

If clang++ is installed in a custom location or you prefer another compiler, override the `CXX` variable:

```bash
make CXX=/usr/bin/g++
```

## 4. Running BeaverKiosk

A single binary now orchestrates both UIs through command-line flags.

### 4.1 HTTP Server (default)

```bash
./beaver_kiosk --http            # or simply ./beaver_kiosk
```

* Serves HTML + JSON at http://0.0.0.0:5000
* HTML is rendered server-side using the middleware data model
* Static assets live under `public/`

To change the port:

```bash
./beaver_kiosk --http --port=8080
```

### 4.2 GTK 4 Desktop App

```bash
./beaver_kiosk --gtk
```

* Native GTK 4 flowbox mirrors the web layout
* Shared CSS-inspired styling applied through a GTK CSS provider
* The middleware feeds the tile content directly—no duplication

> **Tip:** Keep `libgtk-4-dev` installed; runtime packages ship alongside the development headers on Debian.

## 5. Troubleshooting

| Symptom | Resolution |
| --- | --- |
| `make: clang++: No such file or directory` | Install clang (`sudo apt install clang`) or run `make CXX=g++`. |
| GTK compile errors | Ensure `libgtk-4-dev` is installed and that `pkg-config` can locate `gtk4`. |
| Port already in use | Pick another port with `--port=XXXX`. |

## 6. Next Steps

* Extend `AppManager` to load the tile data from JSON/YAML instead of hard-coded values.
* Add additional front-ends (CLI, REST service) by reusing the middleware API.
* Integrate system launch commands for each tile on Debian-based desktops.

Happy hacking on Debian! 🎉
