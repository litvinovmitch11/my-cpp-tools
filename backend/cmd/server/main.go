package main

import (
	"log/slog"
	"os"

	"github.com/litvinovmitch11/my-cpp-tools/backend/internal/cmd"
)

func main() {
	if err := cmd.RunServer(); err != nil {
		slog.Error("server stopped", "error", err)
		os.Exit(1)
	}
}
