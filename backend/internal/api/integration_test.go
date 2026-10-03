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

func TestBuildASTIntegration(t *testing.T) {
	binaryPath := os.Getenv("AST_PRINTER_PATH")
	if binaryPath == "" {
		t.Skip("AST_PRINTER_PATH is not set")
	}
	astRunner, err := runner.New(binaryPath, 5*time.Second)
	if err != nil {
		t.Fatalf("create runner: %v", err)
	}

	request := httptest.NewRequest(
		http.MethodPost,
		"/api/v1/ast",
		strings.NewReader(`{"code":"int main() { return 0; }"}`),
	)
	response := httptest.NewRecorder()
	NewRouter(astRunner).ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
	}

	var body astResponse
	if err := json.NewDecoder(response.Body).Decode(&body); err != nil {
		t.Fatalf("decode response: %v", err)
	}
	if !strings.Contains(body.DOT, "digraph AST") || !strings.Contains(body.DOT, `label="main"`) {
		t.Fatalf("unexpected graph: %s", body.DOT)
	}
}
