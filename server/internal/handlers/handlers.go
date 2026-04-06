package handlers

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strings"

	"github.com/gorilla/mux"
	"github.com/qmlnetdebugger/server/internal/config"
	"github.com/qmlnetdebugger/server/internal/models"
	"github.com/rs/zerolog/log"
)

// Handler contains the HTTP handlers and dependencies
type Handler struct {
	cfg *config.Config
}

// New creates a new handler instance
func New(cfg *config.Config) *Handler {
	return &Handler{
		cfg: cfg,
	}
}

// ListFiles handles GET /api/files
func (h *Handler) ListFiles(w http.ResponseWriter, r *http.Request) {
	// Get query parameters
	recursive := r.URL.Query().Get("recursive") != "false"

	// Walk the QML directory
	var files []models.FileInfo
	err := filepath.Walk(h.cfg.QMLDir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		// Skip directories
		if info.IsDir() {
			// Skip subdirectories if not recursive
			if !recursive && path != h.cfg.QMLDir {
				return filepath.SkipDir
			}
			return nil
		}

		// Check file extension
		if !h.isAllowedExtension(path) {
			return nil
		}

		// Get relative path
		relPath, err := filepath.Rel(h.cfg.QMLDir, path)
		if err != nil {
			log.Error().Err(err).Str("path", path).Msg("Failed to get relative path")
			return nil
		}

		// Generate ETag
		etag := h.generateETag(path, info)

		files = append(files, models.FileInfo{
			Path:         filepath.ToSlash(relPath), // Use forward slashes for consistency
			Size:         info.Size(),
			LastModified: info.ModTime(),
			ETag:         etag,
		})

		return nil
	})

	if err != nil {
		log.Error().Err(err).Msg("Failed to walk QML directory")
		h.respondError(w, http.StatusInternalServerError, "Failed to list files")
		return
	}

	h.respondJSON(w, http.StatusOK, models.FileListResponse{Files: files})
}

// GetFile handles GET /api/file/{name}
func (h *Handler) GetFile(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	name := vars["name"]

	// Validate and sanitize the file path
	filePath, err := h.getSafeFilePath(name)
	if err != nil {
		log.Warn().Err(err).Str("name", name).Msg("Invalid file path")
		h.respondError(w, http.StatusForbidden, err.Error())
		return
	}

	// Check if file exists
	info, err := os.Stat(filePath)
	if err != nil {
		if os.IsNotExist(err) {
			h.respondError(w, http.StatusNotFound, "File not found")
			return
		}
		log.Error().Err(err).Str("path", filePath).Msg("Failed to stat file")
		h.respondError(w, http.StatusInternalServerError, "Failed to access file")
		return
	}

	// Check if it's a directory
	if info.IsDir() {
		h.respondError(w, http.StatusBadRequest, "Path is a directory, not a file")
		return
	}

	// Check file extension
	if !h.isAllowedExtension(filePath) {
		h.respondError(w, http.StatusBadRequest, "File type not allowed")
		return
	}

	// Check file size
	if info.Size() > h.cfg.MaxFileSize {
		h.respondError(w, http.StatusRequestEntityTooLarge, "File too large")
		return
	}

	// Generate ETag
	etag := h.generateETag(filePath, info)

	// Check If-None-Match header for conditional request
	ifNoneMatch := r.Header.Get("If-None-Match")
	if ifNoneMatch == etag {
		w.WriteHeader(http.StatusNotModified)
		return
	}

	// Open file
	file, err := os.Open(filePath)
	if err != nil {
		log.Error().Err(err).Str("path", filePath).Msg("Failed to open file")
		h.respondError(w, http.StatusInternalServerError, "Failed to open file")
		return
	}
	defer file.Close()

	// Set headers
	w.Header().Set("Content-Type", "application/x-qml")
	w.Header().Set("ETag", etag)
	w.Header().Set("Last-Modified", info.ModTime().UTC().Format(http.TimeFormat))
	w.Header().Set("Cache-Control", "no-cache")

	// Copy file content to response
	_, err = io.Copy(w, file)
	if err != nil {
		log.Error().Err(err).Str("path", filePath).Msg("Failed to write file content")
	}
}

