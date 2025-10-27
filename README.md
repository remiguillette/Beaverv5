# BeaverKiosk - C++ Edition

A complete web application written in pure C++ demonstrating how to convert a React-based web application into a native C++ HTTP server with server-side HTML generation.

## Overview

This project is a **complete conversion** of the React-based BeaverKiosk menu application into native C++. It demonstrates:

- **Custom HTTP Server**: Built from scratch using POSIX sockets
- **Server-Side HTML Generation**: Replaces React components with C++ functions
- **RESTful API**: JSON endpoints for menu data
- **Static File Serving**: CSS and other assets
- **Zero JavaScript Required**: The client-side requires no JavaScript to function

## React vs C++ Comparison

### Original React Version
- **Frontend**: React components with JSX
- **Rendering**: Client-side with React DOM
- **State Management**: React hooks (useState, useEffect)
- **HTTP Requests**: Fetch API calls to backend
- **Dependencies**: React, ReactDOM, Babel, Lucide icons
- **Runtime**: Requires Node.js backend + browser JavaScript

### C++ Version
- **Backend**: Custom HTTP server using raw sockets
- **Rendering**: Server-side HTML generation in C++
- **State Management**: Static data structures in memory
- **HTTP Requests**: Direct socket communication
- **Dependencies**: Only C++ standard library + POSIX
- **Runtime**: Single native binary, no JavaScript needed

## Project Structure

```
├── include/
│   ├── html_generator.h    # HTML generation functions (replaces React components)
│   └── http_utils.h        # HTTP parsing and response building
├── src/
│   ├── html_generator.cpp  # Implementation of HTML generators
│   ├── http_utils.cpp      # HTTP protocol implementation
│   └── server.cpp          # Main server with socket programming
├── public/
│   └── css/
│       └── styles.css      # Same CSS as React version
├── build/                  # Compiled object files
├── Makefile               # Build configuration
└── beaver_kiosk           # Compiled executable
```

## Key Components

### 1. HTTP Server (src/server.cpp)
The core HTTP server implementation using:
- **POSIX Sockets**: Low-level network programming
- **Request Handling**: Parse HTTP requests and route to handlers
- **Response Building**: Generate proper HTTP responses
- **Static File Serving**: Read and serve CSS files

### 2. HTML Generator (src/html_generator.cpp)
Replaces React components with C++ functions:
- **`generate_app_tile_html()`**: Equivalent to React's `<AppTile>` component
- **`generate_menu_page_html()`**: Equivalent to `<BeaverMenu>` component
- **`generate_menu_json()`**: Generates JSON API responses

### 3. HTTP Utilities (src/http_utils.cpp)
HTTP protocol handling:
- **Request Parsing**: Parse HTTP method, path, headers, body
- **Response Building**: Create properly formatted HTTP responses
- **MIME Type Detection**: Serve files with correct content types
- **URL Decoding**: Handle URL-encoded strings

## Conversion Process: React → C++

### React Component → C++ Function

**React (Original):**
```jsx
const AppTile = ({ name, icon: Icon, accent }) => {
  return (
    <button className={`app-tile app-tile--${accent}`}>
      <div className="app-tile__icon">
        <Icon size={42} />
      </div>
      <h3>{name}</h3>
    </button>
  );
};
```

**C++ (Converted):**
```cpp
std::string generate_app_tile_html(const AppTile& app) {
    std::ostringstream html;
    html << "<button class=\"app-tile app-tile--" << app.accent << "\">\n";
    html << "  <div class=\"app-tile__icon\">\n";
    html << "    <span>" << app.name[0] << "</span>\n";
    html << "  </div>\n";
    html << "  <h3>" << app.name << "</h3>\n";
    html << "</button>\n";
    return html.str();
}
```

### React Fetch → C++ Direct Response

**React (Original):**
```jsx
const response = await fetch('/api/menu');
const data = await response.json();
setApps(mapRemoteApps(data));
```

**C++ (Converted):**
```cpp
if (request.path == "/api/menu") {
    std::vector<AppTile> apps = get_sample_apps();
    response.body = generate_menu_json(apps);
    response.headers["Content-Type"] = "application/json";
}
```

## Building and Running

### Prerequisites
- C++ compiler (clang++ or g++)
- Make
- POSIX-compliant system (Linux, macOS)

### Build
```bash
make clean  # Clean previous builds
make        # Compile the project
```

### Run
```bash
./beaver_kiosk
# or
make run
```

The server will start on `http://0.0.0.0:5000`

### Clean Build
```bash
make clean
```

## API Endpoints

### `GET /`
Returns the server-generated HTML page with all app tiles.

**Response**: `text/html`

### `GET /api/menu`
Returns JSON data with all available applications.

**Response**: `application/json`
```json
{
  "apps": [
    {
      "name": "Code Editor",
      "accent": "violet",
      "icon": "Code"
    },
    ...
  ]
}
```

### `GET /css/styles.css`
Serves the CSS stylesheet.

**Response**: `text/css`

## Technical Highlights

### 1. **No External Dependencies**
- Pure C++ implementation
- Only uses standard library and POSIX APIs
- No heavy frameworks like Node.js or Express

### 2. **Low Memory Footprint**
- Compiled binary is very small
- No runtime environment needed
- Direct memory management in C++

### 3. **High Performance**
- Native code execution
- No JavaScript interpretation
- Direct socket communication

### 4. **Educational Value**
Demonstrates:
- Socket programming in C++
- HTTP protocol implementation
- Server-side rendering
- String manipulation and generation
- File I/O operations

## Limitations Compared to React

1. **No Real-time Updates**: The C++ version serves static HTML (no live updates without page refresh)
2. **No Client Interaction**: React has onClick handlers; C++ version is display-only
3. **No Icon Library**: Simplified to use text initials instead of icon libraries
4. **No Client-Side Routing**: All navigation requires server round-trips

## Future Enhancements

- Add WebSocket support for real-time updates
- Implement template engine for better HTML management
- Add multi-threading for concurrent request handling
- Implement caching mechanisms
- Add logging system
- Support for actual app launching (process spawning)

## License

This is a demonstration project for educational purposes.

## Author

Built with C++ to demonstrate native web server implementation and React-to-C++ conversion techniques.

---

**BeaverKiosk C++ Edition** - Proving that you don't always need JavaScript! 🦫
