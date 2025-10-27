# BeaverKiosk C++ Edition

## Project Overview
A complete web application written in pure C++ demonstrating the conversion of a React-based application into a native C++ HTTP server with server-side HTML generation. This project showcases low-level socket programming, HTTP protocol implementation, and server-side rendering without any JavaScript on the client side.

## Current State
✅ **Fully Functional** - The C++ web server is running on port 5000 and serving:
- Dynamic HTML pages (server-side rendered)
- JSON API endpoints (`/api/menu`)
- Static CSS files
- All with zero client-side JavaScript required

## Project Architecture

### Technology Stack
- **Language**: C++17
- **Compiler**: Clang++ 14
- **Build System**: Make
- **Server**: Custom HTTP server using POSIX sockets
- **Frontend**: Server-side HTML generation (no React, no JavaScript)
- **Styling**: Pure CSS (same as original React version)

### Directory Structure
```
├── src/              # C++ source files
│   ├── server.cpp           # Main HTTP server (socket programming)
│   ├── http_utils.cpp       # HTTP parsing & response building
│   └── html_generator.cpp   # HTML generation (replaces React)
├── include/          # Header files
│   ├── http_utils.h
│   └── html_generator.h
├── public/           # Static assets
│   └── css/
│       └── styles.css
├── build/            # Compiled object files
├── Makefile          # Build configuration
└── beaver_kiosk      # Compiled executable
```

## Key Features

### 1. Custom HTTP Server
- Built from scratch using raw POSIX sockets
- Handles GET requests for HTML, JSON, and CSS
- Proper HTTP/1.1 response formatting
- Port: 5000, bound to 0.0.0.0

### 2. Server-Side Rendering
- C++ functions generate HTML directly
- Replaces React components with string concatenation
- No client-side JavaScript needed
- Full HTML page served on every request

### 3. RESTful API
- `/` - Main menu page (HTML)
- `/api/menu` - JSON endpoint with app data
- `/css/styles.css` - Static CSS file

## Build & Run

### Compile
```bash
make clean && make
```

### Run Server
```bash
./beaver_kiosk
```
Server starts on http://0.0.0.0:5000

### Workflow
The "Server" workflow is configured to run `./beaver_kiosk` and automatically restarts when code changes.

## React → C++ Conversion

### What Changed
- **React Components** → C++ string generation functions
- **JSX** → ostringstream concatenation
- **Client-side rendering** → Server-side HTML generation
- **Fetch API** → Direct HTTP response from server
- **React hooks** → Static data structures
- **Node.js backend** → Custom C++ socket server

### What Stayed the Same
- CSS styling (100% identical)
- Visual design and layout
- App tile structure and data
- Dark theme with gradient background

## Recent Changes (October 27, 2025)

### Created Complete C++ Web Server
1. ✅ Implemented custom HTTP server with POSIX sockets
2. ✅ Created HTTP utilities for request parsing and response building
3. ✅ Developed HTML generation functions to replace React components
4. ✅ Set up Makefile for easy compilation
5. ✅ Configured workflow to run server on port 5000
6. ✅ Tested all endpoints successfully
7. ✅ Created comprehensive README documentation

### Files Created
- `src/server.cpp` - Main server implementation
- `src/http_utils.cpp` - HTTP protocol handling
- `src/html_generator.cpp` - HTML generation
- `include/http_utils.h` - HTTP utilities header
- `include/html_generator.h` - HTML generator header
- `public/css/styles.css` - Copied from original React version
- `Makefile` - Build configuration
- `README.md` - Project documentation
- `.gitignore` - C++ build artifacts

## Technical Notes

### Architecture Review (Architect Feedback)
✅ **Pass** - Implementation meets conversion goals
- HTTP server correctly handles routing
- HTML generation mirrors React components
- Build system is properly configured
- Documentation is clear and comprehensive

**Noted Improvements for Future**:
- Add multi-threading for concurrent requests
- Enhance HTTP parsing validation
- Expand logging and error handling

### Memory Management
- No dynamic allocations in hot path
- String concatenation uses ostringstream
- Stack-based data structures
- No memory leaks detected

### Performance
- Native compiled binary (no interpreter)
- Direct socket communication
- Minimal overhead compared to Node.js/React stack

## User Preferences
None specified yet.

## Known Limitations
1. Single-threaded server (one request at a time)
2. 8KB buffer limit for requests
3. No client-side interactivity (buttons don't execute actions)
4. Icons replaced with text initials (no icon library)
5. No WebSocket support (no live updates)

## Future Enhancement Ideas
- Add multi-threading using pthreads
- Implement connection pooling
- Add template engine for cleaner HTML generation
- Support POST/PUT/DELETE methods
- Add authentication system
- Implement actual app launching functionality
- Add WebSocket support for real-time updates

## Dependencies
- C++ compiler (Clang++ or G++)
- POSIX-compliant system (Linux/macOS)
- Make build tool
- C++17 standard library

No external libraries required - pure C++ and POSIX APIs!

---

**Last Updated**: October 27, 2025
**Status**: Complete and running
**Server**: http://0.0.0.0:5000
