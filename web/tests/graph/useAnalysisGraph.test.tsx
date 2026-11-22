import { act, renderHook, waitFor } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { AnalysisApiError } from "../../src/api/analysis";
import { useAnalysisGraph } from "../../src/graph/useAnalysisGraph";

const mocks = vi.hoisted(() => ({
    buildAnalysisGraph: vi.fn(),
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

vi.mock("../../src/api/analysis", async (importOriginal) => ({
    ...(await importOriginal<typeof import("../../src/api/analysis")>()),
    buildAnalysisGraph: mocks.buildAnalysisGraph,
}));

vi.mock("../../src/graph/useGraphviz", () => ({
    useGraphviz: () => ({
        ...mocks.graphviz,
        clear: mocks.clear,
        render: mocks.render,
        restart: mocks.restart,
    }),
}));

describe("useAnalysisGraph", () => {
    beforeEach(() => {
        vi.clearAllMocks();
        mocks.graphviz.status = "ready";
        mocks.graphviz.result = null;
        mocks.graphviz.error = null;
        mocks.graphviz.isRendering = false;
    });

    it.each([
        ["ast", "digraph AST {}"],
        ["cfg", "digraph CFG {}"],
        ["cdg", "digraph CDG {}"],
    ] as const)("builds and renders a %s graph", async (kind, dot) => {
        const request = deferred<string>();
        mocks.buildAnalysisGraph.mockReturnValueOnce(request.promise);
        const { result } = renderHook(() => useAnalysisGraph());

        if (kind !== "ast") {
            act(() => result.current.selectKind(kind));
        }
        act(() => void result.current.build("int main() {}"));
        expect(result.current.status).toBe("building");
        expect(mocks.buildAnalysisGraph).toHaveBeenCalledWith(
            kind,
            "int main() {}",
            expect.any(AbortSignal),
        );

        request.resolve(dot);
        await waitFor(() => expect(result.current.status).toBe("ready"));
        expect(mocks.render).toHaveBeenCalledWith(dot);
    });

    it("exposes structured backend diagnostics", async () => {
        mocks.buildAnalysisGraph.mockRejectedValueOnce(
            new AnalysisApiError(
                422,
                "invalid_source",
                "Invalid C++",
                "line 1",
            ),
        );
        const { result } = renderHook(() => useAnalysisGraph());

        act(() => result.current.selectKind("cfg"));
        await act(() => result.current.build("broken"));

        expect(result.current.error).toEqual({
            message: "Invalid C++",
            diagnostics: "line 1",
        });
    });

    it("clears state and ignores an AST response after selecting CFG", async () => {
        const astRequest = deferred<string>();
        mocks.buildAnalysisGraph.mockReturnValueOnce(astRequest.promise);
        const { result } = renderHook(() => useAnalysisGraph());

        act(() => void result.current.build("first"));
        const signal = mocks.buildAnalysisGraph.mock.calls[0]?.[2] as
            AbortSignal | undefined;
        act(() => result.current.selectKind("cfg"));

        expect(result.current.kind).toBe("cfg");
        expect(result.current.status).toBe("ready");
        expect(signal?.aborted).toBe(true);
        expect(mocks.clear).toHaveBeenCalledTimes(2);

        astRequest.resolve("digraph stale {}");
        await act(async () => astRequest.promise);
        expect(mocks.render).not.toHaveBeenCalled();
    });

    it("ignores a stale response after a newer request starts", async () => {
        const first = deferred<string>();
        const second = deferred<string>();
        mocks.buildAnalysisGraph
            .mockReturnValueOnce(first.promise)
            .mockReturnValueOnce(second.promise);
        const { result } = renderHook(() => useAnalysisGraph());

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
        const { result, rerender } = renderHook(() => useAnalysisGraph());
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
        mocks.buildAnalysisGraph.mockRejectedValueOnce("offline");
        const { result } = renderHook(() => useAnalysisGraph());

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
