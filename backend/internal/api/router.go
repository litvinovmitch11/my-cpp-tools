package api

import (
	"encoding/json"
	"errors"
	"io"
	"log/slog"
	"mime"
	"net/http"
	"strings"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/analysis"
)

const (
	maxCodeBytes = 256 << 10
	maxBodyBytes = 6*maxCodeBytes + (4 << 10)
	healthPath   = "/healthz"
	astPath      = "/api/v1/ast"
	cfgPath      = "/api/v1/cfg"
)

type graphRequest struct {
	Code string `json:"code"`
}

type graphResponse struct {
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

func NewRouter(builder analysis.Builder) http.Handler {
	router := chi.NewRouter()
	router.Use(middleware.RequestID)
	router.Use(securityHeaders)
	router.Use(logRequests)
	router.Use(recoverPanics)

	router.Get(healthPath, health)
	router.Post(astPath, buildGraph(analysis.AST, builder))
	router.Post(cfgPath, buildGraph(analysis.CFG, builder))
	router.NotFound(notFound)
	router.MethodNotAllowed(methodNotAllowed)
	return router
}

func health(w http.ResponseWriter, _ *http.Request) {
	writeJSON(w, http.StatusOK, map[string]string{"status": "ok"})
}

func notFound(w http.ResponseWriter, _ *http.Request) {
	writeError(w, http.StatusNotFound, "not_found", "route not found", "")
}

func methodNotAllowed(w http.ResponseWriter, r *http.Request) {
	switch r.URL.Path {
	case healthPath:
		w.Header().Set("Allow", http.MethodGet)
	case astPath, cfgPath:
		w.Header().Set("Allow", http.MethodPost)
	}
	writeError(w, http.StatusMethodNotAllowed, "method_not_allowed", "method not allowed", "")
}

func buildGraph(kind analysis.Kind, builder analysis.Builder) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if !acceptsJSONContentType(r) {
			writeError(w, http.StatusUnsupportedMediaType, "unsupported_media_type", "Content-Type must be application/json", "")
			return
		}
		r.Body = http.MaxBytesReader(w, r.Body, maxBodyBytes)
		decoder := json.NewDecoder(r.Body)
		decoder.DisallowUnknownFields()

		var request graphRequest
		if err := decoder.Decode(&request); err != nil {
			writeRequestError(w, err)
			return
		}
		if err := ensureJSONEnd(decoder); err != nil {
			var maxBytesError *http.MaxBytesError
			if errors.As(err, &maxBytesError) {
				writeRequestError(w, err)
				return
			}
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

		dot, err := builder.BuildGraph(r.Context(), kind, request.Code)
		if err == nil {
			writeJSON(w, http.StatusOK, graphResponse{DOT: dot})
			return
		}

		var sourceError *analysis.SourceError
		label := strings.ToUpper(string(kind))
		switch {
		case errors.As(err, &sourceError):
			writeError(w, http.StatusUnprocessableEntity, "invalid_source", "source code could not be parsed", sourceError.Diagnostics)
		case errors.Is(err, analysis.ErrTimeout):
			writeError(w, http.StatusGatewayTimeout, "builder_timeout", label+" builder timed out", "")
		case errors.Is(err, analysis.ErrOutputTooLarge):
			writeError(w, http.StatusUnprocessableEntity, "graph_too_large", "generated graph is too large", "")
		case errors.Is(err, analysis.ErrBusy):
			writeError(w, http.StatusServiceUnavailable, "builder_busy", label+" builder is at capacity", "")
		default:
			slog.ErrorContext(
				r.Context(),
				"graph generation failed",
				"analysis_kind", kind,
				"error", err,
				"request_id", middleware.GetReqID(r.Context()),
			)
			writeError(w, http.StatusInternalServerError, "internal_error", label+" graph could not be generated", "")
		}
	}
}

func acceptsJSONContentType(r *http.Request) bool {
	contentType := r.Header.Get("Content-Type")
	if contentType == "" {
		return true
	}
	mediaType, _, err := mime.ParseMediaType(contentType)
	return err == nil && mediaType == "application/json"
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
