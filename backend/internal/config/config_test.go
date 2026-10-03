package config

import "testing"

func TestLoadUsesLoopbackAddressByDefault(t *testing.T) {
	t.Setenv("HTTP_ADDRESS", "")
	t.Setenv("AST_PRINTER_PATH", "")
	t.Setenv("AST_RUNNER_TIMEOUT", "")

	cfg, err := Load()
	if err != nil {
		t.Fatalf("load config: %v", err)
	}
	if cfg.HTTPAddress != "127.0.0.1:8080" {
		t.Fatalf("expected loopback address, got %q", cfg.HTTPAddress)
	}
}
