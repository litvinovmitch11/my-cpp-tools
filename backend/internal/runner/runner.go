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

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/analysis"
)

const (
	maxGraphBytes       = 8 << 20
	maxDiagnosticsBytes = 64 << 10
)

type Runner struct {
	paths   BinaryPaths
	timeout time.Duration
	slots   chan struct{}
}

type BinaryPaths struct {
	AST string
	CFG string
}

type Options struct {
	Timeout       time.Duration
	MaxConcurrent int
}

func New(paths BinaryPaths, options Options) (*Runner, error) {
	if options.Timeout <= 0 {
		return nil, errors.New("runner timeout must be positive")
	}
	if options.MaxConcurrent <= 0 {
		return nil, errors.New("runner max concurrency must be positive")
	}
	resolvedPaths, err := resolveBinaryPaths(paths)
	if err != nil {
		return nil, err
	}
	return &Runner{
		paths:   resolvedPaths,
		timeout: options.Timeout,
		slots:   make(chan struct{}, options.MaxConcurrent),
	}, nil
}

func resolveBinaryPaths(paths BinaryPaths) (BinaryPaths, error) {
	astPath, err := resolveBinaryPath("AST", paths.AST)
	if err != nil {
		return BinaryPaths{}, err
	}
	cfgPath, err := resolveBinaryPath("CFG", paths.CFG)
	if err != nil {
		return BinaryPaths{}, err
	}
	return BinaryPaths{AST: astPath, CFG: cfgPath}, nil
}

func resolveBinaryPath(name, binaryPath string) (string, error) {
	resolvedPath, err := exec.LookPath(binaryPath)
	if err != nil {
		return "", fmt.Errorf("find %s builder %q: %w", name, binaryPath, err)
	}
	absolutePath, err := filepath.Abs(resolvedPath)
	if err != nil {
		return "", fmt.Errorf("resolve %s builder path: %w", name, err)
	}
	return absolutePath, nil
}

func (r *Runner) BuildGraph(ctx context.Context, kind analysis.Kind, source string) (string, error) {
	if err := ctx.Err(); err != nil {
		return "", err
	}
	binaryPath, err := r.binaryPath(kind)
	if err != nil {
		return "", err
	}
	select {
	case r.slots <- struct{}{}:
		defer func() { <-r.slots }()
	default:
		return "", analysis.ErrBusy
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
		binaryPath,
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
		return "", analysis.ErrTimeout
	}
	if errors.Is(runCtx.Err(), context.Canceled) {
		return "", runCtx.Err()
	}
	if err != nil {
		var exitError *exec.ExitError
		if errors.As(err, &exitError) {
			if exitError.ExitCode() == analysis.InvalidSourceExitCode {
				diagnostics := sanitizeDiagnostics(stderr.String(), sourcePath, tempDir)
				if stderr.truncated {
					diagnostics += "\n[diagnostics truncated]"
				}
				return "", &analysis.SourceError{Diagnostics: strings.TrimSpace(diagnostics)}
			}
			return "", fmt.Errorf("%s analysis tool failed with exit code %d", kind, exitError.ExitCode())
		}
		return "", fmt.Errorf("start %s analysis tool: %w", kind, err)
	}
	if stdout.truncated {
		return "", analysis.ErrOutputTooLarge
	}
	if stdout.buffer.Len() == 0 {
		return "", fmt.Errorf("%s analysis tool returned empty output", kind)
	}

	return stdout.String(), nil
}

func sanitizeDiagnostics(diagnostics, sourcePath, tempDir string) string {
	diagnostics = strings.ReplaceAll(diagnostics, sourcePath, "input.cpp")
	return strings.ReplaceAll(diagnostics, tempDir, "<temporary-directory>")
}

func (r *Runner) binaryPath(kind analysis.Kind) (string, error) {
	switch kind {
	case analysis.AST:
		return r.paths.AST, nil
	case analysis.CFG:
		return r.paths.CFG, nil
	default:
		return "", kind.Validate()
	}
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
