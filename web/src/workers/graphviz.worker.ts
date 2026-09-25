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
        // Ошибка загрузки — приходит через ready-канал и без renderId.
        // Отправляем как error без renderId, main поймёт по отсутствию поля.
        console.error("graphviz load failed", e);
    });

self.onmessage = (ev: MessageEvent<WorkerRequest>) => {
    const msg = ev.data;
    if (msg.kind !== "layout") return;

    if (!graphviz) {
        post({ kind: "error", renderId: msg.renderId, message: "graphviz not loaded" });
        return;
    }

    try {
        const svg = graphviz.dot(msg.dot);
        post({ kind: "svg", renderId: msg.renderId, svg });
    } catch (e) {
        post({ kind: "error", renderId: msg.renderId, message: (e as Error).message });
    }
};
