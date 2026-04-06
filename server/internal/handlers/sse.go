package handlers

import (
	"encoding/json"
	"fmt"
	"net/http"
	"sync"
	"time"

	"github.com/qmlnetdebugger/server/internal/models"
	"github.com/rs/zerolog/log"
)

// SSEClient represents a connected SSE client
type SSEClient struct {
	ID     string
	Events chan<- models.FileChangedEvent
}

// SSEBroker manages SSE connections and broadcasts events
type SSEBroker struct {
	clients      map[string]*SSEClient
	clientsMutex sync.RWMutex
	eventQueue   chan models.FileChangedEvent
}

// NewSSEBroker creates a new SSE broker
func NewSSEBroker() *SSEBroker {
	broker := &SSEBroker{
		clients:    make(map[string]*SSEClient),
		eventQueue: make(chan models.FileChangedEvent, 100),
	}

	// Start event broadcaster
	go broker.broadcast()

	return broker
}

// AddClient adds a new SSE client
func (b *SSEBroker) AddClient(client *SSEClient) {
	b.clientsMutex.Lock()
	defer b.clientsMutex.Unlock()

	b.clients[client.ID] = client
	log.Info().Str("client_id", client.ID).Msg("SSE client connected")
}

// RemoveClient removes an SSE client
func (b *SSEBroker) RemoveClient(clientID string) {
	b.clientsMutex.Lock()
	defer b.clientsMutex.Unlock()

	if client, exists := b.clients[clientID]; exists {
		close(client.Events)
		delete(b.clients, clientID)
		log.Info().Str("client_id", clientID).Msg("SSE client disconnected")
	}
}

// Broadcast sends an event to all connected clients
func (b *SSEBroker) Broadcast(event models.FileChangedEvent) {
	select {
	case b.eventQueue <- event:
	default:
		log.Warn().Msg("SSE event queue full, dropping event")
	}
}

// broadcast processes events from the queue and sends them to clients
func (b *SSEBroker) broadcast() {
	for event := range b.eventQueue {
		b.clientsMutex.RLock()

		for clientID, client := range b.clients {
			select {
			case client.Events <- event:
				// Event sent successfully
			default:
				// Client channel full, remove client
				log.Warn().Str("client_id", clientID).Msg("SSE client channel full, removing")
				go b.RemoveClient(clientID)
			}
		}

		b.clientsMutex.RUnlock()
	}
}

// GetClientCount returns the number of connected clients
func (b *SSEBroker) GetClientCount() int {
	b.clientsMutex.RLock()
	defer b.clientsMutex.RUnlock()
	return len(b.clients)
}

// HandleSSE handles GET /api/events
func (h *Handler) HandleSSE(broker *SSEBroker) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		// Get query parameters
		pathFilter := r.URL.Query().Get("path")

		// Set SSE headers
		w.Header().Set("Content-Type", "text/event-stream")
		w.Header().Set("Cache-Control", "no-cache")
		w.Header().Set("Connection", "keep-alive")
		w.Header().Set("X-Accel-Buffering", "no") // Disable nginx buffering

		// Create event channel
		events := make(chan models.FileChangedEvent, 10)

		// Generate client ID
		clientID := fmt.Sprintf("%s-%d", r.RemoteAddr, time.Now().UnixNano())

		// Create client
		client := &SSEClient{
			ID:     clientID,
			Events: events,
		}

		// Add client to broker
		broker.AddClient(client)

		// Ensure client is removed when connection closes
		defer broker.RemoveClient(clientID)

		// Create flusher
		flusher, ok := w.(http.Flusher)
		if !ok {
			log.Error().Msg("HTTP streaming not supported")
			return
		}

		// Send initial connection event
		h.sendSSEEvent(w, flusher, "connected", map[string]string{
			"client_id":   clientID,
			"server_time": time.Now().UTC().Format(time.RFC3339),
		})

		// Send keepalive events every 30 seconds
		keepalive := time.NewTicker(30 * time.Second)
		defer keepalive.Stop()

		// Handle client disconnect
		ctx := r.Context()

		for {
			select {
			case <-ctx.Done():
				// Client disconnected
				return

			case <-keepalive.C:
				// Send keepalive
				h.sendSSEEvent(w, flusher, "keepalive", nil)

			case event := <-events:
				// Check path filter
				if pathFilter != "" && event.Path != pathFilter {
					continue
				}

				// Send file change event
				h.sendSSEEvent(w, flusher, "file_changed", event)
			}
		}
	}
}

// sendSSEEvent sends an SSE event to the client
func (h *Handler) sendSSEEvent(w http.ResponseWriter, flusher http.Flusher, eventType string, data interface{}) {
	var eventData string

	if data != nil {
		jsonData, err := json.Marshal(data)
		if err != nil {
			log.Error().Err(err).Msg("Failed to marshal SSE event data")
			return
		}
		eventData = string(jsonData)
	}

	// Format SSE event
	if eventData != "" {
		fmt.Fprintf(w, "event: %s\n", eventType)
		fmt.Fprintf(w, "data: %s\n\n", eventData)
	} else {
		fmt.Fprintf(w, "event: %s\n\n", eventType)
	}

	// Flush to ensure immediate delivery
	flusher.Flush()
}
