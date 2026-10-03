package ast

import (
	"context"
	"errors"
)

var (
	ErrTimeout        = errors.New("AST builder timed out")
	ErrOutputTooLarge = errors.New("AST graph is too large")
)

type Builder interface {
	BuildAST(ctx context.Context, source string) (string, error)
}

type BuildError struct {
	Diagnostics string
}

func (e *BuildError) Error() string {
	return "AST builder rejected the source code"
}
