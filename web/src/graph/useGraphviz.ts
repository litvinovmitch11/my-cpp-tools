import { useCallback, useEffect, useRef, useState } from "react";
import type { WorkerRequest, WorkerResponse } from "../workers/messages";
import type { LayoutResult } from "./types";

type WorkerStatus = "loading" | "ready" | "error";

export function useGraphviz() {
    const workerRef = useRef<Worker | null>(null);
    const workerReadyRef = useRef(false);
    const [status, setStatus] = useState<WorkerStatus>("loading");
    const [generation, setGeneration] = useState(0);
    const [result, setResult] = useState<LayoutResult | null>(null);
    const [error, setError] = useState<string | null>(null);
    const [isRendering, setIsRendering] = useState(false);
    const renderIdRef = useRef(0);

    useEffect(() => {
        let worker: Worker;
        try {
            worker = new Worker(
                new URL("../workers/graphviz.worker.ts", import.meta.url),
                { type: "module" },
            );
        } catch (cause) {
            setStatus("error");
            setError(cause instanceof Error ? cause.message : String(cause));
            return;
        }
        workerRef.current = worker;

        const fail = (message: string) => {
            if (workerRef.current !== worker) return;
            workerRef.current = null;
            workerReadyRef.current = false;
            worker.terminate();
            setStatus("error");
            setError(message);
            setIsRendering(false);
        };

        worker.onerror = (event) => {
            event.preventDefault();
            fail(event.message || "Graphviz Worker failed");
        };
        worker.onmessageerror = () =>
            fail("Could not read Graphviz Worker response");
        worker.onmessage = (event: MessageEvent<WorkerResponse>) => {
            if (workerRef.current !== worker) return;
            const message = event.data;
            if (message.kind === "init-error") {
                fail(message.message);
                return;
            }
            if (message.kind === "ready") {
                workerReadyRef.current = true;
                setStatus("ready");
                setError(null);
                return;
            }
            // A completed layout can already belong to an older request.
            if (message.renderId !== renderIdRef.current) return;
            if (message.kind === "svg") {
                setResult({ renderId: message.renderId, svg: message.svg });
                setError(null);
                setIsRendering(false);
            } else {
                setError(message.message);
                setIsRendering(false);
            }
        };

        return () => {
            worker.terminate();
            if (workerRef.current === worker) {
                workerRef.current = null;
                workerReadyRef.current = false;
            }
        };
    }, [generation]);

    const restart = useCallback(() => {
        workerRef.current?.terminate();
        workerRef.current = null;
        workerReadyRef.current = false;
        renderIdRef.current += 1;
        setStatus("loading");
        setError(null);
        setIsRendering(false);
        setGeneration((current) => current + 1);
    }, []);

    const render = useCallback((dot: string) => {
        const worker = workerRef.current;
        if (!worker || !workerReadyRef.current) return;
        renderIdRef.current += 1;
        setError(null);
        setResult(null);
        setIsRendering(true);
        const request: WorkerRequest = {
            kind: "layout",
            renderId: renderIdRef.current,
            dot,
        };
        worker.postMessage(request);
    }, []);

    const clear = useCallback(() => {
        renderIdRef.current += 1;
        setResult(null);
        setError(null);
        setIsRendering(false);
    }, []);

    return { status, result, error, isRendering, render, clear, restart };
}
