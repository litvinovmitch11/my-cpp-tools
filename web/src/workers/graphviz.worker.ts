/// <reference lib="webworker" />
import { Graphviz } from "@hpcc-js/wasm-graphviz";

import type { WorkerRequest, WorkerResponse } from "./messages";

let graphviz: Awaited<ReturnType<typeof Graphviz.load>> | null = null;

const post = (msg: WorkerResponse) => self.postMessage(msg);

Graphviz.load()
    .then((g) => {
        graphviz = g;
        post({ kind: "ready" });
    })
    .catch((e) => {
        post({
            kind: "init-error",
            message: e instanceof Error ? e.message : String(e),
        });
    });

self.onmessage = (ev: MessageEvent<WorkerRequest>) => {
    const msg = ev.data;
    if (msg.kind !== "layout") return;

    if (!graphviz) {
        post({
            kind: "error",
            renderId: msg.renderId,
            message: "graphviz not loaded",
        });
        return;
    }

    try {
        const svg = graphviz.dot(msg.dot);
        post({ kind: "svg", renderId: msg.renderId, svg });
    } catch (e) {
        post({
            kind: "error",
            renderId: msg.renderId,
            message: e instanceof Error ? e.message : String(e),
        });
    }
};
