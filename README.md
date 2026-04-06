# QmlNetDebugger

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/qmlnetdebugger/qmlnetdebugger)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Go Version](https://img.shields.io/badge/Go-1.21+-00ADD8.svg)](https://golang.org/)
[![Qt Version](https://img.shields.io/badge/Qt-6.5+-41CD52.svg)](https://www.qt.io/)

A cross-platform development tool for QML applications that enables hot-reloading of QML files over the network. QmlNetDebugger consists of a lightweight Go REST server and a Qt6-based client application, providing real-time QML updates during development.

## Table of Contents

- [Project Overview](#project-overview)
- [Architecture Overview](#architecture-overview)
- [Quick Start Guide](#quick-start-guide)
- [Features](#features)
- [API Reference](#api-reference)
- [Configuration](#configuration)
- [Development](#development)
- [Troubleshooting](#troubleshooting)
- [Roadmap](#roadmap)
- [License](#license)

---

## Project Overview

QmlNetDebugger is a two-part application designed to streamline QML development by enabling remote loading and hot-reloading of QML files. It eliminates the need to rebuild your application when modifying QML files, significantly reducing development iteration time.

### Key Components

1. **Go REST Server** (`server/`) - A lightweight HTTP server that serves QML files from a specified directory and broadcasts real-time change notifications
2. **Qt6 Client** (`client/`) - A cross-platform desktop application that loads QML files from the server and updates them dynamically

### Key Benefits

- **Faster Development**: No need to rebuild your application when modifying QML files
- **Cross-Platform**: Works on Windows, Linux, and macOS
- **Real-Time Updates**: Instant hot-reloading via Server-Sent Events (SSE)
- **Efficient Bandwidth**: ETag-based caching minimizes unnecessary data transfer
- **Flexible Deployment**: Serve QML files from any directory on your development machine
- **Network Support**: Test QML on remote devices or different platforms

### Use Cases

- **Rapid QML Prototyping**: Quickly iterate on QML designs without full application rebuilds
- **Cross-Platform Testing**: Test QML on multiple platforms from a single development machine
- **Remote Development**: Develop QML on one machine and test on another
- **Team Collaboration**: Share QML designs with team members in real-time
- **Embedded Development**: Test QML on embedded devices with limited build capabilities

---

## Architecture Overview

QmlNetDebugger follows a client-server architecture with the following components:

```
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│  Qt6 Client     │         │  Go REST Server │         │  QML Files      │
│                 │         │                 │         │                 │
│  - QML Engine   │◄────────┤  - File Server  │◄────────┤  - main.qml     │
│  - Network Mgr  │  HTTP   │  - SSE Handler  │  Serve  │  - components/  │
│  - UI Renderer │         │  - File Watcher │         │  - styles/      │
└─────────────────┘         └─────────────────┘         └─────────────────┘
      ▲                                                           │
      │                                                           │
      │ SSE Events                                                │
      └───────────────────────────────────────────────────────────┘
```

### Component Interaction

1. **Client Startup**: The Qt6 client prompts for server connection details (host, port, filename)
2. **Initial Load**: Client requests the QML file from the server via HTTP GET
3. **File Serving**: Go server reads and returns the QML file with ETag headers
4. **QML Rendering**: Client loads and renders the QML in its QML engine
5. **Change Detection**: Server monitors the QML directory for file modifications
6. **Real-Time Updates**: Server broadcasts changes via SSE; client receives notifications and reloads

### Detailed Architecture

For a comprehensive understanding of the system architecture, including detailed component diagrams, data flow, and technical specifications, see the [Architecture Design Document](plans/QmlNetDebugger-Architecture.md).

---

## Quick Start Guide

### Prerequisites

#### Server Requirements
- **Go**: Version 1.21 or later
- **Operating System**: Windows, Linux, or macOS

#### Client Requirements
- **Qt**: Version 6.5 or later with modules:
  - Qt Core
  - Qt Network
  - Qt Qml
  - Qt Quick
  - Qt Quick Controls 2
- **CMake**: Version 3.16 or later
- **C++ Compiler**: C++17 compatible (GCC 8+, Clang 8+, MSVC 2019+)

### Installation

#### 1. Clone the Repository

```bash
git clone https://github.com/qmlnetdebugger/qmlnetdebugger.git
cd qmlnetdebugger
```

#### 2. Build the Server

```bash
cd server
go mod download
go build -o bin/qmlnetd .
```

#### 3. Build the Client

**Linux/macOS:**
```bash
cd client
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build . --config Release
```

**Windows:**
```cmd
cd client
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64" -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### Running the Application

#### Step 1: Start the Server

```bash
# From the project root
./server/bin/qmlnetd -dir ./server/test-qml -port 8080
```

The server will start listening on `http://localhost:8080` and serve QML files from the `./server/test-qml` directory.

#### Step 2: Launch the Client

**Linux/macOS:**
```bash
./client/build/bin/qmlnetdebugger
```

**Windows:**
```cmd
client\build\bin\Release\qmlnetdebugger.exe
```

#### Step 3: Connect to the Server

1. The connection dialog will appear automatically
2. Enter the following details:
   - **Server Host**: `localhost`
   - **Server Port**: `8080`
   - **QML Filename**: `main.qml`
3. Click "Connect"

The client will load and display the QML file. Any changes you make to the QML files in the `./server/test-qml` directory will be automatically reflected in the client.

### Example Workflow

```bash
# Terminal 1: Start the server
./server/bin/qmlnetd -dir ./my-qml-project -port 8080

# Terminal 2: Start the client
./client/build/bin/qmlnetdebugger

# Terminal 3: Edit QML files (changes will appear instantly in the client)
vim ./my-qml-project/main.qml
```

---

## Features

### Server Features

- **REST API**: Clean HTTP API for serving QML files and metadata
- **Real-Time Notifications**: Server-Sent Events (SSE) for instant change broadcasts
- **ETag Caching**: Efficient conditional requests using ETag headers
- **Gzip Compression**: Reduced bandwidth usage for file transfers
- **CORS Support**: Enabled for development convenience
- **Path Traversal Protection**: Security validation to prevent directory traversal attacks
- **Structured Logging**: Comprehensive logging with zerolog
- **Graceful Shutdown**: Proper cleanup of resources on termination
- **File Watching**: Real-time monitoring of the QML directory for changes

### Client Features

- **Remote QML Loading**: Download and display QML files from a remote server
- **Hot-Reload**: Automatic QML reloading when files change on the server
- **ETag Support**: Efficient conditional requests using ETag headers
- **Server-Sent Events (SSE)**: Real-time file change notifications
- **Polling Fallback**: Automatic fallback to polling if SSE is unavailable
- **Connection History**: Save and reuse connection configurations
- **Cross-Platform**: Works on Windows, Linux, and macOS
- **Settings Persistence**: Save application settings using QSettings
- **Manual Refresh**: Force reload QML content on demand

### Key Improvements Over Original Design

| Feature | Original Design | Implemented |
|---------|-----------------|-------------|
| Update Mechanism | Polling only | SSE + Polling fallback |
| Bandwidth Efficiency | Full file transfers | ETag-based caching |
| Compression | None | Gzip compression |
| Security | Basic | Path traversal protection |
| Logging | Simple | Structured logging |
| Connection Management | Manual | History & persistence |
| Error Handling | Basic | Comprehensive error handling |

---

## API Reference

The server provides a REST API for QML file operations and real-time events.

### Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/files` | GET | List all QML files |
| `/api/file/{name}` | GET | Retrieve a specific QML file |
| `/api/file/{name}/info` | GET | Get file metadata |
| `/api/events` | GET | SSE endpoint for change notifications |
| `/api/health` | GET | Server health check |
| `/api/info` | GET | Server configuration and capabilities |

### Example Requests

#### List Files
```http
GET /api/files?recursive=true HTTP/1.1
Host: localhost:8080
```

#### Get File with ETag
```http
GET /api/file/main.qml HTTP/1.1
Host: localhost:8080
If-None-Match: "abc123"
```

#### SSE Events
```http
GET /api/events?path=main.qml HTTP/1.1
Host: localhost:8080
Accept: text/event-stream
```

For detailed API documentation including request/response formats, see the [Server README](server/README.md).

---

## Configuration

### Server Configuration

The server can be configured via command-line arguments:

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `-dir` | string | `./qml` | QML files directory |
| `-port` | int | `8080` | Server port |
| `-host` | string | `0.0.0.0` | Server host |
| `-verbose` | bool | `false` | Enable verbose logging |
| `-log-level` | string | `info` | Log level (debug, info, warn, error) |

**Example:**
```bash
./server/bin/qmlnetd -dir /path/to/qml -port 9000 -verbose
```

### Client Configuration

The client stores configuration settings using QSettings:

- **Server Host**: Last connected server address
- **Server Port**: Last connected port
- **QML Filename**: Last loaded QML file
- **Use SSE**: Preference for SSE vs. polling
- **Update Interval**: Polling interval in milliseconds (when SSE disabled)
- **Connection History**: List of recent connections

Configuration is automatically saved and restored between sessions.

**Configuration Location:**
- **Linux**: `~/.config/QmlNetDebugger/QmlNetDebugger.conf`
- **macOS**: `~/Library/Preferences/com.qmlnetdebugger.qmlnetdebugger.plist`
- **Windows**: `HKEY_CURRENT_USER\Software\QmlNetDebugger\QmlNetDebugger`

---

## Development

### Project Structure

```
qmlnetdebugger/
├── client/                 # Qt6 client application
│   ├── src/               # C++ source files
│   ├── resources/        # QML resources
│   ├── CMakeLists.txt    # CMake build configuration
│   └── README.md         # Client documentation
├── server/                # Go REST server
│   ├── internal/         # Internal packages
│   │   ├── config/      # Configuration management
│   │   ├── handlers/    # HTTP handlers
│   │   ├── middleware/  # HTTP middleware
│   │   ├── models/      # Data models
│   │   └── watcher/     # File system watcher
│   ├── test-qml/        # Sample QML files
│   ├── go.mod           # Go module definition
│   ├── main.go          # Server entry point
│   └── README.md        # Server documentation
├── plans/                # Architecture and design documents
│   └── QmlNetDebugger-Architecture.md
└── README.md             # This file
```

### Building Components

#### Server

```bash
cd server
go mod download
go build -o bin/qmlnetd .
```

#### Client

```bash
cd client
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build . --config Release
```

### Testing

#### Server Tests

```bash
cd server
go test ./...
```

#### Client Tests

The client currently does not have automated tests. Manual testing is performed by:
1. Starting the server with sample QML files
2. Launching the client and connecting to the server
3. Modifying QML files and verifying hot-reload functionality
4. Testing error scenarios (server unavailable, invalid files, etc.)

---

## Troubleshooting

### Common Issues

#### Server Issues

**Issue: Server fails to start with "address already in use" error**

**Solution:**
```bash
# Check if port is in use
lsof -i :8080  # Linux/macOS
netstat -ano | findstr :8080  # Windows

# Either kill the process or use a different port
./server/bin/qmlnetd -port 8081
```

**Issue: Server cannot access QML directory**

**Solution:**
- Verify the directory path is correct
- Check file system permissions
- Ensure the directory exists:
  ```bash
  ls -la /path/to/qml-directory
  ```

**Issue: SSE events not received**

**Solution:**
- Check if a firewall is blocking the connection
- Verify the client is using the correct endpoint (`/api/events`)
- Enable verbose logging on the server to see if events are being sent:
  ```bash
  ./server/bin/qmlnetd -verbose
  ```

#### Client Issues

**Issue: Client cannot connect to server**

**Solution:**
- Verify the server is running and listening on the correct port
- Check network connectivity:
  ```bash
  curl http://localhost:8080/api/health
  ```
- Ensure firewall is not blocking the connection
- Verify the host and port in the connection dialog

**Issue: QML file fails to load**

**Solution:**
- Verify the QML filename is correct (case-sensitive)
- Check the server logs for errors
- Ensure the QML file is valid syntax
- Try accessing the file directly:
  ```bash
  curl http://localhost:8080/api/file/main.qml
  ```

**Issue: Hot-reload not working**

**Solution:**
- Verify SSE is enabled in client settings
- Check server logs to see if file changes are detected
- If SSE fails, the client should fall back to polling
- Manually refresh using the refresh button (Ctrl+R)

**Issue: Client crashes on startup**

**Solution:**
- Check Qt installation and required modules
- Verify the Qt path in CMake configuration
- Run the client from terminal to see error messages
- Ensure Qt platform plugins are available

### Error Messages

| Error Message | Cause | Solution |
|---------------|-------|----------|
| `connection refused` | Server not running | Start the server |
| `404 Not Found` | File doesn't exist | Check filename and directory |
| `304 Not Modified` | File unchanged (ETag match) | Normal behavior, no action needed |
| `500 Internal Server Error` | Server error | Check server logs with `-verbose` |
| `ETag mismatch` | Cache invalidation | Client will re-download file |
| `SSE connection lost` | Network issue | Client will attempt reconnection |

### Getting Help

If you encounter issues not covered here:
1. Check the [Server README](server/README.md) for server-specific issues
2. Check the [Client README](client/README.md) for client-specific issues
3. Review the [Architecture Document](plans/QmlNetDebugger-Architecture.md) for technical details
4. Enable verbose logging on both components to gather diagnostic information

---

## Roadmap

### Potential Future Improvements

#### Server Enhancements
- [ ] WebSocket support as an alternative to SSE
- [ ] Authentication and authorization for secure deployments
- [ ] Rate limiting to prevent abuse
- [ ] TLS/HTTPS support for encrypted connections
- [ ] Multiple directory support with virtual paths
- [ ] QML syntax validation before serving
- [ ] Built-in QML minification for production
- [ ] Metrics and monitoring endpoints

#### Client Enhancements
- [ ] QML editor integration with syntax highlighting
- [ ] Debug console for QML/JavaScript errors
- [ ] Performance profiling tools
- [ ] Screenshot capture functionality
- [ ] Multiple QML file support (tabbed interface)
- [ ] Dark/light theme switching
- [ ] Keyboard shortcuts for common actions
- [ ] Export/import connection settings

#### Cross-Component Features
- [ ] Automatic server discovery on local network
- [ ] Bidirectional communication (client → server)
- [ ] Collaborative editing support
- [ ] Version control integration
- [ ] CI/CD pipeline integration
- [ ] Docker containerization

### Known Limitations

1. **Single File Focus**: The client is designed to load a single main QML file. Complex applications with multiple entry points may require additional setup.

2. **No Authentication**: The server does not implement authentication. It should not be exposed to untrusted networks.

3. **File Size Limits**: Large QML files may experience slower load times due to network transfer.

4. **Network Dependency**: The client requires continuous network connectivity to the server for hot-reload functionality.

5. **Platform-Specific Qt**: The client requires Qt6 installation, which may not be available on all platforms.

6. **No Built-in Editor**: The client displays QML but does not provide editing capabilities.

7. **Limited Error Recovery**: Some error conditions may require restarting the client or server.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2026 QmlNetDebugger Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgments

- Qt framework for the cross-platform UI capabilities
- Go standard library for the HTTP server implementation
- zerolog for structured logging
- The open-source community for various QML and Go resources
