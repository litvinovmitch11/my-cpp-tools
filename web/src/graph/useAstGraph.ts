import { useCallback, useEffect, useRef, useState } from "react";
import { AstApiError, buildAst } from "../api/ast";
import { useGraphviz } from "./useGraphviz";

type RequestStatus = "idle" | "building";

export type AstGraphStatus =
    "initializing" | "ready" | "building" | "rendering" | "worker-error";

export type AstGraphError = {
    message: string;
    diagnostics?: string;
};

export function useAstGraph() {
    const {
        status: graphvizStatus,
        result,
        error: graphvizError,
        isRendering,
        render,
        clear,
        restart,
    } = useGraphviz();
    const requestRef = useRef<AbortController | null>(null);
    const [requestStatus, setRequestStatus] = useState<RequestStatus>("idle");
    const [requestError, setRequestError] = useState<AstGraphError | null>(
        null,
    );

    useEffect(
        () => () => {
            requestRef.current?.abort();
        },
        [],
    );

    const build = useCallback(
        async (code: string) => {
            requestRef.current?.abort();
            const request = new AbortController();
            requestRef.current = request;
            setRequestStatus("building");
            setRequestError(null);
            clear();

            try {
                const dot = await buildAst(code, request.signal);
                if (requestRef.current !== request) return;
                setRequestStatus("idle");
                render(dot);
            } catch (cause) {
                if (request.signal.aborted) return;
                setRequestStatus("idle");
                setRequestError(toAstGraphError(cause));
            } finally {
                if (requestRef.current === request) {
                    requestRef.current = null;
                }
            }
        },
        [clear, render],
    );

    let status: AstGraphStatus = "ready";
    if (graphvizStatus === "loading") status = "initializing";
    else if (graphvizStatus === "error") status = "worker-error";
    else if (requestStatus === "building") status = "building";
    else if (isRendering) status = "rendering";

    return {
        status,
        result,
        error:
            requestError ?? (graphvizError ? { message: graphvizError } : null),
        build,
        restart,
    };
}

function toAstGraphError(cause: unknown): AstGraphError {
    if (cause instanceof AstApiError) {
        return {
            message: cause.message,
            diagnostics: cause.diagnostics,
        };
    }
    return {
        message:
            cause instanceof Error
                ? cause.message
                : "Could not connect to the backend",
    };
}
