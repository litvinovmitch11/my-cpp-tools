package cmd

import (
	"context"
	"errors"
	"log/slog"
	"net/http"
	"os/signal"
	"syscall"
	"time"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/api"
	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/config"
	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/runner"
)

const (
	readHeaderTimeout = 5 * time.Second
	readTimeout       = 10 * time.Second
	writeGracePeriod  = 5 * time.Second
	idleTimeout       = 60 * time.Second
)

func RunServer() error {
	cfg, err := config.Load()
	if err != nil {
		return err
	}
	analysisRunner, err := runner.New(runner.BinaryPaths{
		AST: cfg.ASTPrinter,
		CFG: cfg.CFGPrinter,
		CDG: cfg.CDGPrinter,
	}, runner.Options{
		Timeout:       cfg.RunnerTimeout,
		MaxConcurrent: cfg.MaxConcurrent,
	})
	if err != nil {
		return err
	}

	server := newHTTPServer(cfg, api.NewRouter(analysisRunner))

	shutdownSignal, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()

	serverError := make(chan error, 1)
	go func() {
		slog.Info("server started", "address", cfg.HTTPAddress)
		serverError <- server.ListenAndServe()
	}()

	select {
	case err := <-serverError:
		if errors.Is(err, http.ErrServerClosed) {
			return nil
		}
		return err
	case <-shutdownSignal.Done():
		shutdownContext, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		return server.Shutdown(shutdownContext)
	}
}

func newHTTPServer(cfg config.Config, handler http.Handler) *http.Server {
	return &http.Server{
		Addr:              cfg.HTTPAddress,
		Handler:           handler,
		ReadHeaderTimeout: readHeaderTimeout,
		ReadTimeout:       readTimeout,
		WriteTimeout:      readTimeout + cfg.RunnerTimeout + writeGracePeriod,
		IdleTimeout:       idleTimeout,
	}
}
