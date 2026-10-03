package runner

import (
	"path/filepath"
	"testing"
	"time"
)

func TestNewRejectsMissingBinary(t *testing.T) {
	missingPath := filepath.Join(t.TempDir(), "missing-AstPrinter")

	if _, err := New(missingPath, time.Second); err == nil {
		t.Fatal("expected an error for a missing AST builder")
	}
}
