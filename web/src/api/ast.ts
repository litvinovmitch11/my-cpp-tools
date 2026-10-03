export type AstApiErrorBody = {
    error: {
        code: string;
        message: string;
        diagnostics?: string;
    };
};

type AstResponse = {
    dot: string;
};

export class AstApiError extends Error {
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
        this.name = "AstApiError";
        this.status = status;
        this.code = code;
        this.diagnostics = diagnostics;
    }
}

export async function buildAst(
    code: string,
    signal?: AbortSignal,
): Promise<string> {
    const response = await fetch("/api/v1/ast", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code }),
        signal,
    });

    const payload: unknown = await response.json().catch(() => null);
    if (!response.ok) {
        if (isErrorBody(payload)) {
            throw new AstApiError(
                response.status,
                payload.error.code,
                payload.error.message,
                payload.error.diagnostics,
            );
        }
        throw new AstApiError(
            response.status,
            "invalid_response",
            "Backend returned an invalid error response",
        );
    }

    if (!isAstResponse(payload)) {
        throw new AstApiError(
            response.status,
            "invalid_response",
            "Backend returned an invalid AST response",
        );
    }
    return payload.dot;
}

function isAstResponse(value: unknown): value is AstResponse {
    return (
        typeof value === "object" &&
        value !== null &&
        "dot" in value &&
        typeof value.dot === "string"
    );
}

function isErrorBody(value: unknown): value is AstApiErrorBody {
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
