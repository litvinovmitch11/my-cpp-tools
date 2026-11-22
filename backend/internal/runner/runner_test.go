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

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/analysis"
)

func TestNewValidatesOptionsAndBinaries(t *testing.T) {
	script := writeScript(t, "printf 'digraph {}'")
	missing := filepath.Join(t.TempDir(), "missing")
	tests := []struct {
		name    string
		paths   BinaryPaths
		options Options
	}{
		{name: "missing AST binary", paths: BinaryPaths{AST: missing, CFG: script, CDG: script}, options: testOptions()},
		{name: "missing CFG binary", paths: BinaryPaths{AST: script, CFG: missing, CDG: script}, options: testOptions()},
		{name: "missing CDG binary", paths: BinaryPaths{AST: script, CFG: script, CDG: missing}, options: testOptions()},
		{name: "zero timeout", paths: BinaryPaths{AST: script, CFG: script, CDG: script}, options: Options{MaxConcurrent: 1}},
		{name: "zero concurrency", paths: BinaryPaths{AST: script, CFG: script, CDG: script}, options: Options{Timeout: time.Second}},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			if _, err := New(test.paths, test.options); err == nil {
				t.Fatal("expected validation error")
			}
		})
	}
}

func TestBuildGraphSelectsBinaryForKind(t *testing.T) {
	astScript := writeScript(t, "printf 'digraph AST {}'")
	cfgScript := writeScript(t, "printf 'digraph CFG {}'")
	cdgScript := writeScript(t, "printf 'digraph CDG {}'")
	runner, err := New(BinaryPaths{AST: astScript, CFG: cfgScript, CDG: cdgScript}, testOptions())
	if err != nil {
		t.Fatalf("create runner: %v", err)
	}

	tests := []struct {
		kind analysis.Kind
		want string
	}{
		{kind: analysis.AST, want: "digraph AST {}"},
		{kind: analysis.CFG, want: "digraph CFG {}"},
		{kind: analysis.CDG, want: "digraph CDG {}"},
	}
	for _, test := range tests {
		t.Run(string(test.kind), func(t *testing.T) {
			graph, err := runner.BuildGraph(context.Background(), test.kind, "int main() {}")
			if err != nil {
				t.Fatalf("build %s: %v", test.kind, err)
			}
			if graph != test.want {
				t.Fatalf("unexpected graph: %q", graph)
			}
		})
	}
}

func TestBuildGraphUsesIsolatedEnvironment(t *testing.T) {
	t.Setenv("RUNNER_TEST_SECRET", "must-not-leak")
	statPath, err := exec.LookPath("stat")
	if err != nil {
		t.Skip("stat is not available")
	}
	script := writeScript(t, fmt.Sprintf(`
[ "$PWD" = "$(dirname "$1")" ] || exit 10
[ "$HOME" = "$PWD" ] || exit 11
[ "$TMPDIR" = "$PWD" ] || exit 12
[ -z "${RUNNER_TEST_SECRET:-}" ] || exit 13
[ "$2" = "--" ] || exit 14
[ "$3" = "-std=c++20" ] || exit 15
[ "$(%[1]q -c %%a "$PWD")" = "700" ] || exit 16
[ "$(%[1]q -c %%a "$1")" = "600" ] || exit 17
printf 'digraph AST {}'
`, statPath))
	runner := newTestRunner(t, script, script, testOptions())

	graph, err := runner.BuildGraph(context.Background(), analysis.AST, "int main() {}")
	if err != nil {
		t.Fatalf("build AST: %v", err)
	}
	if graph != "digraph AST {}" {
		t.Fatalf("unexpected graph: %q", graph)
	}
}

func TestBuildGraphReturnsSanitizedSourceDiagnostics(t *testing.T) {
	script := writeScript(t, `
printf '%s:1:1: error: rejected\n' "$1" >&2
printf 'working directory: %s\n' "$PWD" >&2
exit 2
`)
	runner := newTestRunner(t, script, script, testOptions())

	_, err := runner.BuildGraph(context.Background(), analysis.CFG, "broken")
	var sourceError *analysis.SourceError
	if !errors.As(err, &sourceError) {
		t.Fatalf("expected source error, got %v", err)
	}
	if sourceError.Diagnostics != "input.cpp:1:1: error: rejected\nworking directory: <temporary-directory>" {
		t.Fatalf("unexpected diagnostics: %q", sourceError.Diagnostics)
	}
	if strings.Contains(sourceError.Diagnostics, os.TempDir()) {
		t.Fatalf("temporary path leaked: %q", sourceError.Diagnostics)
	}
}

