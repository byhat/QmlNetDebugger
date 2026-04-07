package models

import "time"

// FileInfo represents metadata about a QML file
type FileInfo struct {
	Path         string    `json:"path"`           // Relative path from the QML directory
	Size         int64     `json:"size"`           // File size in bytes
	LastModified time.Time `json:"lastModified"`   // Last modification timestamp
	ETag         string    `json:"etag,omitempty"` // ETag for caching
}

// FileListResponse is the response for listing files
type FileListResponse struct {
	Files []FileInfo `json:"files"`
}

// FileChangedEvent represents a file change event for SSE
type FileChangedEvent struct {
	Path   string `json:"path"`           // Relative path of the changed file
	Action string `json:"action"`         // Action: "created", "modified", "deleted"
	ETag   string `json:"etag,omitempty"` // New ETag (not set for deleted files)
}

// HealthResponse is the response for health check
type HealthResponse struct {
	Status  string `json:"status"` // "healthy" or "unhealthy"
	Version string `json:"version"`
	Uptime  int64  `json:"uptime"` // Uptime in seconds
}

// InfoResponse contains server information
type InfoResponse struct {
	Version           string   `json:"version"`
	Features          Features `json:"features"`
	MaxFileSize       int64    `json:"maxFileSize"`
	AllowedExtensions []string `json:"allowedExtensions"`
}

// Features describes server capabilities
type Features struct {
	SSE         bool `json:"sse"`
	WebSocket   bool `json:"websocket"`
	Compression bool `json:"compression"`
}

// ErrorResponse represents an error response
type ErrorResponse struct {
	Error   string `json:"error"`
	Message string `json:"message,omitempty"`
}

// FileBundleResponse is the response for downloading all QML files as a bundle
type FileBundleResponse struct {
	Files     map[string]string `json:"files"`     // relative path → file content (text or base64)
	Encodings map[string]string `json:"encodings"` // relative path → "text" or "base64"
	Count     int               `json:"count"`
	Timestamp time.Time         `json:"timestamp"`
}
