import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { getZoomDirection } from "../keyboard/zoom";
import { sanitizeSvg } from "./svg-sanitize";
import type { LayoutResult } from "./types";

const minZoom = 1;
const maxZoom = 8;
const zoomStep = 0.25;

type GraphRegion = {
    index: number;
    label: string;
};

type SvgBounds = {
    x: number;
    y: number;
    width: number;
    height: number;
};

type Props = {
    result: LayoutResult | null;
    emptyMessage: string;
};

export function GraphView({ result, emptyMessage }: Props) {
    const [zoom, setZoom] = useState(minZoom);
    const [regions, setRegions] = useState<GraphRegion[]>([]);
    const [focusedRegion, setFocusedRegion] = useState("all");
    const viewportRef = useRef<HTMLDivElement>(null);
    const canvasRef = useRef<HTMLDivElement>(null);
    const originalViewBoxRef = useRef("");
    const cleanSvg = useMemo(
        () => (result ? sanitizeSvg(result.svg) : ""),
        [result],
    );

    useEffect(() => {
        setZoom(minZoom);
        setFocusedRegion("all");

        const svg = canvasRef.current?.querySelector("svg");
        if (!svg) {
            originalViewBoxRef.current = "";
            setRegions([]);
            return;
        }

        originalViewBoxRef.current = svg.getAttribute("viewBox") ?? "";
        const nextRegions = Array.from(
            svg.querySelectorAll<SVGGElement>("g.cluster"),
        ).map((cluster, index) => ({
            index,
            label:
                cluster.querySelector("text")?.textContent?.trim() ||
                `Graph ${index + 1}`,
        }));
        setRegions(nextRegions);
    }, [cleanSvg, result?.renderId]);

    const changeZoom = useCallback((delta: number) => {
        setZoom((current) =>
            Math.min(maxZoom, Math.max(minZoom, current + delta)),
        );
    }, []);

    const focusRegion = useCallback((value: string) => {
        const svg = canvasRef.current?.querySelector("svg");
        if (!svg) return;

        if (value === "all") {
            const originalViewBox = originalViewBoxRef.current;
            if (originalViewBox) {
                svg.setAttribute("viewBox", originalViewBox);
            } else {
                svg.removeAttribute("viewBox");
            }
            setFocusedRegion("all");
            setZoom(minZoom);
            viewportRef.current?.scrollTo?.({ left: 0, top: 0 });
            return;
        }

        const index = Number(value);
        const cluster = svg.querySelectorAll<SVGGElement>("g.cluster")[index];
        if (!cluster) return;

        const bounds = boundsInRootCoordinates(svg, cluster);
        const padding = Math.max(
            12,
            Math.min(bounds.width, bounds.height) * 0.08,
        );
        svg.setAttribute(
            "viewBox",
            `${bounds.x - padding} ${bounds.y - padding} ${bounds.width + padding * 2} ${bounds.height + padding * 2}`,
        );
        setFocusedRegion(value);
        setZoom(minZoom);
        viewportRef.current?.scrollTo?.({ left: 0, top: 0 });
    }, []);

    const focusRegionAtPoint = useCallback(
        (clientX: number, clientY: number) => {
            const svg = canvasRef.current?.querySelector("svg");
            if (!svg) return;

            const matchingRegion = Array.from(
                svg.querySelectorAll<SVGGElement>("g.cluster"),
            )
                .map((cluster, index) => ({
                    index,
                    bounds: cluster.getBoundingClientRect(),
                }))
                .filter(
                    ({ bounds }) =>
                        clientX >= bounds.left &&
                        clientX <= bounds.right &&
                        clientY >= bounds.top &&
                        clientY <= bounds.bottom,
                )
                .sort(
                    (left, right) =>
                        left.bounds.width * left.bounds.height -
                        right.bounds.width * right.bounds.height,
                )[0];

            focusRegion(matchingRegion ? String(matchingRegion.index) : "all");
        },
        [focusRegion],
    );

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
        <div
            className="graph-view"
            data-testid="analysis-graph"
            onKeyDown={(event) => {
                if (
                    event.key === "Escape" &&
                    focusedRegion !== "all" &&
                    event.target instanceof Element &&
                    event.target.closest(".graph-viewport")
                ) {
                    event.preventDefault();
                    focusRegion("all");
                    return;
                }
                if (!event.ctrlKey && !event.metaKey) return;
                const direction = getZoomDirection(event);
                if (direction === 0) return;
                event.preventDefault();
                changeZoom(direction * zoomStep);
            }}
        >
            <div
                className="graph-controls"
                aria-label="Graph zoom"
                title="Ctrl/⌘ + mouse wheel or +/-"
            >
                {regions.length > 1 && (
                    <select
                        className="graph-focus-select"
                        aria-label="Focused function graph"
                        value={focusedRegion}
                        onChange={(event) => focusRegion(event.target.value)}
                    >
                        <option value="all">All graphs</option>
                        {regions.map((region) => (
                            <option key={region.index} value={region.index}>
                                {region.label}
                            </option>
                        ))}
                    </select>
                )}
                <button
                    type="button"
                    aria-label="Zoom out"
                    disabled={zoom <= minZoom}
                    onClick={() => changeZoom(-zoomStep)}
                >
                    −
                </button>
                <output aria-label="Graph zoom">
                    {Math.round(zoom * 100)}%
                </output>
                <button
                    type="button"
                    aria-label="Zoom in"
                    disabled={zoom >= maxZoom}
                    onClick={() => changeZoom(zoomStep)}
                >
                    +
                </button>
                <button
                    type="button"
                    className="graph-fit-button"
                    disabled={zoom === minZoom && focusedRegion === "all"}
                    onClick={() => focusRegion("all")}
                >
                    Fit
                </button>
            </div>
            <div
                ref={viewportRef}
                className="graph-viewport"
                data-testid="graph-viewport"
                tabIndex={0}
                aria-label="Graph viewport. Double-click a function graph to focus it. Press Escape to show all graphs."
                onPointerDown={(event) =>
                    event.currentTarget.focus({ preventScroll: true })
                }
                onDoubleClick={(event) =>
                    focusRegionAtPoint(event.clientX, event.clientY)
                }
            >
                <div
                    ref={canvasRef}
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

function boundsInRootCoordinates(
    svg: SVGSVGElement,
    element: SVGGraphicsElement,
): SvgBounds {
    const bounds = element.getBBox();
    const rootMatrix = svg.getCTM?.();
    const elementMatrix = element.getCTM?.();
    if (!rootMatrix || !elementMatrix) return bounds;

    const determinant =
        rootMatrix.a * rootMatrix.d - rootMatrix.b * rootMatrix.c;
    if (determinant === 0) return bounds;

    const transform = (x: number, y: number) => {
        const viewportX =
            elementMatrix.a * x + elementMatrix.c * y + elementMatrix.e;
        const viewportY =
            elementMatrix.b * x + elementMatrix.d * y + elementMatrix.f;
        const translatedX = viewportX - rootMatrix.e;
        const translatedY = viewportY - rootMatrix.f;
        return {
            x:
                (rootMatrix.d * translatedX - rootMatrix.c * translatedY) /
                determinant,
            y:
                (-rootMatrix.b * translatedX + rootMatrix.a * translatedY) /
                determinant,
        };
    };

    const corners = [
        transform(bounds.x, bounds.y),
        transform(bounds.x + bounds.width, bounds.y),
        transform(bounds.x, bounds.y + bounds.height),
        transform(bounds.x + bounds.width, bounds.y + bounds.height),
    ];
    const xCoordinates = corners.map((point) => point.x);
    const yCoordinates = corners.map((point) => point.y);
    const minimumX = Math.min(...xCoordinates);
    const maximumX = Math.max(...xCoordinates);
    const minimumY = Math.min(...yCoordinates);
    const maximumY = Math.max(...yCoordinates);
    return {
        x: minimumX,
        y: minimumY,
        width: maximumX - minimumX,
        height: maximumY - minimumY,
    };
}
