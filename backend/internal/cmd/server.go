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

func RunServer() error {
	cfg, err := config.Load()
	if err != nil {
		return err
	}
	astRunner, err := runner.New(cfg.ASTPrinter, cfg.RunnerTimeout)
	if err != nil {
		return err
	}

	server := &http.Server{
		Addr:              cfg.HTTPAddress,
		Handler:           api.NewRouter(astRunner),
		ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout:       10 * time.Second,
		WriteTimeout:      15 * time.Second,
		IdleTimeout:       60 * time.Second,
	}

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
