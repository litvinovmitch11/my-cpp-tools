import { defineConfig } from "vitest/config";

export default defineConfig({
    test: {
        environment: "jsdom",
        include: ["tests/**/*.test.{ts,tsx}"],
        setupFiles: ["./tests/setup.ts"],
        restoreMocks: true,
        coverage: {
            provider: "v8",
            reporter: ["text", "json-summary", "lcov"],
            reportsDirectory: "coverage",
            include: [
                "src/App.tsx",
                "src/api/**/*.ts",
                "src/graph/GraphView.tsx",
                "src/graph/svg-sanitize.ts",
                "src/graph/useAstGraph.ts",
            ],
            thresholds: {
                lines: 70,
                functions: 70,
                branches: 60,
                statements: 70,
            },
        },
    },
});
