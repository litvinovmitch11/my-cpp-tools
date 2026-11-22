package config

import (
	"testing"
	"time"
)

func TestLoadUsesLoopbackAddressByDefault(t *testing.T) {
	t.Setenv("HTTP_ADDRESS", "")
	t.Setenv("AST_PRINTER_PATH", "")
	t.Setenv("CFG_PRINTER_PATH", "")
	t.Setenv("CDG_PRINTER_PATH", "")
	t.Setenv("ANALYSIS_RUNNER_TIMEOUT", "")
	t.Setenv("ANALYSIS_RUNNER_MAX_CONCURRENT", "")

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
	if cfg.ASTPrinter != "AstPrinter" || cfg.CFGPrinter != "CfgPrinter" || cfg.CDGPrinter != "CdgPrinter" {
		t.Fatalf("unexpected default printer paths: %+v", cfg)
	}
}

func TestLoadReadsOverrides(t *testing.T) {
	t.Setenv("HTTP_ADDRESS", "127.0.0.1:9090")
	t.Setenv("AST_PRINTER_PATH", "/tmp/AstPrinter")
	t.Setenv("CFG_PRINTER_PATH", "/tmp/CfgPrinter")
	t.Setenv("CDG_PRINTER_PATH", "/tmp/CdgPrinter")
	t.Setenv("ANALYSIS_RUNNER_TIMEOUT", "3s")
	t.Setenv("ANALYSIS_RUNNER_MAX_CONCURRENT", "4")

	cfg, err := Load()
	if err != nil {
		t.Fatalf("load config: %v", err)
	}
	if cfg.HTTPAddress != "127.0.0.1:9090" || cfg.ASTPrinter != "/tmp/AstPrinter" || cfg.CFGPrinter != "/tmp/CfgPrinter" || cfg.CDGPrinter != "/tmp/CdgPrinter" {
		t.Fatalf("unexpected config: %+v", cfg)
	}
	if cfg.RunnerTimeout != 3*time.Second || cfg.MaxConcurrent != 4 {
		t.Fatalf("unexpected runner config: %+v", cfg)
	}
}

func TestLoadAcceptsLoopbackAddressForms(t *testing.T) {
	for _, address := range []string{"localhost:8080", "127.0.0.2:9090", "[::1]:8080"} {
		t.Run(address, func(t *testing.T) {
			t.Setenv("HTTP_ADDRESS", address)
			cfg, err := Load()
			if err != nil {
				t.Fatalf("load config: %v", err)
			}
			if cfg.HTTPAddress != address {
				t.Fatalf("expected address %q, got %q", address, cfg.HTTPAddress)
			}
		})
	}
}

func TestLoadRejectsNonLoopbackAddress(t *testing.T) {
	for _, address := range []string{
		"0.0.0.0:8080",
		"[::]:8080",
		"192.0.2.1:8080",
		"example.com:8080",
		"127.0.0.1",
		"127.0.0.1:http",
	} {
		t.Run(address, func(t *testing.T) {
			t.Setenv("HTTP_ADDRESS", address)
			if _, err := Load(); err == nil {
				t.Fatal("expected configuration error")
			}
		})
	}
}

func TestLoadRejectsInvalidRunnerSettings(t *testing.T) {
	tests := []struct {
		name  string
		env   string
		value string
	}{
		{name: "invalid timeout", env: "ANALYSIS_RUNNER_TIMEOUT", value: "later"},
		{name: "zero timeout", env: "ANALYSIS_RUNNER_TIMEOUT", value: "0s"},
		{name: "invalid concurrency", env: "ANALYSIS_RUNNER_MAX_CONCURRENT", value: "many"},
		{name: "zero concurrency", env: "ANALYSIS_RUNNER_MAX_CONCURRENT", value: "0"},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			t.Setenv("ANALYSIS_RUNNER_TIMEOUT", "")
			t.Setenv("ANALYSIS_RUNNER_MAX_CONCURRENT", "")
			t.Setenv(test.env, test.value)
			if _, err := Load(); err == nil {
				t.Fatal("expected configuration error")
			}
		})
	}
}
