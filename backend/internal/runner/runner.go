package runner

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

const (
	maxGraphBytes       = 8 << 20
	maxDiagnosticsBytes = 64 << 10
)

var (
	ErrTimeout        = errors.New("AST builder timed out")
	ErrOutputTooLarge = errors.New("AST graph is too large")
)

type BuildError struct {
	Diagnostics string
}

func (e *BuildError) Error() string {
	return "AST builder rejected the source code"
}

type Runner struct {
	binaryPath string
	timeout    time.Duration
}

func New(binaryPath string, timeout time.Duration) (*Runner, error) {
	resolvedPath, err := exec.LookPath(binaryPath)
	if err != nil {
		return nil, fmt.Errorf("find AST builder %q: %w", binaryPath, err)
	}
	return &Runner{binaryPath: resolvedPath, timeout: timeout}, nil
}

func (r *Runner) BuildAST(ctx context.Context, source string) (string, error) {
	tempDir, err := os.MkdirTemp("", "my-cpp-tools-")
	if err != nil {
		return "", fmt.Errorf("create temporary directory: %w", err)
	}
	defer os.RemoveAll(tempDir)

	sourcePath := filepath.Join(tempDir, "input.cpp")
	if err := os.WriteFile(sourcePath, []byte(source), 0o600); err != nil {
		return "", fmt.Errorf("write source file: %w", err)
	}

	runCtx, cancel := context.WithTimeout(ctx, r.timeout)
	defer cancel()

	stdout := newLimitedBuffer(maxGraphBytes)
	stderr := newLimitedBuffer(maxDiagnosticsBytes)
	command := exec.CommandContext(
		runCtx,
		r.binaryPath,
		sourcePath,
		"--",
		"-std=c++20",
	)
	command.Stdout = stdout
	command.Stderr = stderr

	err = command.Run()
	if errors.Is(runCtx.Err(), context.DeadlineExceeded) {
		return "", ErrTimeout
	}
	if stdout.truncated {
		return "", ErrOutputTooLarge
	}
	if err != nil {
		var exitError *exec.ExitError
		if errors.As(err, &exitError) {
			diagnostics := strings.ReplaceAll(stderr.String(), sourcePath, "input.cpp")
			if stderr.truncated {
				diagnostics += "\n[diagnostics truncated]"
			}
			return "", &BuildError{Diagnostics: strings.TrimSpace(diagnostics)}
		}
		return "", fmt.Errorf("start AST builder: %w", err)
	}

	return stdout.String(), nil
}

type limitedBuffer struct {
	buffer    bytes.Buffer
	limit     int
	truncated bool
}

func newLimitedBuffer(limit int) *limitedBuffer {
	return &limitedBuffer{limit: limit}
}

func (b *limitedBuffer) Write(data []byte) (int, error) {
	written := len(data)
	remaining := b.limit - b.buffer.Len()
	if remaining <= 0 {
		b.truncated = true
		return written, nil
	}
	if len(data) > remaining {
		_, _ = b.buffer.Write(data[:remaining])
		b.truncated = true
		return written, nil
	}
	_, _ = b.buffer.Write(data)
	return written, nil
}

func (b *limitedBuffer) String() string {
	return b.buffer.String()
}
