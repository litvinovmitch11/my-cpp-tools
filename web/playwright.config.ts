import { defineConfig, devices } from "@playwright/test";

export default defineConfig({
    testDir: "./e2e",
    fullyParallel: false,
    workers: 1,
    reporter: [["list"], ["html", { open: "never" }]],
    use: {
        baseURL: "http://127.0.0.1:15173",
        trace: "retain-on-failure",
        screenshot: "only-on-failure",
        ...devices["Desktop Chrome"],
    },
    webServer: [
        {
            command:
                "AST_PRINTER_PATH=../cpp/build/src/tools/AstPrinter/AstPrinter HTTP_ADDRESS=127.0.0.1:18080 go run ./cmd/server",
            cwd: "../backend",
            url: "http://127.0.0.1:18080/healthz",
            reuseExistingServer: false,
            timeout: 120_000,
        },
        {
            command:
                "AST_API_TARGET=http://127.0.0.1:18080 npm run dev -- --port 15173",
            url: "http://127.0.0.1:15173",
            reuseExistingServer: false,
            timeout: 120_000,
        },
    ],
});
