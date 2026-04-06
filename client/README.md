# QmlNetDebugger Client

A Qt6-based cross-platform desktop application for loading QML files over a network with hot-reload support.

## Features

- **Remote QML Loading**: Download and display QML files from a remote server
- **Hot-Reload**: Automatic QML reloading when files change on the server
- **ETag Support**: Efficient conditional requests using ETag headers
- **Server-Sent Events (SSE)**: Real-time file change notifications
- **Polling Fallback**: Automatic fallback to polling if SSE is unavailable
- **Connection History**: Save and reuse connection configurations
- **Cross-Platform**: Works on Windows, Linux, and macOS
- **Settings Persistence**: Save application settings using QSettings

## Requirements

### System Requirements

- **Operating System**: Windows 10+, Linux (Ubuntu 20.04+), macOS 10.15+
- **Compiler**: C++17 compatible compiler
  - Windows: MSVC 2019+ or MinGW-w64
  - Linux: GCC 8+ or Clang 8+
  - macOS: Xcode 11+ or Clang 8+
- **CMake**: Version 3.16 or later
- **Qt**: Version 6.5 or later with the following modules:
  - Qt Core
  - Qt Network
  - Qt Qml
  - Qt Quick
  - Qt Quick Controls 2

### Installing Dependencies

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-network-dev \
    qt6-qmltooling-plugins
```

#### Fedora

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    qt6-qtbase-devel \
    qt6-qtdeclarative-devel \
    qt6-qtnetworkauth-devel
```

#### Arch Linux

```bash
sudo pacman -S \
    base-devel \
    cmake \
    qt6-base \
    qt6-declarative \
    qt6-networkauth
```

#### macOS (using Homebrew)

```bash
brew install cmake qt@6
```

#### Windows

Option 1: Using vcpkg

```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# Install Qt6
./vcpkg install qt6:x64-windows
```

Option 2: Using Qt Online Installer

1. Download the Qt Online Installer from https://www.qt.io/download
2. Install Qt 6.5 or later with the required modules
3. Add Qt bin directory to your PATH

## Building

### Linux/macOS

```bash
# Navigate to client directory
cd client

# Create build directory
mkdir build
cd build

# Configure CMake (adjust Qt path if needed)
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64

# Build
cmake --build . --config Release

# Or use make
make -j$(nproc)
```

### Windows (Command Prompt)

```cmd
cd client
mkdir build
cd build

REM Configure CMake (adjust Qt path if needed)
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64" -G "Visual Studio 16 2019" -A x64

REM Build
cmake --build . --config Release
```

### Windows (PowerShell)

```powershell
cd client
mkdir build
cd build

# Configure CMake (adjust Qt path if needed)
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64" -G "Visual Studio 16 2019" -A x64

# Build
cmake --build . --config Release
```

### Using Qt Creator

1. Open Qt Creator
2. File → Open File or Project
3. Select `client/CMakeLists.txt`
4. Configure the project
5. Build and run

## Running

### Linux/macOS

```bash
cd client/build/bin
./qmlnetdebugger
```

### Windows

```cmd
cd client\build\bin\Release
qmlnetdebugger.exe
```

## Usage

### First Launch

1. Launch the application
2. The connection dialog will appear automatically
3. Enter the server details:
   - **Server Host**: IP address or hostname of the QML server (e.g., `localhost` or `192.168.1.100`)
   - **Server Port**: Port number of the QML server (default: `8080`)
   - **QML Filename**: Name of the main QML file to load (e.g., `main.qml`)
4. Configure advanced settings (optional):
   - **Use Server-Sent Events**: Enable real-time updates via SSE (recommended)
   - **Update Interval**: Polling interval in milliseconds (when SSE is disabled)
5. Click "Connect"

### Subsequent Launches

The application will remember your last connection settings. To connect to a different server:

1. Click **File → Connect...** or press `Ctrl+O`
2. Enter the new server details
3. Click "Connect"

### Recent Connections

The connection dialog shows a list of recent connections. Click on any recent connection to quickly reconnect.

### Manual Refresh

To manually refresh the QML content:

- Click **View → Refresh** or press `F5`
- Click the **Refresh** button in the status bar

### Disconnecting

To disconnect from the server:

- Click **File → Disconnect** or press `Ctrl+W`

## Server Integration

The client connects to the QmlNetDebugger Go server. Ensure the server is running before connecting.

### Starting the Server

```bash
cd server
./bin/qmlnetd -dir ./test-qml -port 8080
```

### API Endpoints

The client uses the following API endpoints:

- `GET /api/file/{name}` - Download QML file with ETag support
- `GET /api/events` - Server-Sent Events for real-time updates

## Troubleshooting

### Connection Issues

**Problem**: "Connection refused" or "Network error"

**Solutions**:
1. Verify the server is running
2. Check the server host and port are correct
3. Ensure firewall allows connections to the server port
4. Try connecting with `localhost` if running on the same machine

### QML Loading Errors

**Problem**: "QML file not found" or "QML parsing error"

**Solutions**:
1. Verify the QML filename is correct
2. Check the server logs for file access errors
3. Ensure the QML file exists in the server's QML directory
4. Validate QML syntax using a QML linter

### SSE Connection Issues

**Problem**: "SSE connection lost, falling back to polling"

**Solutions**:
1. This is normal if SSE is not supported by the server
2. The client will automatically fall back to polling
3. Check server logs for SSE endpoint errors

### Build Errors

**Problem**: "Qt6 not found" or similar CMake errors

**Solutions**:
1. Ensure Qt6 is installed correctly
2. Set `CMAKE_PREFIX_PATH` to your Qt installation directory
3. On Windows, ensure Qt bin directory is in your PATH
4. Verify all required Qt modules are installed

## Configuration

### Settings Location

Application settings are stored in platform-specific locations:

- **Windows**: `HKEY_CURRENT_USER\Software\QmlNetDebugger\QmlNetDebuggerClient`
- **Linux**: `~/.config/QmlNetDebugger/QmlNetDebuggerClient.conf`
- **macOS**: `~/Library/Preferences/com.qmlnetdebugger.client.plist`

### Settings Include

- Server host and port
- QML filename
- Update interval
- SSE enabled/disabled
- Auto-reconnect settings
- Window geometry and state
- Recent connections

## Development

### Project Structure

```
client/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── src/
│   ├── main.cpp            # Application entry point
│   ├── mainwindow.h/cpp    # Main window implementation
│   ├── qmlnetworkloader.h/cpp  # Network loader for QML files
│   ├── connectiondialog.h/cpp  # Connection dialog
│   └── settings.h/cpp      # Settings management
└── resources/
    └── qml/
        └── ConnectionDialog.qml  # QML resources
```

### Key Components

- **MainWindow**: Main application window hosting the QML view
- **QmlNetworkLoader**: Handles HTTP requests and SSE connections
- **ConnectionDialog**: Dialog for configuring server connection
- **Settings**: Manages application settings persistence

## License

This project is part of QmlNetDebugger. See the main project LICENSE file for details.

## Contributing

Contributions are welcome! Please read the main project's contributing guidelines.

## Support

For issues, questions, or suggestions, please use the main project's issue tracker.
