package cmd

import (
	"net/http"
	"testing"
	"time"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/config"
)

func TestHTTPServerTimeoutsLeaveTimeForAnalysisResponse(t *testing.T) {
	runnerTimeout := 23 * time.Second
	server := newHTTPServer(config.Config{
		HTTPAddress:   "127.0.0.1:0",
		RunnerTimeout: runnerTimeout,
	}, http.NotFoundHandler())

	if server.WriteTimeout != readTimeout+runnerTimeout+writeGracePeriod {
		t.Fatalf("unexpected write timeout: %s", server.WriteTimeout)
	}
	if server.WriteTimeout <= readTimeout+runnerTimeout {
		t.Fatalf("write timeout must include response grace: %s", server.WriteTimeout)
	}
	if server.ReadHeaderTimeout != readHeaderTimeout || server.ReadTimeout != readTimeout || server.IdleTimeout != idleTimeout {
		t.Fatalf("unexpected server timeouts: %+v", server)
	}
}
