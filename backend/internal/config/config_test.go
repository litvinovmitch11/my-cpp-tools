package config

import (
	"testing"
	"time"
)

func TestLoadUsesLoopbackAddressByDefault(t *testing.T) {
	t.Setenv("HTTP_ADDRESS", "")
	t.Setenv("AST_PRINTER_PATH", "")
	t.Setenv("AST_RUNNER_TIMEOUT", "")
	t.Setenv("AST_RUNNER_MAX_CONCURRENT", "")

	cfg, err := Load()
	if err != nil {
		t.Fatalf("load config: %v", err)
	}
	if cfg.HTTPAddress != "127.0.0.1:8080" {
		t.Fatalf("expected loopback address, got %q", cfg.HTTPAddress)
	}
	if cfg.MaxConcurrent != 2 {
		t.Fatalf("expected default concurrency 2, got %d", cfg.MaxConcurrent)
	}
}

func TestLoadReadsOverrides(t *testing.T) {
	t.Setenv("HTTP_ADDRESS", "127.0.0.1:9090")
	t.Setenv("AST_PRINTER_PATH", "/tmp/AstPrinter")
	t.Setenv("AST_RUNNER_TIMEOUT", "3s")
	t.Setenv("AST_RUNNER_MAX_CONCURRENT", "4")

	cfg, err := Load()
	if err != nil {
		t.Fatalf("load config: %v", err)
	}
	if cfg.HTTPAddress != "127.0.0.1:9090" || cfg.ASTPrinter != "/tmp/AstPrinter" {
		t.Fatalf("unexpected config: %+v", cfg)
	}
	if cfg.RunnerTimeout != 3*time.Second || cfg.MaxConcurrent != 4 {
		t.Fatalf("unexpected runner config: %+v", cfg)
	}
}

func TestLoadRejectsInvalidRunnerSettings(t *testing.T) {
	tests := []struct {
		name  string
		env   string
		value string
	}{
		{name: "invalid timeout", env: "AST_RUNNER_TIMEOUT", value: "later"},
		{name: "zero timeout", env: "AST_RUNNER_TIMEOUT", value: "0s"},
		{name: "invalid concurrency", env: "AST_RUNNER_MAX_CONCURRENT", value: "many"},
		{name: "zero concurrency", env: "AST_RUNNER_MAX_CONCURRENT", value: "0"},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			t.Setenv("AST_RUNNER_TIMEOUT", "")
			t.Setenv("AST_RUNNER_MAX_CONCURRENT", "")
			t.Setenv(test.env, test.value)
			if _, err := Load(); err == nil {
				t.Fatal("expected configuration error")
			}
		})
	}
}
