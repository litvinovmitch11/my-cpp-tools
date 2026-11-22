package config

import (
	"fmt"
	"net"
	"os"
	"strconv"
	"strings"
	"time"
)

const (
	defaultHTTPAddress   = "127.0.0.1:8080"
	defaultASTPrinter    = "AstPrinter"
	defaultCFGPrinter    = "CfgPrinter"
	defaultCDGPrinter    = "CdgPrinter"
	defaultRunnerTimeout = 5 * time.Second
	defaultMaxConcurrent = 2
)

type Config struct {
	HTTPAddress   string
	ASTPrinter    string
	CFGPrinter    string
	CDGPrinter    string
	RunnerTimeout time.Duration
	MaxConcurrent int
}

func Load() (Config, error) {
	cfg := Config{
		HTTPAddress:   envOrDefault("HTTP_ADDRESS", defaultHTTPAddress),
		ASTPrinter:    envOrDefault("AST_PRINTER_PATH", defaultASTPrinter),
		CFGPrinter:    envOrDefault("CFG_PRINTER_PATH", defaultCFGPrinter),
		CDGPrinter:    envOrDefault("CDG_PRINTER_PATH", defaultCDGPrinter),
		RunnerTimeout: defaultRunnerTimeout,
		MaxConcurrent: defaultMaxConcurrent,
	}
	if err := validateLoopbackAddress(cfg.HTTPAddress); err != nil {
		return Config{}, err
	}
	if value := os.Getenv("ANALYSIS_RUNNER_MAX_CONCURRENT"); value != "" {
		maxConcurrent, err := strconv.Atoi(value)
		if err != nil || maxConcurrent <= 0 {
			return Config{}, fmt.Errorf("ANALYSIS_RUNNER_MAX_CONCURRENT must be a positive integer")
		}
		cfg.MaxConcurrent = maxConcurrent
	}

	if value := os.Getenv("ANALYSIS_RUNNER_TIMEOUT"); value != "" {
		timeout, err := time.ParseDuration(value)
		if err != nil || timeout <= 0 {
			return Config{}, fmt.Errorf("ANALYSIS_RUNNER_TIMEOUT must be a positive duration")
		}
		cfg.RunnerTimeout = timeout
	}

	return cfg, nil
}

func validateLoopbackAddress(address string) error {
	host, port, err := net.SplitHostPort(address)
	if err != nil || host == "" || port == "" {
		return fmt.Errorf("HTTP_ADDRESS must be a loopback host and port")
	}
	if _, err := strconv.ParseUint(port, 10, 16); err != nil {
		return fmt.Errorf("HTTP_ADDRESS must use a numeric port")
	}
	if strings.EqualFold(host, "localhost") {
		return nil
	}
	ip := net.ParseIP(host)
	if ip == nil || !ip.IsLoopback() {
		return fmt.Errorf("HTTP_ADDRESS must use a loopback host")
	}
	return nil
}

func envOrDefault(name, fallback string) string {
	if value := os.Getenv(name); value != "" {
		return value
	}
	return fallback
}
