package runner

import (
	"context"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
	"time"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/ast"
)

func TestNewValidatesOptionsAndBinary(t *testing.T) {
	script := writeScript(t, "printf 'digraph AST {}'")
	tests := []struct {
		name    string
		path    string
		options Options
	}{
		{name: "missing binary", path: filepath.Join(t.TempDir(), "missing"), options: testOptions()},
		{name: "zero timeout", path: script, options: Options{MaxConcurrent: 1}},
		{name: "zero concurrency", path: script, options: Options{Timeout: time.Second}},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			if _, err := New(test.path, test.options); err == nil {
				t.Fatal("expected validation error")
			}
		})
	}
}

func TestBuildASTReturnsGraphAndUsesIsolatedEnvironment(t *testing.T) {
	t.Setenv("RUNNER_TEST_SECRET", "must-not-leak")
	script := writeScript(t, `
[ "$PWD" = "$(dirname "$1")" ] || exit 10
[ "$HOME" = "$PWD" ] || exit 11
[ "$TMPDIR" = "$PWD" ] || exit 12
[ -z "${RUNNER_TEST_SECRET:-}" ] || exit 13
printf 'digraph AST {}'
`)
	runner := newTestRunner(t, script, testOptions())

	graph, err := runner.BuildAST(context.Background(), "int main() {}")
	if err != nil {
		t.Fatalf("build AST: %v", err)
	}
	if graph != "digraph AST {}" {
		t.Fatalf("unexpected graph: %q", graph)
	}
}

func TestBuildASTReturnsSanitizedDiagnostics(t *testing.T) {
	script := writeScript(t, `
printf '%s:1:1: error: rejected\n' "$1" >&2
exit 1
`)
	runner := newTestRunner(t, script, testOptions())

	_, err := runner.BuildAST(context.Background(), "broken")
	var buildError *ast.BuildError
	if !errors.As(err, &buildError) {
		t.Fatalf("expected build error, got %v", err)
	}
	if buildError.Diagnostics != "input.cpp:1:1: error: rejected" {
		t.Fatalf("unexpected diagnostics: %q", buildError.Diagnostics)
	}
	if strings.Contains(buildError.Diagnostics, os.TempDir()) {
		t.Fatalf("temporary path leaked: %q", buildError.Diagnostics)
	}
}

func TestBuildASTTruncatesDiagnostics(t *testing.T) {
	headPath, err := exec.LookPath("head")
	if err != nil {
		t.Skip("head is not available")
	}
	script := writeScript(t, fmt.Sprintf(`
%q -c %d /dev/zero >&2
exit 1
`, headPath, maxDiagnosticsBytes+1))
	runner := newTestRunner(t, script, testOptions())

	_, err = runner.BuildAST(context.Background(), "broken")
	var buildError *ast.BuildError
	if !errors.As(err, &buildError) {
		t.Fatalf("expected build error, got %v", err)
	}
	if !strings.HasSuffix(buildError.Diagnostics, "[diagnostics truncated]") {
		t.Fatalf("expected truncation marker, got diagnostics length %d", len(buildError.Diagnostics))
	}
}

func TestBuildASTRejectsLargeGraph(t *testing.T) {
	headPath, err := exec.LookPath("head")
	if err != nil {
		t.Skip("head is not available")
	}
	script := writeScript(t, fmt.Sprintf(`%q -c %d /dev/zero`, headPath, maxGraphBytes+1))
	runner := newTestRunner(t, script, testOptions())

	_, err = runner.BuildAST(context.Background(), "int main() {}")
	if !errors.Is(err, ast.ErrOutputTooLarge) {
		t.Fatalf("expected output-too-large error, got %v", err)
	}
}

func TestBuildASTTimesOutAndReleasesCapacity(t *testing.T) {
	script := writeScript(t, `/bin/sleep 1`)
	runner := newTestRunner(t, script, Options{Timeout: 20 * time.Millisecond, MaxConcurrent: 1})

	for attempt := 0; attempt < 2; attempt++ {
		_, err := runner.BuildAST(context.Background(), "int main() {}")
		if !errors.Is(err, ast.ErrTimeout) {
			t.Fatalf("attempt %d: expected timeout, got %v", attempt, err)
		}
	}
}

func TestBuildASTHonorsCancellation(t *testing.T) {
	script := writeScript(t, `/bin/sleep 1`)
	runner := newTestRunner(t, script, Options{Timeout: time.Second, MaxConcurrent: 1})
	ctx, cancel := context.WithCancel(context.Background())
	cancel()

	_, err := runner.BuildAST(ctx, "int main() {}")
	if !errors.Is(err, context.Canceled) {
		t.Fatalf("expected cancellation, got %v", err)
	}
}

func TestBuildASTRejectsWhenAtCapacity(t *testing.T) {
	marker := filepath.Join(t.TempDir(), "started")
	script := writeScript(t, fmt.Sprintf(`
: > %q
/bin/sleep 0.1
printf 'digraph AST {}'
`, marker))
	runner := newTestRunner(t, script, Options{Timeout: time.Second, MaxConcurrent: 1})
	finished := make(chan error, 1)
	go func() {
		_, err := runner.BuildAST(context.Background(), "first")
		finished <- err
	}()
	waitForFile(t, marker)

	if _, err := runner.BuildAST(context.Background(), "second"); !errors.Is(err, ast.ErrBusy) {
		t.Fatalf("expected busy error, got %v", err)
	}
	if err := <-finished; err != nil {
		t.Fatalf("first build failed: %v", err)
	}
}

func TestBuildASTRemovesTemporaryDirectory(t *testing.T) {
	script := writeScript(t, `printf '%s' "$1"`)
	runner := newTestRunner(t, script, testOptions())

	sourcePath, err := runner.BuildAST(context.Background(), "int main() {}")
	if err != nil {
		t.Fatalf("build AST: %v", err)
	}
	if _, err := os.Stat(filepath.Dir(sourcePath)); !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("temporary directory still exists: %v", err)
	}
}

func TestLimitedBuffer(t *testing.T) {
	buffer := newLimitedBuffer(4)
	written, err := buffer.Write([]byte("abcdef"))
	if err != nil || written != 6 {
		t.Fatalf("unexpected write result: written=%d err=%v", written, err)
	}
	if got := buffer.String(); got != "abcd" {
		t.Fatalf("unexpected contents: %q", got)
	}
	if !buffer.truncated {
		t.Fatal("expected truncation")
	}
	if written, err = buffer.Write([]byte("z")); err != nil || written != 1 {
		t.Fatalf("unexpected full-buffer write result: written=%d err=%v", written, err)
	}
}

func newTestRunner(t *testing.T, binaryPath string, options Options) *Runner {
	t.Helper()
	runner, err := New(binaryPath, options)
	if err != nil {
		t.Fatalf("create runner: %v", err)
	}
	return runner
}

func testOptions() Options {
	return Options{Timeout: time.Second, MaxConcurrent: 1}
}

func writeScript(t *testing.T, body string) string {
	t.Helper()
	path := filepath.Join(t.TempDir(), "helper.sh")
	if err := os.WriteFile(path, []byte("#!/bin/sh\nset -eu\n"+body+"\n"), 0o700); err != nil {
		t.Fatalf("write helper script: %v", err)
	}
	return path
}

func waitForFile(t *testing.T, path string) {
	t.Helper()
	deadline := time.Now().Add(time.Second)
	for time.Now().Before(deadline) {
		if _, err := os.Stat(path); err == nil {
			return
		}
		time.Sleep(5 * time.Millisecond)
	}
	t.Fatalf("timed out waiting for %s", path)
}
