import { useCallback, useEffect, useRef, useState } from "react";
import type { WorkerRequest, WorkerResponse } from "../workers/messages";

export type LayoutResult = { renderId: number; svg: string } | null;

export function useGraphviz() {
    const workerRef = useRef<Worker | null>(null);
    const [ready, setReady] = useState(false);
    const [result, setResult] = useState<LayoutResult>(null);
    const [error, setError] = useState<string | null>(null);
    const renderIdRef = useRef(0);

    useEffect(() => {
        const w = new Worker(
            new URL("../workers/graphviz.worker.ts", import.meta.url),
            { type: "module" }
        );
        workerRef.current = w;

        w.onmessage = (ev: MessageEvent<WorkerResponse>) => {
            const msg = ev.data;
            if (msg.kind === "ready") {
                setReady(true);
                return;
            }
            if (msg.kind === "svg") {
                setResult({ renderId: msg.renderId, svg: msg.svg });
                setError(null);
                return;
            }
            // error
            setError(msg.message);
        };

        return () => {
            w.terminate();
            workerRef.current = null;
        };
    }, []);

    const render = useCallback((dot: string) => {
        const w = workerRef.current;
        if (!w) return;
        renderIdRef.current += 1;
        const req: WorkerRequest = {
            kind: "layout",
            renderId: renderIdRef.current,
            dot,
        };
        w.postMessage(req);
    }, []);

    return { ready, result, error, render };
}
