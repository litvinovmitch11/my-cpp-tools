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

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/ast"
)

const (
	maxGraphBytes       = 8 << 20
	maxDiagnosticsBytes = 64 << 10
)

type Runner struct {
	binaryPath string
	timeout    time.Duration
	slots      chan struct{}
}

type Options struct {
	Timeout       time.Duration
	MaxConcurrent int
}

func New(binaryPath string, options Options) (*Runner, error) {
	if options.Timeout <= 0 {
		return nil, errors.New("runner timeout must be positive")
	}
	if options.MaxConcurrent <= 0 {
		return nil, errors.New("runner max concurrency must be positive")
	}
	resolvedPath, err := exec.LookPath(binaryPath)
	if err != nil {
		return nil, fmt.Errorf("find AST builder %q: %w", binaryPath, err)
	}
	absolutePath, err := filepath.Abs(resolvedPath)
	if err != nil {
		return nil, fmt.Errorf("resolve AST builder path: %w", err)
	}
	return &Runner{
		binaryPath: absolutePath,
		timeout:    options.Timeout,
		slots:      make(chan struct{}, options.MaxConcurrent),
	}, nil
}

func (r *Runner) BuildAST(ctx context.Context, source string) (string, error) {
	if err := ctx.Err(); err != nil {
		return "", err
	}
	select {
	case r.slots <- struct{}{}:
		defer func() { <-r.slots }()
	default:
		return "", ast.ErrBusy
	}

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
	command.Dir = tempDir
	command.Env = []string{
		"HOME=" + tempDir,
		"TMPDIR=" + tempDir,
		"LANG=C",
		"LC_ALL=C",
	}

	err = command.Run()
	if errors.Is(runCtx.Err(), context.DeadlineExceeded) {
		return "", ast.ErrTimeout
	}
	if errors.Is(runCtx.Err(), context.Canceled) {
		return "", runCtx.Err()
	}
	if stdout.truncated {
		return "", ast.ErrOutputTooLarge
	}
	if err != nil {
		var exitError *exec.ExitError
		if errors.As(err, &exitError) {
			diagnostics := strings.ReplaceAll(stderr.String(), sourcePath, "input.cpp")
			if stderr.truncated {
				diagnostics += "\n[diagnostics truncated]"
			}
			return "", &ast.BuildError{Diagnostics: strings.TrimSpace(diagnostics)}
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