// GetFileInfo handles GET /api/file/{name}/info
func (h *Handler) GetFileInfo(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	name := vars["name"]

	// Validate and sanitize the file path
	filePath, err := h.getSafeFilePath(name)
	if err != nil {
		log.Warn().Err(err).Str("name", name).Msg("Invalid file path")
		h.respondError(w, http.StatusForbidden, err.Error())
		return
	}

	// Check if file exists
	info, err := os.Stat(filePath)
	if err != nil {
		if os.IsNotExist(err) {
			h.respondError(w, http.StatusNotFound, "File not found")
			return
		}
		log.Error().Err(err).Str("path", filePath).Msg("Failed to stat file")
		h.respondError(w, http.StatusInternalServerError, "Failed to access file")
		return
	}

	// Check if it's a directory
	if info.IsDir() {
		h.respondError(w, http.StatusBadRequest, "Path is a directory, not a file")
		return
	}

	// Check file extension
	if !h.isAllowedExtension(filePath) {
		h.respondError(w, http.StatusBadRequest, "File type not allowed")
		return
	}

	// Generate ETag
	etag := h.generateETag(filePath, info)

	// Get relative path
	relPath, err := filepath.Rel(h.cfg.QMLDir, filePath)
	if err != nil {
		log.Error().Err(err).Str("path", filePath).Msg("Failed to get relative path")
		h.respondError(w, http.StatusInternalServerError, "Failed to process file")
		return
	}

	h.respondJSON(w, http.StatusOK, models.FileInfo{
		Path:         filepath.ToSlash(relPath),
		Size:         info.Size(),
		LastModified: info.ModTime(),
		ETag:         etag,
	})
}

// HealthCheck handles GET /api/health
func (h *Handler) HealthCheck(w http.ResponseWriter, r *http.Request) {
	h.respondJSON(w, http.StatusOK, models.HealthResponse{
		Status:  "healthy",
		Version: h.cfg.GetVersion(),
		Uptime:  h.cfg.GetUptime(),
	})
}

// ServerInfo handles GET /api/info
func (h *Handler) ServerInfo(w http.ResponseWriter, r *http.Request) {
	h.respondJSON(w, http.StatusOK, models.InfoResponse{
		Version: h.cfg.Version,
		Features: models.Features{
			SSE:         true,
			WebSocket:   false,
			Compression: true,
		},
		MaxFileSize:       h.cfg.MaxFileSize,
		AllowedExtensions: h.cfg.AllowedExtensions,
	})
}

// getSafeFilePath validates and returns a safe file path
func (h *Handler) getSafeFilePath(name string) (string, error) {
	// Clean the input path
	cleanName := filepath.Clean(name)

	// Check for path traversal attempts
	if strings.Contains(cleanName, "..") {
		return "", fmt.Errorf("path traversal detected")
	}

	// Join with QML directory
	fullPath := filepath.Join(h.cfg.QMLDir, cleanName)

	// Ensure the resulting path is still within the QML directory
	absQMLDir, err := filepath.Abs(h.cfg.QMLDir)
	if err != nil {
		return "", fmt.Errorf("failed to get absolute QML directory path")
	}

	absFullPath, err := filepath.Abs(fullPath)
	if err != nil {
		return "", fmt.Errorf("failed to get absolute file path")
	}

	rel, err := filepath.Rel(absQMLDir, absFullPath)
	if err != nil {
		return "", fmt.Errorf("failed to verify path is within QML directory")
	}

	// Check if the relative path starts with ".."
	if strings.HasPrefix(rel, "..") {
		return "", fmt.Errorf("path is outside the allowed directory")
	}

	return absFullPath, nil
}

// isAllowedExtension checks if the file has an allowed extension
func (h *Handler) isAllowedExtension(path string) bool {
	ext := strings.ToLower(filepath.Ext(path))
	for _, allowed := range h.cfg.AllowedExtensions {
		if ext == allowed {
			return true
		}
	}
	return false
}

// generateETag generates an ETag for a file based on its modification time and size
func (h *Handler) generateETag(path string, info os.FileInfo) string {
	// Use modification time and size for ETag
	data := fmt.Sprintf("%s-%d-%d", path, info.ModTime().UnixNano(), info.Size())
	hash := sha256.Sum256([]byte(data))
	return hex.EncodeToString(hash[:])[:16] // Use first 16 characters
}

// respondJSON sends a JSON response
func (h *Handler) respondJSON(w http.ResponseWriter, status int, data interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)

	if err := json.NewEncoder(w).Encode(data); err != nil {
		log.Error().Err(err).Msg("Failed to encode JSON response")
	}
}

// respondError sends an error response
func (h *Handler) respondError(w http.ResponseWriter, status int, message string) {
	h.respondJSON(w, status, models.ErrorResponse{
		Error:   http.StatusText(status),
		Message: message,
	})
}
