package config

import (
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"time"
)

// Config holds the server configuration
type Config struct {
	// Server settings
	Host string
	Port int

	// QML directory
	QMLDir string

	// Logging
	Verbose  bool
	LogLevel string

	// Server info
	Version   string
	StartTime time.Time

	// File settings
	MaxFileSize       int64
	AllowedExtensions []string
}

// New creates a new configuration from command-line arguments
func New() (*Config, error) {
	cfg := &Config{
		Version:           "1.0.0",
		StartTime:         time.Now(),
		MaxFileSize:       10 * 1024 * 1024, // 10 MB default
		AllowedExtensions: []string{".qml", ".js", ".json"},
	}

	// Parse command-line flags
	flag.StringVar(&cfg.Host, "host", "0.0.0.0", "Server host address")
	flag.IntVar(&cfg.Port, "port", 8080, "Server port")
	flag.StringVar(&cfg.QMLDir, "dir", "./qml", "QML files directory")
	flag.BoolVar(&cfg.Verbose, "verbose", false, "Enable verbose logging")
	flag.StringVar(&cfg.LogLevel, "log-level", "info", "Log level (debug, info, warn, error)")

	flag.Parse()

	// Validate and normalize QML directory
	if err := cfg.validateAndNormalizeQMLDir(); err != nil {
		return nil, err
	}

	// Set log level based on verbose flag
	if cfg.Verbose {
		cfg.LogLevel = "debug"
	}

	return cfg, nil
}

// validateAndNormalizeQMLDir validates and normalizes the QML directory path
func (c *Config) validateAndNormalizeQMLDir() error {
	// Convert to absolute path
	absPath, err := filepath.Abs(c.QMLDir)
	if err != nil {
		return fmt.Errorf("failed to get absolute path for QML directory: %w", err)
	}

	// Check if directory exists
	info, err := os.Stat(absPath)
	if err != nil {
		if os.IsNotExist(err) {
			return fmt.Errorf("QML directory does not exist: %s", absPath)
		}
		return fmt.Errorf("failed to access QML directory: %w", err)
	}

	// Check if it's a directory
	if !info.IsDir() {
		return fmt.Errorf("QML path is not a directory: %s", absPath)
	}

	// Normalize the path
	c.QMLDir = filepath.Clean(absPath)

	return nil
}

// GetAddress returns the server address in host:port format
func (c *Config) GetAddress() string {
	return fmt.Sprintf("%s:%d", c.Host, c.Port)
}

// GetUptime returns the server uptime in seconds
func (c *Config) GetUptime() int64 {
	return int64(time.Since(c.StartTime).Seconds())
}

// GetVersion returns the server version with additional info
func (c *Config) GetVersion() string {
	return fmt.Sprintf("%s (Go %s)", c.Version, runtime.Version())
}
