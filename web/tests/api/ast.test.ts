import { afterEach, describe, expect, it, vi } from "vitest";
import { AstApiError, buildAst } from "../../src/api/ast";

describe("buildAst", () => {
    afterEach(() => vi.unstubAllGlobals());

    it("returns DOT from a successful response", async () => {
        const fetchMock = vi
            .fn()
            .mockResolvedValue(response(200, { dot: "digraph AST {}" }));
        vi.stubGlobal("fetch", fetchMock);
        const controller = new AbortController();

        await expect(
            buildAst("int main() {}", controller.signal),
        ).resolves.toBe("digraph AST {}");
        expect(fetchMock).toHaveBeenCalledWith("/api/v1/ast", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ code: "int main() {}" }),
            signal: controller.signal,
        });
    });

    it("preserves a structured backend error", async () => {
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

        const error = await buildAst("broken").catch((cause: unknown) => cause);
        expect(error).toBeInstanceOf(AstApiError);
        expect(error).toMatchObject({
            status: 422,
            code: "invalid_source",
            message: "source code could not be parsed",
            diagnostics: "input.cpp:1:1: error",
        });
    });

    it.each([
        [200, { graph: "not-dot" }, "invalid AST response"],
        [500, { message: "not-structured" }, "invalid error response"],
        [200, null, "invalid AST response"],
    ])(
        "rejects malformed response %#",
        async (status, payload, expectedMessage) => {
            vi.stubGlobal(
                "fetch",
                vi.fn().mockResolvedValue(response(status, payload)),
            );

            await expect(buildAst("int main() {}")).rejects.toThrow(
                expectedMessage,
            );
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

        await expect(buildAst("int main() {}")).rejects.toMatchObject({
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
