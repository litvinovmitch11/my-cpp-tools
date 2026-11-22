export type ZoomDirection = -1 | 0 | 1;

export function getZoomDirection(event: {
    key: string;
    code: string;
}): ZoomDirection {
    if (event.key === "+" || event.key === "=" || event.code === "NumpadAdd") {
        return 1;
    }
    if (
        event.key === "-" ||
        event.key === "_" ||
        event.code === "NumpadSubtract"
    ) {
        return -1;
    }
    return 0;
}
