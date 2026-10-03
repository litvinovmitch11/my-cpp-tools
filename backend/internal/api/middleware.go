package api

import (
	"log/slog"
	"net/http"
	"time"
)

func logRequests(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		startedAt := time.Now()
		next.ServeHTTP(w, r)
		slog.InfoContext(
			r.Context(),
			"request completed",
			"method", r.Method,
			"path", r.URL.Path,
			"duration", time.Since(startedAt),
		)
	})
}
