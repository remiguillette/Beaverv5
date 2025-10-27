# BeaverKiosk – Native C++ Middleware Edition

BeaverKiosk is now a single C++20 codebase that powers both a headless HTTP server and a native GTK 4 desktop experience. A lightweight **middleware layer** (`AppManager`) sits between the presentation layers and the underlying data so every user interface stays in sync without duplicating logic.

## Highlights

- **Shared Core:** `AppManager` exposes the kiosk catalogue as structured data and can serialise it to HTML or JSON.
- **HTTP Front-End:** A POSIX socket server serves HTML, JSON, and static assets using the middleware output.
- **GTK 4 Front-End:** A modern desktop interface implemented with `GtkApplication`, `GtkFlowBox`, and CSS styling inspired by the web version.
- **Clang-First Build:** The Makefile targets `clang++` by default and consumes the proper GTK 4 flags via `pkg-config`.
- **Single Binary:** `./beaver_kiosk` selects the desired UI at runtime (`--http` or `--gtk`).

## Repository Layout

```
├── include/
│   ├── core/app_manager.h       # Middleware API shared by every UI layer
│   └── ui/
│       ├── gtk/gtk_app.h        # GTK window/controller declaration
│       └── http/
│           ├── http_server.h    # HTTP façade built on the middleware
│           └── http_utils.h     # Request/response helpers
├── src/
│   ├── core/app_manager.cpp
│   ├── main.cpp                 # Entry point + CLI for choosing the UI
│   └── ui/
│       ├── gtk/gtk_app.cpp
│       └── http/
│           ├── http_server.cpp
│           └── http_utils.cpp
├── public/css/styles.css        # Shared styling for the HTML UI
├── docs/debian-local.md         # Debian focused setup guide
└── Makefile                     # clang + GTK aware build instructions
```

## Building

Install the prerequisites on Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y clang make pkg-config libgtk-4-dev
```

Then compile the project:

```bash
make clean
make
```

The Makefile automatically discovers every source file, applies the GTK 4 compiler flags from `pkg-config`, and links them into a single `beaver_kiosk` executable. Override the compiler if necessary:

```bash
make CXX=/usr/bin/g++
```

More detailed platform notes live in [docs/debian-local.md](docs/debian-local.md).

## Running

The executable supports both runtimes through command-line flags:

```bash
./beaver_kiosk --http            # Start the HTTP server on port 5000
./beaver_kiosk --http --port=8080
./beaver_kiosk --gtk             # Launch the GTK 4 desktop UI
```

Use `./beaver_kiosk --help` to list all options. If no flag is provided the HTTP server is selected automatically.

## Middleware Flow

```
┌───────────────┐      ┌────────────────────┐
│ HTTP Requests │ ---> │ HttpServerApp      │
│ GTK Signals   │ ---> │ GtkApp             │
└───────────────┘      └────────┬───────────┘
                                 │
                         ┌───────▼────────┐
                         │ AppManager     │
                         │  • get_available_apps()
                         │  • to_html()
                         │  • to_json()
                         └───────────────┘
```

Both presentation layers consume the same `AppTile` data supplied by `AppManager`, ensuring a single source of truth. Adding another frontend (CLI, REST gateway, etc.) only requires plugging into the middleware.

## Styling & Accessibility

- **GTK CSS Provider:** Recreates the kiosk look-and-feel with gradient backgrounds and accent colours per tile.
- **Flowbox Layout:** Automatically wraps tiles while maintaining keyboard focus and pointer hover feedback.
- **Web Output:** Continues to serve the existing `public/css/styles.css`, so browsers and GTK remain visually aligned.

## Next Steps

- Load kiosk tiles from an external JSON/YAML file rather than hard-coded data.
- Add command dispatch so each tile can launch Debian desktop applications.
- Extend the middleware with persistence or remote management APIs.

Enjoy the new middleware-driven BeaverKiosk! 🦫
