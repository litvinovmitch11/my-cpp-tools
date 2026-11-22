export const analysisKinds = ["ast", "cfg", "cdg"] as const;

export type AnalysisKind = (typeof analysisKinds)[number];

export type AnalysisApiErrorBody = {
    error: {
        code: string;
        message: string;
        diagnostics?: string;
    };
};

type GraphResponse = {
    dot: string;
};

const analysisPaths: Record<AnalysisKind, string> = {
    ast: "/api/v1/ast",
    cfg: "/api/v1/cfg",
    cdg: "/api/v1/cdg",
};

export class AnalysisApiError extends Error {
    readonly status: number;
    readonly code: string;
    readonly diagnostics?: string;

    constructor(
        status: number,
        code: string,
        message: string,
        diagnostics?: string,
    ) {
        super(message);
        this.name = "AnalysisApiError";
        this.status = status;
        this.code = code;
        this.diagnostics = diagnostics;
    }
}

export async function buildAnalysisGraph(
    kind: AnalysisKind,
    code: string,
    signal?: AbortSignal,
): Promise<string> {
    const response = await fetch(analysisPaths[kind], {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code }),
        signal,
    });

    const payload: unknown = await response.json().catch(() => null);
    if (!response.ok) {
        if (isErrorBody(payload)) {
            throw new AnalysisApiError(
                response.status,
                payload.error.code,
                payload.error.message,
                payload.error.diagnostics,
            );
        }
        throw new AnalysisApiError(
            response.status,
            "invalid_response",
            "Backend returned an invalid error response",
        );
    }

    if (!isGraphResponse(payload)) {
        throw new AnalysisApiError(
            response.status,
            "invalid_response",
            "Backend returned an invalid graph response",
        );
    }
    return payload.dot;
}

function isGraphResponse(value: unknown): value is GraphResponse {
    return (
        typeof value === "object" &&
        value !== null &&
        "dot" in value &&
        typeof value.dot === "string"
    );
}

function isErrorBody(value: unknown): value is AnalysisApiErrorBody {
    if (
        typeof value !== "object" ||
        value === null ||
        !("error" in value) ||
        typeof value.error !== "object" ||
        value.error === null
    ) {
        return false;
    }

    const error = value.error;
    return (
        "code" in error &&
        typeof error.code === "string" &&
        "message" in error &&
        typeof error.message === "string" &&
        (!("diagnostics" in error) || typeof error.diagnostics === "string")
    );
}