func TestBuildGraphTruncatesSourceDiagnostics(t *testing.T) {
	headPath, err := exec.LookPath("head")
	if err != nil {
		t.Skip("head is not available")
	}
	script := writeScript(t, fmt.Sprintf(`
%q -c %d /dev/zero >&2
exit 2
`, headPath, maxDiagnosticsBytes+1))
	runner := newTestRunner(t, script, script, testOptions())

	_, err = runner.BuildGraph(context.Background(), analysis.AST, "broken")
	var sourceError *analysis.SourceError
	if !errors.As(err, &sourceError) {
		t.Fatalf("expected source error, got %v", err)
	}
	if !strings.HasSuffix(sourceError.Diagnostics, "[diagnostics truncated]") {
		t.Fatalf("expected truncation marker, got diagnostics length %d", len(sourceError.Diagnostics))
	}
}

func TestBuildGraphKeepsInternalDiagnosticsPrivate(t *testing.T) {
	script := writeScript(t, `
printf 'secret internal detail\n' >&2
exit 1
`)
	runner := newTestRunner(t, script, script, testOptions())

	_, err := runner.BuildGraph(context.Background(), analysis.CFG, "int main() {}")
	var sourceError *analysis.SourceError
	if err == nil || errors.As(err, &sourceError) {
		t.Fatalf("expected internal tool error, got %v", err)
	}
	if strings.Contains(err.Error(), "secret internal detail") {
		t.Fatalf("internal diagnostics leaked: %v", err)
	}
}

func TestBuildGraphTreatsSignalTerminationAsInternalFailure(t *testing.T) {
	script := writeScript(t, `kill -TERM $$`)
	runner := newTestRunner(t, script, script, testOptions())

	_, err := runner.BuildGraph(context.Background(), analysis.AST, "int main() {}")
	var sourceError *analysis.SourceError
	if err == nil || errors.As(err, &sourceError) {
		t.Fatalf("expected internal signal failure, got %v", err)
	}
}

func TestBuildGraphReportsBinaryRemovedAfterStartupAsInternalFailure(t *testing.T) {
	astScript := writeScript(t, `printf 'digraph AST {}'`)
	cfgScript := writeScript(t, `printf 'digraph CFG {}'`)
	runner := newTestRunner(t, astScript, cfgScript, testOptions())
	if err := os.Remove(astScript); err != nil {
		t.Fatalf("remove AST helper: %v", err)
	}

	_, err := runner.BuildGraph(context.Background(), analysis.AST, "int main() {}")
	var sourceError *analysis.SourceError
	if err == nil || errors.As(err, &sourceError) {
		t.Fatalf("expected internal start failure, got %v", err)
	}
}

func TestBuildGraphDoesNotMaskInternalFailureWithLargeOutput(t *testing.T) {
	headPath, err := exec.LookPath("head")
	if err != nil {
		t.Skip("head is not available")
	}
	script := writeScript(t, fmt.Sprintf(`
%q -c %d /dev/zero
exit 1
`, headPath, maxGraphBytes+1))
	runner := newTestRunner(t, script, script, testOptions())

	_, err = runner.BuildGraph(context.Background(), analysis.CFG, "int main() {}")
	if err == nil || errors.Is(err, analysis.ErrOutputTooLarge) {
		t.Fatalf("expected internal tool error, got %v", err)
	}
}

func TestBuildGraphRejectsLargeGraph(t *testing.T) {
	headPath, err := exec.LookPath("head")
	if err != nil {
		t.Skip("head is not available")
	}
	script := writeScript(t, fmt.Sprintf(`%q -c %d /dev/zero`, headPath, maxGraphBytes+1))
	runner := newTestRunner(t, script, script, testOptions())

	_, err = runner.BuildGraph(context.Background(), analysis.CFG, "int main() {}")
	if !errors.Is(err, analysis.ErrOutputTooLarge) {
		t.Fatalf("expected output-too-large error, got %v", err)
	}
}

