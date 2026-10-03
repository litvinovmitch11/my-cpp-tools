package api

import (
	"context"
	"encoding/json"
	"errors"
	"io"
	"log/slog"
	"net/http"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/runner"
)

const (
	maxCodeBytes = 256 << 10
	maxBodyBytes = maxCodeBytes + (4 << 10)
)

type ASTBuilder interface {
	BuildAST(ctx context.Context, source string) (string, error)
}

type astRequest struct {
	Code string `json:"code"`
}

type astResponse struct {
	DOT string `json:"dot"`
}

type errorBody struct {
	Error apiError `json:"error"`
}

type apiError struct {
	Code        string `json:"code"`
	Message     string `json:"message"`
	Diagnostics string `json:"diagnostics,omitempty"`
}

func NewRouter(builder ASTBuilder) http.Handler {
	router := chi.NewRouter()
	router.Use(middleware.RequestID)
	router.Use(middleware.Recoverer)
	router.Use(logRequests)

	router.Get("/healthz", health)
	router.Post("/api/v1/ast", buildAST(builder))
	return router
}

func health(w http.ResponseWriter, _ *http.Request) {
	writeJSON(w, http.StatusOK, map[string]string{"status": "ok"})
}

func buildAST(builder ASTBuilder) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		r.Body = http.MaxBytesReader(w, r.Body, maxBodyBytes)
		decoder := json.NewDecoder(r.Body)
		decoder.DisallowUnknownFields()

		var request astRequest
		if err := decoder.Decode(&request); err != nil {
			writeRequestError(w, err)
			return
		}
		if err := ensureJSONEnd(decoder); err != nil {
			writeError(w, http.StatusBadRequest, "invalid_request", "request body must contain one JSON object", "")
			return
		}
		if request.Code == "" {
			writeError(w, http.StatusBadRequest, "invalid_request", "code must not be empty", "")
			return
		}
		if len(request.Code) > maxCodeBytes {
			writeError(w, http.StatusRequestEntityTooLarge, "source_too_large", "code exceeds 256 KiB", "")
			return
		}

		dot, err := builder.BuildAST(r.Context(), request.Code)
		if err == nil {
			writeJSON(w, http.StatusOK, astResponse{DOT: dot})
			return
		}

		var buildError *runner.BuildError
		switch {
		case errors.As(err, &buildError):
			writeError(w, http.StatusUnprocessableEntity, "invalid_source", "source code could not be parsed", buildError.Diagnostics)
		case errors.Is(err, runner.ErrTimeout):
			writeError(w, http.StatusGatewayTimeout, "builder_timeout", "AST builder timed out", "")
		case errors.Is(err, runner.ErrOutputTooLarge):
			writeError(w, http.StatusUnprocessableEntity, "graph_too_large", "generated graph is too large", "")
		default:
			slog.ErrorContext(
				r.Context(),
				"AST graph generation failed",
				"error", err,
				"request_id", middleware.GetReqID(r.Context()),
			)
			writeError(w, http.StatusInternalServerError, "internal_error", "AST graph could not be generated", "")
		}
	}
}

func ensureJSONEnd(decoder *json.Decoder) error {
	var extra any
	if err := decoder.Decode(&extra); !errors.Is(err, io.EOF) {
		if err == nil {
			return errors.New("unexpected trailing JSON value")
		}
		return err
	}
	return nil
}

func writeRequestError(w http.ResponseWriter, err error) {
	var maxBytesError *http.MaxBytesError
	if errors.As(err, &maxBytesError) {
		writeError(w, http.StatusRequestEntityTooLarge, "request_too_large", "request body is too large", "")
		return
	}
	writeError(w, http.StatusBadRequest, "invalid_request", "request body must be valid JSON", "")
}

func writeError(w http.ResponseWriter, status int, code, message, diagnostics string) {
	writeJSON(w, status, errorBody{Error: apiError{
		Code:        code,
		Message:     message,
		Diagnostics: diagnostics,
	}})
}

func writeJSON(w http.ResponseWriter, status int, value any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(value)
}
