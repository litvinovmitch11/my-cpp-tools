package api

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/analysis"
)

type builderFunc func(ctx context.Context, kind analysis.Kind, source string) (string, error)

func (f builderFunc) BuildGraph(ctx context.Context, kind analysis.Kind, source string) (string, error) {
	return f(ctx, kind, source)
}

func TestBuildGraphRoutesKinds(t *testing.T) {
	tests := []struct {
		name string
		path string
		kind analysis.Kind
		dot  string
	}{
		{name: "AST", path: "/api/v1/ast", kind: analysis.AST, dot: "digraph AST {}"},
		{name: "CFG", path: "/api/v1/cfg", kind: analysis.CFG, dot: "digraph CFG {}"},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			builder := builderFunc(func(_ context.Context, kind analysis.Kind, source string) (string, error) {
				if kind != test.kind {
					t.Fatalf("unexpected analysis kind: got %q, want %q", kind, test.kind)
				}
				if source != "int main() {}" {
					t.Fatalf("unexpected source: %q", source)
				}
				return test.dot, nil
			})

			response := serveRequest(builder, test.path, `{"code":"int main() {}"}`)
			if response.Code != http.StatusOK {
				t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
			}
			if got := response.Header().Get("Content-Type"); got != "application/json" {
				t.Fatalf("expected JSON response, got %q", got)
			}
			assertSecurityHeaders(t, response)
			if got := response.Body.String(); got != "{\"dot\":\""+test.dot+"\"}\n" {
				t.Fatalf("unexpected response: %s", got)
			}
		})
	}
}

func TestBuildGraphAcceptsEscapedSourceWithinCodeLimit(t *testing.T) {
	code := "int main() {}\n" + strings.Repeat("\n", 200<<10)
	builder := builderFunc(func(_ context.Context, kind analysis.Kind, source string) (string, error) {
		if kind != analysis.AST {
			t.Fatalf("unexpected kind: %q", kind)
		}
		if source != code {
			t.Fatalf("unexpected source length: got %d, want %d", len(source), len(code))
		}
		return "digraph AST {}", nil
	})
	payload, err := json.Marshal(graphRequest{Code: code})
	if err != nil {
		t.Fatalf("encode request: %v", err)
	}
	if len(payload) <= maxCodeBytes+(4<<10) {
		t.Fatalf("test payload must exercise the escaped-body allowance")
	}

	response := serveRequest(builder, "/api/v1/ast", string(payload))
	if response.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
	}
}

func TestBuildGraphAcceptsSourceAtLimitForEveryKind(t *testing.T) {
	code := strings.Repeat("x", maxCodeBytes)
	tests := []struct {
		name string
		path string
		kind analysis.Kind
	}{
		{name: "AST", path: astPath, kind: analysis.AST},
		{name: "CFG", path: cfgPath, kind: analysis.CFG},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			builder := builderFunc(func(_ context.Context, kind analysis.Kind, source string) (string, error) {
				if kind != test.kind || source != code {
					t.Fatalf("unexpected build request: kind=%q source length=%d", kind, len(source))
				}
				return "digraph {}", nil
			})
			payload, err := json.Marshal(graphRequest{Code: code})
			if err != nil {
				t.Fatalf("encode request: %v", err)
			}

			response := serveRequest(builder, test.path, string(payload))
			if response.Code != http.StatusOK {
				t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
			}
		})
	}
}

func TestBuildGraphRejectsInvalidRequests(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ analysis.Kind, _ string) (string, error) {
		t.Fatal("builder must not be called")
		return "", nil
	})
	tests := []struct {
		name    string
		body    string
		status  int
		code    string
		message string
	}{
		{name: "malformed JSON", body: `{`, status: http.StatusBadRequest, code: "invalid_request", message: "request body must be valid JSON"},
		{name: "unknown field", body: `{"source":"int main() {}"}`, status: http.StatusBadRequest, code: "invalid_request", message: "request body must be valid JSON"},
		{name: "trailing value", body: `{"code":"ok"} {}`, status: http.StatusBadRequest, code: "invalid_request", message: "request body must contain one JSON object"},
		{name: "empty code", body: `{"code":""}`, status: http.StatusBadRequest, code: "invalid_request", message: "code must not be empty"},
		{name: "source too large", body: `{"code":"` + strings.Repeat("x", maxCodeBytes+1) + `"}`, status: http.StatusRequestEntityTooLarge, code: "source_too_large", message: "code exceeds 256 KiB"},
		{name: "body too large", body: strings.Repeat(" ", maxBodyBytes+1), status: http.StatusRequestEntityTooLarge, code: "request_too_large", message: "request body is too large"},
		{name: "trailing body too large", body: `{"code":"ok"}` + strings.Repeat(" ", maxBodyBytes), status: http.StatusRequestEntityTooLarge, code: "request_too_large", message: "request body is too large"},
	}

	for _, path := range []string{astPath, cfgPath} {
		for _, test := range tests {
			t.Run(path+"/"+test.name, func(t *testing.T) {
				response := serveRequest(builder, path, test.body)
				assertErrorResponse(t, response, test.status, apiError{Code: test.code, Message: test.message})
			})
		}
	}
}