func TestBuildGraphRejectsEmptySuccessfulOutput(t *testing.T) {
	script := writeScript(t, `exit 0`)
	runner := newTestRunner(t, script, script, testOptions())

	graph, err := runner.BuildGraph(context.Background(), analysis.AST, "int main() {}")
	if err == nil || graph != "" {
		t.Fatalf("expected empty output protocol error, got graph=%q err=%v", graph, err)
	}
}

func TestBuildGraphTimesOutAndReleasesCapacity(t *testing.T) {
	script := writeScript(t, `/bin/sleep 1`)
	runner := newTestRunner(t, script, script, Options{Timeout: 20 * time.Millisecond, MaxConcurrent: 1})

	for attempt := 0; attempt < 2; attempt++ {
		_, err := runner.BuildGraph(context.Background(), analysis.AST, "int main() {}")
		if !errors.Is(err, analysis.ErrTimeout) {
			t.Fatalf("attempt %d: expected timeout, got %v", attempt, err)
		}
	}
}

func TestBuildGraphHonorsCancellation(t *testing.T) {
	marker := filepath.Join(t.TempDir(), "started")
	astScript := writeScript(t, fmt.Sprintf(`
: > %q
/bin/sleep 1
printf 'must not complete'
`, marker))
	cfgScript := writeScript(t, `printf 'digraph CFG {}'`)
	runner := newTestRunner(t, astScript, cfgScript, Options{Timeout: time.Second, MaxConcurrent: 1})
	ctx, cancel := context.WithCancel(context.Background())
	finished := make(chan error, 1)
	go func() {
		_, err := runner.BuildGraph(ctx, analysis.AST, "int main() {}")
		finished <- err
	}()
	waitForFile(t, marker)
	cancel()

	err := <-finished
	if !errors.Is(err, context.Canceled) {
		t.Fatalf("expected cancellation, got %v", err)
	}
	graph, err := runner.BuildGraph(context.Background(), analysis.CFG, "int main() {}")
	if err != nil || graph != "digraph CFG {}" {
		t.Fatalf("capacity was not released after cancellation: graph=%q err=%v", graph, err)
	}
}

func TestBuildGraphSharesCapacityAcrossKinds(t *testing.T) {
	marker := filepath.Join(t.TempDir(), "started")
	astScript := writeScript(t, fmt.Sprintf(`
: > %q
/bin/sleep 0.1
printf 'digraph AST {}'
`, marker))
	cfgScript := writeScript(t, "printf 'digraph CFG {}'")
	runner := newTestRunner(t, astScript, cfgScript, Options{Timeout: time.Second, MaxConcurrent: 1})
	finished := make(chan error, 1)
	go func() {
		_, err := runner.BuildGraph(context.Background(), analysis.AST, "first")
		finished <- err
	}()
	waitForFile(t, marker)

	if _, err := runner.BuildGraph(context.Background(), analysis.CFG, "second"); !errors.Is(err, analysis.ErrBusy) {
		t.Fatalf("expected shared busy error, got %v", err)
	}
	if err := <-finished; err != nil {
		t.Fatalf("first build failed: %v", err)
	}
}

func TestBuildGraphRejectsUnknownKind(t *testing.T) {
	script := writeScript(t, "printf 'must not run'")
	runner := newTestRunner(t, script, script, testOptions())

	if _, err := runner.BuildGraph(context.Background(), analysis.Kind("pdg"), "source"); err == nil {
		t.Fatal("expected unsupported-kind error")
	}
}

func TestBuildGraphRemovesTemporaryDirectory(t *testing.T) {
	script := writeScript(t, `printf '%s' "$1"`)
	runner := newTestRunner(t, script, script, testOptions())

	sourcePath, err := runner.BuildGraph(context.Background(), analysis.AST, "int main() {}")
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

func newTestRunner(t *testing.T, astPath, cfgPath string, options Options) *Runner {
	t.Helper()
	runner, err := New(BinaryPaths{AST: astPath, CFG: cfgPath, CDG: cfgPath}, options)
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
