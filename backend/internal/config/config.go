package config

import (
	"fmt"
	"os"
	"time"
)

const (
	defaultHTTPAddress   = "127.0.0.1:8080"
	defaultASTPrinter    = "AstPrinter"
	defaultRunnerTimeout = 5 * time.Second
)

type Config struct {
	HTTPAddress   string
	ASTPrinter    string
	RunnerTimeout time.Duration
}

func Load() (Config, error) {
	cfg := Config{
		HTTPAddress:   envOrDefault("HTTP_ADDRESS", defaultHTTPAddress),
		ASTPrinter:    envOrDefault("AST_PRINTER_PATH", defaultASTPrinter),
		RunnerTimeout: defaultRunnerTimeout,
	}

	if value := os.Getenv("AST_RUNNER_TIMEOUT"); value != "" {
		timeout, err := time.ParseDuration(value)
		if err != nil || timeout <= 0 {
			return Config{}, fmt.Errorf("AST_RUNNER_TIMEOUT must be a positive duration")
		}
		cfg.RunnerTimeout = timeout
	}

	return cfg, nil
}

func envOrDefault(name, fallback string) string {
	if value := os.Getenv(name); value != "" {
		return value
	}
	return fallback
}