func TestBuildGraphRequiresJSONContentType(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ analysis.Kind, _ string) (string, error) {
		t.Fatal("builder must not be called")
		return "", nil
	})
	tests := []struct {
		name        string
		contentType string
	}{
		{name: "plain text", contentType: "text/plain"},
		{name: "malformed", contentType: "application/json; charset"},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			request := httptest.NewRequest(http.MethodPost, astPath, strings.NewReader(`{"code":"int main() {}"}`))
			if test.contentType != "" {
				request.Header.Set("Content-Type", test.contentType)
			}
			response := httptest.NewRecorder()
			NewRouter(builder).ServeHTTP(response, request)

			assertErrorResponse(t, response, http.StatusUnsupportedMediaType, apiError{
				Code:    "unsupported_media_type",
				Message: "Content-Type must be application/json",
			})
		})
	}
}

func TestBuildGraphAcceptsMissingContentTypeForCompatibility(t *testing.T) {
	builder := builderFunc(func(_ context.Context, kind analysis.Kind, _ string) (string, error) {
		if kind != analysis.AST {
			t.Fatalf("unexpected kind: %q", kind)
		}
		return "digraph AST {}", nil
	})
	request := httptest.NewRequest(http.MethodPost, astPath, strings.NewReader(`{"code":"int main() {}"}`))
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
	}
}

func TestBuildGraphAcceptsJSONContentTypeParameters(t *testing.T) {
	builder := builderFunc(func(_ context.Context, kind analysis.Kind, _ string) (string, error) {
		if kind != analysis.CFG {
			t.Fatalf("unexpected kind: %q", kind)
		}
		return "digraph CFG {}", nil
	})
	request := httptest.NewRequest(http.MethodPost, cfgPath, strings.NewReader(`{"code":"int main() {}"}`))
	request.Header.Set("Content-Type", "application/json; charset=utf-8")
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d: %s", response.Code, response.Body.String())
	}
}

func TestBuildGraphMapsErrorsForEveryKind(t *testing.T) {
	errorTests := []struct {
		name        string
		err         error
		status      int
		code        string
		message     func(string) string
		diagnostics string
	}{
		{name: "invalid source", err: &analysis.SourceError{Diagnostics: "input.cpp:1:1: error"}, status: http.StatusUnprocessableEntity, code: "invalid_source", message: func(string) string { return "source code could not be parsed" }, diagnostics: "input.cpp:1:1: error"},
		{name: "timeout", err: analysis.ErrTimeout, status: http.StatusGatewayTimeout, code: "builder_timeout", message: func(label string) string { return label + " builder timed out" }},
		{name: "large graph", err: analysis.ErrOutputTooLarge, status: http.StatusUnprocessableEntity, code: "graph_too_large", message: func(string) string { return "generated graph is too large" }},
		{name: "busy", err: analysis.ErrBusy, status: http.StatusServiceUnavailable, code: "builder_busy", message: func(label string) string { return label + " builder is at capacity" }},
		{name: "internal", err: errors.New("secret internal error"), status: http.StatusInternalServerError, code: "internal_error", message: func(label string) string { return label + " graph could not be generated" }},
	}
	kinds := []struct {
		name string
		path string
		kind analysis.Kind
	}{
		{name: "AST", path: "/api/v1/ast", kind: analysis.AST},
		{name: "CFG", path: "/api/v1/cfg", kind: analysis.CFG},
	}

	for _, kind := range kinds {
		for _, errorTest := range errorTests {
			t.Run(kind.name+"/"+errorTest.name, func(t *testing.T) {
				builder := builderFunc(func(_ context.Context, gotKind analysis.Kind, _ string) (string, error) {
					if gotKind != kind.kind {
						t.Fatalf("unexpected kind: %q", gotKind)
					}
					return "", errorTest.err
				})

				response := serveRequest(builder, kind.path, `{"code":"int main() {}"}`)
				assertErrorResponse(t, response, errorTest.status, apiError{
					Code:        errorTest.code,
					Message:     errorTest.message(kind.name),
					Diagnostics: errorTest.diagnostics,
				})
				if strings.Contains(response.Body.String(), "secret internal error") {
					t.Fatalf("internal error leaked to response: %s", response.Body.String())
				}
			})
		}
	}
}

