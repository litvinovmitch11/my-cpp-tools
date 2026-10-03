package api

import (
	"context"
	"encoding/json"
	"errors"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/ast"
)

type builderFunc func(ctx context.Context, source string) (string, error)

func (f builderFunc) BuildAST(ctx context.Context, source string) (string, error) {
	return f(ctx, source)
}

func TestBuildAST(t *testing.T) {
	builder := builderFunc(func(_ context.Context, source string) (string, error) {
		if source != "int main() {}" {
			t.Fatalf("unexpected source: %q", source)
		}
		return "digraph AST {}", nil
	})

	request := httptest.NewRequest(http.MethodPost, "/api/v1/ast", strings.NewReader(`{"code":"int main() {}"}`))
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
	}
	if got := response.Header().Get("Content-Type"); got != "application/json" {
		t.Fatalf("expected JSON response, got %q", got)
	}
	if got := response.Body.String(); got != "{\"dot\":\"digraph AST {}\"}\n" {
		t.Fatalf("unexpected response: %s", got)
	}
}

func TestBuildASTAcceptsEscapedSourceWithinCodeLimit(t *testing.T) {
	code := "int main() {}\n" + strings.Repeat("\n", 200<<10)
	builder := builderFunc(func(_ context.Context, source string) (string, error) {
		if source != code {
			t.Fatalf("unexpected source length: got %d, want %d", len(source), len(code))
		}
		return "digraph AST {}", nil
	})
	payload, err := json.Marshal(astRequest{Code: code})
	if err != nil {
		t.Fatalf("encode request: %v", err)
	}
	if len(payload) <= maxCodeBytes+(4<<10) {
		t.Fatalf("test payload must exceed the old encoded body limit")
	}

	request := httptest.NewRequest(http.MethodPost, "/api/v1/ast", strings.NewReader(string(payload)))
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
	}
}

func TestBuildASTRejectsInvalidJSON(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ string) (string, error) {
		t.Fatal("builder must not be called")
		return "", nil
	})

	request := httptest.NewRequest(http.MethodPost, "/api/v1/ast", strings.NewReader(`{"source":"int main() {}"}`))
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusBadRequest {
		t.Fatalf("expected status 400, got %d: %s", response.Code, response.Body.String())
	}
}

func TestBuildASTReturnsDiagnostics(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ string) (string, error) {
		return "", &ast.BuildError{Diagnostics: "input.cpp:1:1: error"}
	})

	request := httptest.NewRequest(http.MethodPost, "/api/v1/ast", strings.NewReader(`{"code":"broken"}`))
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusUnprocessableEntity {
		t.Fatalf("expected status 422, got %d: %s", response.Code, response.Body.String())
	}
	if !strings.Contains(response.Body.String(), "input.cpp:1:1: error") {
		t.Fatalf("expected diagnostics in response: %s", response.Body.String())
	}
}

func TestBuildASTMapsTimeout(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ string) (string, error) {
		return "", ast.ErrTimeout
	})

	request := httptest.NewRequest(http.MethodPost, "/api/v1/ast", strings.NewReader(`{"code":"int main() {}"}`))
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusGatewayTimeout {
		t.Fatalf("expected status 504, got %d: %s", response.Code, response.Body.String())
	}
}

func TestBuildASTHidesInternalErrors(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ string) (string, error) {
		return "", errors.New("secret internal error")
	})

	request := httptest.NewRequest(http.MethodPost, "/api/v1/ast", strings.NewReader(`{"code":"int main() {}"}`))
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusInternalServerError {
		t.Fatalf("expected status 500, got %d: %s", response.Code, response.Body.String())
	}
	if strings.Contains(response.Body.String(), "secret internal error") {
		t.Fatalf("internal error leaked to response: %s", response.Body.String())
	}
}
