package main

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gorilla/mux"
	"github.com/qmlnetdebugger/server/internal/config"
	"github.com/qmlnetdebugger/server/internal/handlers"
	"github.com/qmlnetdebugger/server/internal/middleware"
	"github.com/qmlnetdebugger/server/internal/models"
	"github.com/qmlnetdebugger/server/internal/watcher"
	"github.com/rs/cors"
	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
)

func main() {
	// Load configuration
	cfg, err := config.New()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to load configuration: %v\n", err)
		os.Exit(1)
	}

	// Configure logging
	configureLogging(cfg)

	// Log startup information
	log.Info().
		Str("version", cfg.Version).
		Str("address", cfg.GetAddress()).
		Str("qml_dir", cfg.QMLDir).
		Msg("Starting QmlNetDebugger server")

	// Create SSE broker
	sseBroker := handlers.NewSSEBroker()

	// Create handlers
	h := handlers.New(cfg)

	// Create router
	router := mux.NewRouter()

	// Apply middleware
	router.Use(middleware.RecoveryMiddleware)
	router.Use(middleware.LoggingMiddleware)
	router.Use(middleware.RequestIDMiddleware)
	router.Use(middleware.SecurityHeadersMiddleware)

	// Apply CORS using rs/cors for better control
	c := cors.New(cors.Options{
		AllowedOrigins:   []string{"*"},
		AllowedMethods:   []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
		AllowedHeaders:   []string{"Content-Type", "Authorization", "X-Requested-With", "If-None-Match"},
		AllowCredentials: true,
		MaxAge:           86400,
	})
	router.Use(c.Handler)

	// Apply gzip compression (skip SSE endpoints)
	router.Use(func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			// Skip gzip for SSE
			if r.URL.Path == "/api/events" {
				next.ServeHTTP(w, r)
				return
			}
			middleware.GzipMiddleware(next).ServeHTTP(w, r)
		})
	})

	// Register API routes
	api := router.PathPrefix("/api").Subrouter()

	// File endpoints - more specific routes first
	api.HandleFunc("/files/bundle", h.BundleFiles).Methods("GET")
	api.HandleFunc("/files", h.ListFiles).Methods("GET")
	api.HandleFunc("/file/{name:.*}/info", h.GetFileInfo).Methods("GET")
	api.HandleFunc("/file/{name:.*}", h.GetFile).Methods("GET")

	// SSE endpoint
	api.HandleFunc("/events", h.HandleSSE(sseBroker)).Methods("GET")

	// Health and info endpoints
	api.HandleFunc("/health", h.HealthCheck).Methods("GET")
	api.HandleFunc("/info", h.ServerInfo).Methods("GET")

	// Create and start file watcher
	fileWatcher, err := watcher.New(
		cfg.QMLDir,
		cfg.AllowedExtensions,
		func(event models.FileChangedEvent) {
			// Broadcast file change to SSE clients
			sseBroker.Broadcast(event)
		},
	)
	if err != nil {
		log.Fatal().Err(err).Msg("Failed to create file watcher")
	}
	fileWatcher.Start()
	defer fileWatcher.Stop()

	// Create HTTP server
	server := &http.Server{
		Addr:         cfg.GetAddress(),
		Handler:      router,
		ReadTimeout:  30 * time.Second,
		WriteTimeout: 30 * time.Second,
		IdleTimeout:  120 * time.Second,
	}

	// Start server in a goroutine
	serverErrors := make(chan error, 1)
	go func() {
		log.Info().Msg("Server listening")
		if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			serverErrors <- err
		}
	}()

	// Handle shutdown gracefully
	shutdown := make(chan os.Signal, 1)
	signal.Notify(shutdown, os.Interrupt, syscall.SIGTERM)

	// Wait for shutdown signal or server error
	select {
	case <-shutdown:
		log.Info().Msg("Shutdown signal received")
	case err := <-serverErrors:
		log.Fatal().Err(err).Msg("Server error")
	}

	// Graceful shutdown
	log.Info().Msg("Shutting down server...")

	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()

	if err := server.Shutdown(ctx); err != nil {
		log.Error().Err(err).Msg("Server shutdown error")
	}

	log.Info().Msg("Server stopped")
}

// configureLogging configures the zerolog logger
func configureLogging(cfg *config.Config) {
	// Set log level
	zerolog.SetGlobalLevel(parseLogLevel(cfg.LogLevel))

	// Configure console writer
	output := zerolog.ConsoleWriter{
		Out:        os.Stdout,
		TimeFormat: time.RFC3339,
		NoColor:    false,
	}

	// Set global logger
	log.Logger = zerolog.New(output).With().Timestamp().Logger()

	// Set log level from environment if set
	if cfg.Verbose {
		zerolog.SetGlobalLevel(zerolog.DebugLevel)
		log.Debug().Msg("Debug logging enabled")
	}
}

// parseLogLevel converts string log level to zerolog level
func parseLogLevel(level string) zerolog.Level {
	switch level {
	case "debug":
		return zerolog.DebugLevel
	case "info":
		return zerolog.InfoLevel
	case "warn":
		return zerolog.WarnLevel
	case "error":
		return zerolog.ErrorLevel
	default:
		return zerolog.InfoLevel
	}
}