func TestHealth(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ analysis.Kind, _ string) (string, error) {
		t.Fatal("builder must not be called")
		return "", nil
	})
	request := httptest.NewRequest(http.MethodGet, healthPath, nil)
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	if response.Code != http.StatusOK || response.Body.String() != "{\"status\":\"ok\"}\n" {
		t.Fatalf("unexpected health response: status=%d body=%s", response.Code, response.Body.String())
	}
	if got := response.Header().Get("Content-Type"); got != "application/json" {
		t.Fatalf("expected JSON content type, got %q", got)
	}
	assertSecurityHeaders(t, response)
}

func TestUnknownRouteReturnsJSONNotFound(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ analysis.Kind, _ string) (string, error) {
		t.Fatal("builder must not be called")
		return "", nil
	})
	request := httptest.NewRequest(http.MethodPost, "/api/v1/cdg", strings.NewReader(`{"code":"int main() {}"}`))
	request.Header.Set("Content-Type", "application/json")
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)

	assertErrorResponse(t, response, http.StatusNotFound, apiError{Code: "not_found", Message: "route not found"})
}

func TestRoutesRejectUnsupportedMethods(t *testing.T) {
	builder := builderFunc(func(_ context.Context, _ analysis.Kind, _ string) (string, error) {
		t.Fatal("builder must not be called")
		return "", nil
	})
	tests := []struct {
		name   string
		method string
		path   string
		allow  string
	}{
		{name: "health", method: http.MethodPost, path: healthPath, allow: http.MethodGet},
		{name: "AST", method: http.MethodGet, path: astPath, allow: http.MethodPost},
		{name: "CFG", method: http.MethodDelete, path: cfgPath, allow: http.MethodPost},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			request := httptest.NewRequest(test.method, test.path, nil)
			response := httptest.NewRecorder()
			NewRouter(builder).ServeHTTP(response, request)

			assertErrorResponse(t, response, http.StatusMethodNotAllowed, apiError{Code: "method_not_allowed", Message: "method not allowed"})
			if got := response.Header().Get("Allow"); got != test.allow {
				t.Fatalf("expected Allow %q, got %q", test.allow, got)
			}
		})
	}
}

func serveRequest(builder analysis.Builder, path, body string) *httptest.ResponseRecorder {
	request := httptest.NewRequest(http.MethodPost, path, strings.NewReader(body))
	request.Header.Set("Content-Type", "application/json")
	response := httptest.NewRecorder()
	NewRouter(builder).ServeHTTP(response, request)
	return response
}

func assertErrorResponse(t *testing.T, response *httptest.ResponseRecorder, status int, want apiError) {
	t.Helper()
	if response.Code != status {
		t.Fatalf("expected status %d, got %d: %s", status, response.Code, response.Body.String())
	}
	expected, err := json.Marshal(errorBody{Error: want})
	if err != nil {
		t.Fatalf("encode expected error response: %v", err)
	}
	expected = append(expected, '\n')
	if got := response.Body.Bytes(); !bytes.Equal(got, expected) {
		t.Fatalf("unexpected error response: got %s, want %s", got, expected)
	}
	if got := response.Header().Get("Content-Type"); got != "application/json" {
		t.Fatalf("expected JSON content type, got %q", got)
	}
	assertSecurityHeaders(t, response)
}

func assertSecurityHeaders(t *testing.T, response *httptest.ResponseRecorder) {
	t.Helper()
	want := map[string]string{
		"Cache-Control":           "no-store",
		"Content-Security-Policy": "default-src 'none'; frame-ancestors 'none'",
		"Referrer-Policy":         "no-referrer",
		"X-Content-Type-Options":  "nosniff",
		"X-Frame-Options":         "DENY",
	}
	for name, value := range want {
		if got := response.Header().Get(name); got != value {
			t.Errorf("expected %s %q, got %q", name, value, got)
		}
	}
}
