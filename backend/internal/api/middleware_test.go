package api

import (
	"bytes"
	"context"
	"encoding/json"
	"io"
	"log/slog"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/analysis"
)

func TestRecoverPanicsReturnsJSONInternalError(t *testing.T) {
	replaceDefaultLogger(t, io.Discard)
	builder := builderFunc(func(_ context.Context, _ analysis.Kind, _ string) (string, error) {
		panic("internal detail")
	})

	response := serveRequest(builder, astPath, `{"code":"int main() {}"}`)
	assertErrorResponse(t, response, http.StatusInternalServerError, apiError{
		Code:    "internal_error",
		Message: "request could not be completed",
	})
	if bytes.Contains(response.Body.Bytes(), []byte("internal detail")) {
		t.Fatalf("panic detail leaked to response: %s", response.Body.String())
	}
}

func TestRequestLogIncludesOutcomeAndRequestID(t *testing.T) {
	var logs bytes.Buffer
	replaceDefaultLogger(t, &logs)
	builder := builderFunc(func(_ context.Context, _ analysis.Kind, _ string) (string, error) {
		t.Fatal("builder must not be called")
		return "", nil
	})

	request := httptest.NewRequest(http.MethodGet, healthPath, nil)
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	var record map[string]any
	if err := json.Unmarshal(logs.Bytes(), &record); err != nil {
		t.Fatalf("decode request log: %v\n%s", err, logs.String())
	}
	if record["msg"] != "request completed" || record["method"] != http.MethodGet || record["path"] != healthPath {
		t.Fatalf("unexpected request log: %+v", record)
	}
	if record["status"] != float64(http.StatusOK) || record["response_bytes"] == float64(0) {
		t.Fatalf("request outcome missing from log: %+v", record)
	}
	if requestID, ok := record["request_id"].(string); !ok || requestID == "" {
		t.Fatalf("request ID missing from log: %+v", record)
	}
}

func replaceDefaultLogger(t *testing.T, output io.Writer) {
	t.Helper()
	previous := slog.Default()
	slog.SetDefault(slog.New(slog.NewJSONHandler(output, nil)))
	t.Cleanup(func() { slog.SetDefault(previous) })
}
