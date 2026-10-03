import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

const astApiTarget = process.env.AST_API_TARGET ?? "http://127.0.0.1:8080";

export default defineConfig({
    plugins: [react()],
    worker: { format: "es" },
    server: {
        host: "127.0.0.1",
        port: 5173,
        strictPort: true,
        proxy: {
            "/api": astApiTarget,
        },
    },
    build: { outDir: "dist", sourcemap: true },
});
