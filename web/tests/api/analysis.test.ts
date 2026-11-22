import { afterEach, describe, expect, it, vi } from "vitest";
import { AnalysisApiError, buildAnalysisGraph } from "../../src/api/analysis";

describe("buildAnalysisGraph", () => {
    afterEach(() => vi.unstubAllGlobals());

    it.each([
        ["ast", "/api/v1/ast", "digraph AST {}"],
        ["cfg", "/api/v1/cfg", "digraph CFG {}"],
        ["cdg", "/api/v1/cdg", "digraph CDG {}"],
    ] as const)(
        "returns DOT from a successful %s response",
        async (kind, path, dot) => {
            const fetchMock = vi.fn().mockResolvedValue(response(200, { dot }));
            vi.stubGlobal("fetch", fetchMock);
            const controller = new AbortController();

            await expect(
                buildAnalysisGraph(kind, "int main() {}", controller.signal),
            ).resolves.toBe(dot);
            expect(fetchMock).toHaveBeenCalledWith(path, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ code: "int main() {}" }),
                signal: controller.signal,
            });
        },
    );

    it.each(["ast", "cfg", "cdg"] as const)(
        "preserves a structured %s backend error",
        async (kind) => {
            vi.stubGlobal(
                "fetch",
                vi.fn().mockResolvedValue(
                    response(422, {
                        error: {
                            code: "invalid_source",
                            message: "source code could not be parsed",
                            diagnostics: "input.cpp:1:1: error",
                        },
                    }),
                ),
            );

            const error = await buildAnalysisGraph(kind, "broken").catch(
                (cause: unknown) => cause,
            );
            expect(error).toBeInstanceOf(AnalysisApiError);
            expect(error).toMatchObject({
                status: 422,
                code: "invalid_source",
                message: "source code could not be parsed",
                diagnostics: "input.cpp:1:1: error",
            });
        },
    );

    it.each([
        [200, { graph: "not-dot" }, "invalid graph response"],
        [500, { message: "not-structured" }, "invalid error response"],
        [200, null, "invalid graph response"],
    ])(
        "rejects malformed response %#",
        async (status, payload, expectedMessage) => {
            vi.stubGlobal(
                "fetch",
                vi.fn().mockResolvedValue(response(status, payload)),
            );

            await expect(
                buildAnalysisGraph("cfg", "int main() {}"),
            ).rejects.toThrow(expectedMessage);
        },
    );

    it("handles a response with invalid JSON", async () => {
        vi.stubGlobal(
            "fetch",
            vi.fn().mockResolvedValue({
                ok: false,
                status: 502,
                json: vi
                    .fn()
                    .mockRejectedValue(new SyntaxError("invalid JSON")),
            }),
        );

        await expect(
            buildAnalysisGraph("ast", "int main() {}"),
        ).rejects.toMatchObject({
            code: "invalid_response",
            status: 502,
        });
    });
});

function response(status: number, payload: unknown) {
    return {
        ok: status >= 200 && status < 300,
        status,
        json: vi.fn().mockResolvedValue(payload),
    };
}
