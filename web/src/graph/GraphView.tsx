import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { sanitizeSvg } from "./svg-sanitize";
import type { LayoutResult } from "./types";

const minZoom = 1;
const maxZoom = 4;
const zoomStep = 0.25;

type Props = {
    result: LayoutResult | null;
    emptyMessage: string;
};

export function GraphView({ result, emptyMessage }: Props) {
    const [zoom, setZoom] = useState(minZoom);
    const viewportRef = useRef<HTMLDivElement>(null);
    const cleanSvg = useMemo(
        () => (result ? sanitizeSvg(result.svg) : ""),
        [result],
    );

    useEffect(() => setZoom(minZoom), [result?.renderId]);

    const changeZoom = useCallback((delta: number) => {
        setZoom((current) =>
            Math.min(maxZoom, Math.max(minZoom, current + delta)),
        );
    }, []);

    useEffect(() => {
        const viewport = viewportRef.current;
        if (!viewport) return;

        const onWheel = (event: WheelEvent) => {
            if (!event.ctrlKey && !event.metaKey) return;
            event.preventDefault();
            changeZoom(event.deltaY < 0 ? zoomStep : -zoomStep);
        };
        viewport.addEventListener("wheel", onWheel, { passive: false });
        return () => viewport.removeEventListener("wheel", onWheel);
    }, [changeZoom, result?.renderId]);

    if (!result) {
        return <div className="graph-empty">{emptyMessage}</div>;
    }

    return (
        <div className="graph-view" data-testid="ast-graph">
            <div
                className="graph-controls"
                aria-label="Масштаб графа"
                title="Ctrl/⌘ + колесо"
            >
                <button
                    type="button"
                    aria-label="Уменьшить граф"
                    disabled={zoom <= minZoom}
                    onClick={() => changeZoom(-zoomStep)}
                >
                    −
                </button>
                <output aria-label="Масштаб графа">
                    {Math.round(zoom * 100)}%
                </output>
                <button
                    type="button"
                    aria-label="Увеличить граф"
                    disabled={zoom >= maxZoom}
                    onClick={() => changeZoom(zoomStep)}
                >
                    +
                </button>
                <button
                    type="button"
                    className="graph-fit-button"
                    disabled={zoom === minZoom}
                    onClick={() => setZoom(minZoom)}
                >
                    Вписать
                </button>
            </div>
            <div
                ref={viewportRef}
                className="graph-viewport"
                data-testid="graph-viewport"
            >
                <div
                    className="graph-canvas"
                    style={{
                        width: `${zoom * 100}%`,
                        height: `${zoom * 100}%`,
                    }}
                    dangerouslySetInnerHTML={{ __html: cleanSvg }}
                />
            </div>
        </div>
    );
}
