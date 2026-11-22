package analysis

import (
	"context"
	"errors"
	"fmt"
)

type Kind string

const (
	AST                   Kind = "ast"
	CFG                   Kind = "cfg"
	CDG                   Kind = "cdg"
	InvalidSourceExitCode      = 2
)

var (
	ErrTimeout        = errors.New("analysis timed out")
	ErrOutputTooLarge = errors.New("analysis graph is too large")
	ErrBusy           = errors.New("analysis runner is at capacity")
)

type Builder interface {
	BuildGraph(ctx context.Context, kind Kind, source string) (string, error)
}

type SourceError struct {
	Diagnostics string
}

func (e *SourceError) Error() string {
	return "analysis tool rejected the source code"
}

func (k Kind) Validate() error {
	switch k {
	case AST, CFG, CDG:
		return nil
	default:
		return fmt.Errorf("unsupported analysis kind %q", k)
	}
}
