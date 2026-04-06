# QmlNetDebugger - Go REST Server

A lightweight HTTP server for serving QML files over the network with real-time change notifications via Server-Sent Events (SSE).

## Features

- **REST API** for serving QML files and metadata
- **Real-time file change notifications** via Server-Sent Events (SSE)
- **ETag-based caching** for efficient bandwidth usage
- **Gzip compression** for responses
- **CORS support** for development
- **Path traversal protection** for security
- **Structured logging** with zerolog
- **Graceful shutdown** handling

## Prerequisites

- Go 1.21 or later

## Installation

### From Source

```bash
# Clone the repository
git clone https://github.com/qmlnetdebugger/qmlnetdebugger.git
cd qmlnetdebugger/server

# Download dependencies
go mod download

# Build the server
go build -o bin/qmlnetd .

# Or use the Makefile
make build
```

### Using Go Install

```bash
go install github.com/qmlnetdebugger/server@latest
```

## Usage

### Basic Usage

```bash
# Start the server with default settings (port 8080, ./qml directory)
./bin/qmlnetd

# Specify a custom QML directory
./bin/qmlnetd -dir /path/to/qml/files

# Specify custom host and port
./bin/qmlnetd -dir /path/to/qml/files -host 0.0.0.0 -port 8080

# Enable verbose logging
./bin/qmlnetd -dir /path/to/qml/files -verbose
```

### Command-Line Arguments

| Flag    | Type   | Default | Description                  |
|---------|--------|---------|------------------------------|
| `-dir`  | string | `./qml` | QML files directory          |
| `-port` | int    | `8080`  | Server port                  |
| `-host` | string | `0.0.0.0`| Server host                  |
| `-verbose`| bool  | `false` | Enable verbose logging       |
| `-log-level`|string| `info`  | Log level (debug, info, warn, error) |

## API Endpoints

### List Files

```http
GET /api/files?recursive=true
```

Returns a list of all QML files in the configured directory.

**Response:**
```json
{
  "files": [
    {
      "path": "main.qml",
      "size": 256,
      "lastModified": "2026-04-06T15:00:00Z",
      "etag": "abc123"
    }
  ]
}
```

### Get File

```http
GET /api/file/{name}
```

Retrieves a specific QML file.

**Headers:**
- `If-None-Match`: ETag for conditional requests (returns 304 if unchanged)

**Response:**
- `Content-Type: application/x-qml`
- `ETag`: File hash for caching
- `Last-Modified`: Last modification timestamp

### Get File Info

```http
GET /api/file/{name}/info
```

Returns metadata about a file without its content.

**Response:**
```json
{
  "path": "main.qml",
  "size": 256,
  "lastModified": "2026-04-06T15:00:00Z",
  "etag": "abc123"
}
```

### Server-Sent Events (SSE)

```http
GET /api/events?path=main.qml
```

Real-time file change notifications.

**Query Parameters:**
- `path`: Optional filter for specific file

**Event Format:**
```
event: file_changed
data: {"path":"main.qml","action":"modified","etag":"new123"}
```

### Health Check

```http
GET /api/health
```

Returns server health status.

**Response:**
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "uptime": 3600
}
```

### Server Info

```http
GET /api/info
```

Returns server configuration and capabilities.

**Response:**
```json
{
  "version": "1.0.0",
  "features": {
    "sse": true,
    "websocket": false,
    "compression": true
  },
  "maxFileSize": 10485760,
  "allowedExtensions": [".qml", ".js", ".json"]
}
```

## Security

### Path Traversal Protection

The server validates all file paths to prevent directory traversal attacks. Files outside the configured QML directory cannot be accessed.

### CORS

CORS is enabled for development purposes. In production, you should restrict allowed origins.

### Rate Limiting

Rate limiting is not implemented by default. Consider adding it for production deployments.

## Development

### Project Structure

```
server/
├── main.go                          # Server entry point
├── go.mod                           # Go module definition
├── go.sum                           # Dependency checksums
├── internal/
│   ├── config/
│   │   └── config.go                # Configuration management
│   ├── handlers/
│   │   ├── handlers.go              # HTTP handlers
│   │   └── sse.go                   # SSE implementation
│   ├── middleware/
│   │   └── middleware.go            # HTTP middleware
│   ├── watcher/
│   │   └── watcher.go               # File system watcher
│   └── models/
│       └── models.go                # Data models
└── README.md                        # This file
```

### Running Tests

```bash
# Run all tests
go test ./...

# Run tests with coverage
go test -cover ./...
```

### Building for Different Platforms

```bash
# Linux
GOOS=linux GOARCH=amd64 go build -o bin/qmlnetd-linux .

# Windows
GOOS=windows GOARCH=amd64 go build -o bin/qmlnetd.exe .

# macOS
GOOS=darwin GOARCH=amd64 go build -o bin/qmlnetd-darwin .
```

## Docker

### Build Docker Image

```bash
docker build -t qmlnetd:latest .
```

### Run Docker Container

```bash
docker run -p 8080:8080 -v /path/to/qml:/qml qmlnetd:latest
```

## Example Workflow

1. **Start the server:**
   ```bash
   ./bin/qmlnetd -dir ./my-qml-app
   ```

2. **Connect an SSE client to receive updates:**
   ```bash
   curl -N http://localhost:8080/api/events
   ```

3. **List available files:**
   ```bash
   curl http://localhost:8080/api/files
   ```

4. **Get a specific file:**
   ```bash
   curl http://localhost:8080/api/file/main.qml
   ```

5. **Modify a QML file** - the SSE client will receive a notification

## Troubleshooting

### File Not Found (404)

- Ensure the file path is relative to the configured QML directory
- Check that the file extension is allowed (`.qml`, `.js`, `.json`)

### SSE Connection Drops

- Check network connectivity
- Ensure no firewall is blocking the connection
- Verify the server is still running

### High Memory Usage

- Reduce the number of watched files
- Adjust the debounce delay in the watcher

## License

See LICENSE file for details.

## Contributing

Contributions are welcome! Please read the contributing guidelines before submitting pull requests.
