import { act, renderHook, waitFor } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { AstApiError } from "../../src/api/ast";
import { useAstGraph } from "../../src/graph/useAstGraph";

const mocks = vi.hoisted(() => ({
    buildAst: vi.fn(),
    clear: vi.fn(),
    render: vi.fn(),
    restart: vi.fn(),
    graphviz: {
        status: "ready" as "loading" | "ready" | "error",
        result: null as { renderId: number; svg: string } | null,
        error: null as string | null,
        isRendering: false,
    },
}));

vi.mock("../../src/api/ast", async (importOriginal) => ({
    ...(await importOriginal<typeof import("../../src/api/ast")>()),
    buildAst: mocks.buildAst,
}));

vi.mock("../../src/graph/useGraphviz", () => ({
    useGraphviz: () => ({
        ...mocks.graphviz,
        clear: mocks.clear,
        render: mocks.render,
        restart: mocks.restart,
    }),
}));

describe("useAstGraph", () => {
    beforeEach(() => {
        mocks.graphviz.status = "ready";
        mocks.graphviz.result = null;
        mocks.graphviz.error = null;
        mocks.graphviz.isRendering = false;
    });

    it("builds and renders an AST graph", async () => {
        const request = deferred<string>();
        mocks.buildAst.mockReturnValueOnce(request.promise);
        const { result } = renderHook(() => useAstGraph());

        act(() => void result.current.build("int main() {}"));
        expect(result.current.status).toBe("building");
        expect(mocks.clear).toHaveBeenCalledOnce();

        request.resolve("digraph AST {}");
        await waitFor(() => expect(result.current.status).toBe("ready"));
        expect(mocks.render).toHaveBeenCalledWith("digraph AST {}");
    });

    it("exposes structured backend diagnostics", async () => {
        mocks.buildAst.mockRejectedValueOnce(
            new AstApiError(422, "invalid_source", "Invalid C++", "line 1"),
        );
        const { result } = renderHook(() => useAstGraph());

        await act(() => result.current.build("broken"));

        expect(result.current.error).toEqual({
            message: "Invalid C++",
            diagnostics: "line 1",
        });
    });

    it("ignores a stale response after a newer request starts", async () => {
        const first = deferred<string>();
        const second = deferred<string>();
        mocks.buildAst
            .mockReturnValueOnce(first.promise)
            .mockReturnValueOnce(second.promise);
        const { result } = renderHook(() => useAstGraph());

        act(() => void result.current.build("first"));
        act(() => void result.current.build("second"));
        first.resolve("digraph stale {}");
        await act(async () => first.promise);
        expect(mocks.render).not.toHaveBeenCalled();

        second.resolve("digraph current {}");
        await waitFor(() =>
            expect(mocks.render).toHaveBeenCalledWith("digraph current {}"),
        );
    });

    it("maps Graphviz lifecycle states and errors", () => {
        mocks.graphviz.status = "loading";
        const { result, rerender } = renderHook(() => useAstGraph());
        expect(result.current.status).toBe("initializing");

        mocks.graphviz.status = "ready";
        mocks.graphviz.isRendering = true;
        rerender();
        expect(result.current.status).toBe("rendering");

        mocks.graphviz.isRendering = false;
        mocks.graphviz.status = "error";
        mocks.graphviz.error = "worker stopped";
        rerender();
        expect(result.current.status).toBe("worker-error");
        expect(result.current.error).toEqual({ message: "worker stopped" });

        act(() => result.current.restart());
        expect(mocks.restart).toHaveBeenCalledOnce();
    });

    it("turns unknown failures into a useful message", async () => {
        mocks.buildAst.mockRejectedValueOnce("offline");
        const { result } = renderHook(() => useAstGraph());

        await act(() => result.current.build("code"));

        expect(result.current.error).toEqual({
            message: "Could not connect to the backend",
        });
    });
});

function deferred<T>() {
    let resolve!: (value: T) => void;
    let reject!: (reason?: unknown) => void;
    const promise = new Promise<T>((resolvePromise, rejectPromise) => {
        resolve = resolvePromise;
        reject = rejectPromise;
    });
    return { promise, resolve, reject };
}
