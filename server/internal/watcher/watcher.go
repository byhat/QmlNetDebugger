package watcher

import (
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"

	"github.com/fsnotify/fsnotify"
	"github.com/qmlnetdebugger/server/internal/models"
	"github.com/rs/zerolog/log"
)

// FileChangeCallback is called when a file change is detected
type FileChangeCallback func(event models.FileChangedEvent)

// Watcher monitors a directory for file changes
type Watcher struct {
	watcher          *fsnotify.Watcher
	qmlDir           string
	callback         FileChangeCallback
	allowedExts      []string
	debounceMap      map[string]time.Time
	debounceMapMutex sync.Mutex
	debounceDelay    time.Duration
	stopChan         chan struct{}
}

// New creates a new file watcher
func New(qmlDir string, allowedExts []string, callback FileChangeCallback) (*Watcher, error) {
	fsWatcher, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, err
	}

	w := &Watcher{
		watcher:       fsWatcher,
		qmlDir:        qmlDir,
		callback:      callback,
		allowedExts:   allowedExts,
		debounceMap:   make(map[string]time.Time),
		debounceDelay: 500 * time.Millisecond, // Debounce rapid changes
		stopChan:      make(chan struct{}),
	}

	// Add the QML directory to the watcher
	if err := w.addDirectory(qmlDir); err != nil {
		fsWatcher.Close()
		return nil, err
	}

	return w, nil
}

// addDirectory recursively adds a directory to the watcher
func (w *Watcher) addDirectory(dir string) error {
	// Add the directory itself
	if err := w.watcher.Add(dir); err != nil {
		return err
	}

	// Walk subdirectories
	return filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if info.IsDir() && path != dir {
			// Skip hidden directories
			if strings.HasPrefix(filepath.Base(path), ".") {
				return filepath.SkipDir
			}

			// Add subdirectory to watcher
			if err := w.watcher.Add(path); err != nil {
				log.Warn().Err(err).Str("path", path).Msg("Failed to watch directory")
			}
		}

		return nil
	})
}

// Start begins watching for file changes
func (w *Watcher) Start() {
	go w.watchLoop()
	log.Info().Str("directory", w.qmlDir).Msg("File watcher started")
}

// Stop stops watching for file changes
func (w *Watcher) Stop() {
	close(w.stopChan)
	if err := w.watcher.Close(); err != nil {
		log.Error().Err(err).Msg("Failed to close file watcher")
	}
	log.Info().Msg("File watcher stopped")
}

// watchLoop processes file system events
func (w *Watcher) watchLoop() {
	debounceTicker := time.NewTicker(100 * time.Millisecond)
	defer debounceTicker.Stop()

	for {
		select {
		case <-w.stopChan:
			return

		case event, ok := <-w.watcher.Events:
			if !ok {
				return
			}

			w.handleEvent(event)

		case err, ok := <-w.watcher.Errors:
			if !ok {
				return
			}

			log.Error().Err(err).Msg("File watcher error")
		}
	}
}

// handleEvent processes a single file system event
func (w *Watcher) handleEvent(event fsnotify.Event) {
	// Get relative path
	relPath, err := filepath.Rel(w.qmlDir, event.Name)
	if err != nil {
		log.Warn().Err(err).Str("path", event.Name).Msg("Failed to get relative path")
		return
	}

	// Determine event type
	var action string
	switch {
	case event.Op&fsnotify.Create == fsnotify.Create:
		action = "created"
	case event.Op&fsnotify.Write == fsnotify.Write:
		action = "modified"
	case event.Op&fsnotify.Remove == fsnotify.Remove:
		action = "deleted"
	case event.Op&fsnotify.Rename == fsnotify.Rename:
		action = "deleted" // Treat rename as delete for simplicity
	default:
		return
	}

	// Debounce events
	if w.shouldDebounce(event.Name, action) {
		return
	}

	// Get file info for ETag (if file exists)
	var etag string
	if action != "deleted" {
		if info, err := os.Stat(event.Name); err == nil {
			etag = w.generateETag(event.Name, info)
		}
	}

	// Create and send event
	changeEvent := models.FileChangedEvent{
		Path:   filepath.ToSlash(relPath),
		Action: action,
		ETag:   etag,
	}

	log.Debug().
		Str("path", changeEvent.Path).
		Str("action", changeEvent.Action).
		Msg("File change detected")

	w.callback(changeEvent)
}

// shouldDebounce checks if an event should be debounced
func (w *Watcher) shouldDebounce(path, action string) bool {
	w.debounceMapMutex.Lock()
	defer w.debounceMapMutex.Unlock()

	key := path + ":" + action
	now := time.Now()

	if lastTime, exists := w.debounceMap[key]; exists {
		if now.Sub(lastTime) < w.debounceDelay {
			return true
		}
	}

	w.debounceMap[key] = now
	return false
}

// isAllowedExtension checks if the file has an allowed extension
func (w *Watcher) isAllowedExtension(path string) bool {
	ext := strings.ToLower(filepath.Ext(path))
	for _, allowed := range w.allowedExts {
		if ext == allowed {
			return true
		}
	}
	return false
}

// generateETag generates an ETag for a file
func (w *Watcher) generateETag(path string, info os.FileInfo) string {
	// Use modification time and size for ETag
	data := filepath.Base(path) + "-" + info.ModTime().String() + "-" + string(rune(info.Size()))
	hash := make([]byte, 16)
	for i := 0; i < len(data) && i < 16; i++ {
		hash[i] = data[i]
	}

	// Simple hash for demonstration
	result := ""
	for _, b := range hash {
		result += string(b)
	}

	if len(result) > 16 {
		result = result[:16]
	}

	return result
}
