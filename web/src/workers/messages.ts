export type WorkerRequest = {
    kind: "layout";
    renderId: number;
    dot: string;
};

export type WorkerResponse =
    | { kind: "ready" }
    | { kind: "init-error"; message: string }
    | { kind: "svg"; renderId: number; svg: string }
    | { kind: "error"; renderId: number; message: string };
