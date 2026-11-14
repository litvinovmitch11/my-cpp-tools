package api

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/runner"
)

func TestGraphEndpointsIntegration(t *testing.T) {
	handler := newNativeIntegrationHandler(t)
	tests := []struct {
		name      string
		path      string
		graphName string
	}{
		{name: "AST", path: "/api/v1/ast", graphName: "AST"},
		{name: "CFG", path: "/api/v1/cfg", graphName: "CFG"},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			request := httptest.NewRequest(
				http.MethodPost,
				test.path,
				strings.NewReader(`{"code":"int main() { return 0; }"}`),
			)
			request.Header.Set("Content-Type", "application/json")
			response := httptest.NewRecorder()
			handler.ServeHTTP(response, request)

			if response.Code != http.StatusOK {
				t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
			}

			var body graphResponse
			if err := json.NewDecoder(response.Body).Decode(&body); err != nil {
				t.Fatalf("decode response: %v", err)
			}
			if !strings.Contains(body.DOT, "digraph "+test.graphName) || !strings.Contains(body.DOT, `label="main : int (void)"`) {
				t.Fatalf("unexpected graph: %s", body.DOT)
			}
			if !strings.Contains(body.DOT, `bgcolor="transparent"`) {
				t.Fatalf("expected a transparent graph background: %s", body.DOT)
			}
		})
	}
}

func TestGraphEndpointsRejectInvalidSourceIntegration(t *testing.T) {
	handler := newNativeIntegrationHandler(t)
	tests := []struct {
		name string
		path string
	}{
		{name: "AST", path: "/api/v1/ast"},
		{name: "CFG", path: "/api/v1/cfg"},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			request := httptest.NewRequest(http.MethodPost, test.path, strings.NewReader(`{"code":"int main( {"}`))
			request.Header.Set("Content-Type", "application/json")
			response := httptest.NewRecorder()
			handler.ServeHTTP(response, request)

			if response.Code != http.StatusUnprocessableEntity {
				t.Fatalf("expected status 422, got %d: %s", response.Code, response.Body.String())
			}
			if !strings.Contains(response.Body.String(), `"code":"invalid_source"`) || !strings.Contains(response.Body.String(), "input.cpp") {
				t.Fatalf("expected sanitized source diagnostics: %s", response.Body.String())
			}
		})
	}
}

func newNativeIntegrationHandler(t *testing.T) http.Handler {
	t.Helper()
	astPath := os.Getenv("AST_PRINTER_PATH")
	cfgPath := os.Getenv("CFG_PRINTER_PATH")
	if astPath == "" || cfgPath == "" {
		t.Skip("AST_PRINTER_PATH and CFG_PRINTER_PATH are required")
	}
	analysisRunner, err := runner.New(runner.BinaryPaths{AST: astPath, CFG: cfgPath}, runner.Options{
		Timeout:       5 * time.Second,
		MaxConcurrent: 1,
	})
	if err != nil {
		t.Fatalf("create runner: %v", err)
	}
	return NewRouter(analysisRunner)
}
