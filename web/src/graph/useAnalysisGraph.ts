import { useCallback, useEffect, useRef, useState } from "react";
import {
    AnalysisApiError,
    buildAnalysisGraph,
    type AnalysisKind,
} from "../api/analysis";
import { useGraphviz } from "./useGraphviz";

type RequestStatus = "idle" | "building";

export type AnalysisGraphStatus =
    "initializing" | "ready" | "building" | "rendering" | "worker-error";

export type AnalysisGraphError = {
    message: string;
    diagnostics?: string;
};

export function useAnalysisGraph() {
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
    const [kind, setKind] = useState<AnalysisKind>("ast");
    const [requestStatus, setRequestStatus] = useState<RequestStatus>("idle");
    const [requestError, setRequestError] = useState<AnalysisGraphError | null>(
        null,
    );

    useEffect(
        () => () => {
            requestRef.current?.abort();
        },
        [],
    );

    const selectKind = useCallback(
        (nextKind: AnalysisKind) => {
            if (nextKind === kind) return;
            requestRef.current?.abort();
            requestRef.current = null;
            setRequestStatus("idle");
            setRequestError(null);
            clear();
            setKind(nextKind);
        },
        [clear, kind],
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
                const dot = await buildAnalysisGraph(
                    kind,
                    code,
                    request.signal,
                );
                if (requestRef.current !== request) return;
                setRequestStatus("idle");
                render(dot);
            } catch (cause) {
                if (request.signal.aborted) return;
                setRequestStatus("idle");
                setRequestError(toAnalysisGraphError(cause));
            } finally {
                if (requestRef.current === request) {
                    requestRef.current = null;
                }
            }
        },
        [clear, kind, render],
    );

    let status: AnalysisGraphStatus = "ready";
    if (graphvizStatus === "loading") status = "initializing";
    else if (graphvizStatus === "error") status = "worker-error";
    else if (requestStatus === "building") status = "building";
    else if (isRendering) status = "rendering";

    return {
        kind,
        selectKind,
        status,
        result,
        error:
            requestError ?? (graphvizError ? { message: graphvizError } : null),
        build,
        restart,
    };
}

function toAnalysisGraphError(cause: unknown): AnalysisGraphError {
    if (cause instanceof AnalysisApiError) {
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
