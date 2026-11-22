import {
    act,
    fireEvent,
    render,
    screen,
    waitFor,
} from "@testing-library/react";
import { describe, expect, it } from "vitest";
import { GraphView } from "../../src/graph/GraphView";

const firstGraph = {
    renderId: 1,
    svg: '<svg viewBox="0 0 10 10"><text>main</text></svg>',
};

const multipleGraphs = {
    renderId: 2,
    svg: `<svg viewBox="0 0 100 100">
        <g class="cluster" data-bounds="0 0 40 40"><text>first()</text></g>
        <g class="cluster" data-bounds="0 0 40 40" data-translate="50 50"><text>second()</text></g>
    </svg>`,
};

describe("GraphView", () => {
    it("renders an empty state without graph controls", () => {
        render(<GraphView result={null} emptyMessage="Build a graph" />);

        expect(screen.getByText("Build a graph")).toBeTruthy();
        expect(screen.queryByRole("button", { name: "Zoom in" })).toBeNull();
    });

    it("zooms with controls and resets to fit", () => {
        render(<GraphView result={firstGraph} emptyMessage="Build a graph" />);
        const zoom = screen.getByRole("status", { name: "Graph zoom" });

        expect(zoom.textContent).toBe("100%");
        fireEvent.click(screen.getByRole("button", { name: "Zoom in" }));
        expect(zoom.textContent).toBe("125%");
        expect(
            (
                screen.getByRole("button", {
                    name: "Zoom out",
                }) as HTMLButtonElement
            ).disabled,
        ).toBe(false);

        fireEvent.click(screen.getByRole("button", { name: "Fit" }));
        expect(zoom.textContent).toBe("100%");
    });

    it("handles Ctrl+wheel without allowing browser zoom", () => {
        render(<GraphView result={firstGraph} emptyMessage="Build a graph" />);
        const viewport = screen.getByTestId("graph-viewport");
        const event = new WheelEvent("wheel", {
            bubbles: true,
            cancelable: true,
            ctrlKey: true,
            deltaY: -100,
        });

        act(() => viewport.dispatchEvent(event));

        expect(event.defaultPrevented).toBe(true);
        expect(
            screen.getByRole("status", { name: "Graph zoom" }).textContent,
        ).toBe("125%");
    });

    it("handles Ctrl plus and supports zoom up to 800 percent", () => {
        render(<GraphView result={firstGraph} emptyMessage="Build a graph" />);
        const viewport = screen.getByTestId("graph-viewport");
        viewport.focus();

        fireEvent.keyDown(viewport, { key: "+", ctrlKey: true });
        expect(
            screen.getByRole("status", { name: "Graph zoom" }).textContent,
        ).toBe("125%");

        for (let index = 0; index < 40; index += 1) {
            fireEvent.click(screen.getByRole("button", { name: "Zoom in" }));
        }
        expect(
            screen.getByRole("status", { name: "Graph zoom" }).textContent,
        ).toBe("800%");
        expect(
            (
                screen.getByRole("button", {
                    name: "Zoom in",
                }) as HTMLButtonElement
            ).disabled,
        ).toBe(true);
    });

    it("focuses one function graph and restores the complete view", () => {
        const originalGetBBox = Object.getOwnPropertyDescriptor(
            Element.prototype,
            "getBBox",
        );
        const originalGetCTM = Object.getOwnPropertyDescriptor(
            Element.prototype,
            "getCTM",
        );
        const originalGetBoundingClientRect = Object.getOwnPropertyDescriptor(
            Element.prototype,
            "getBoundingClientRect",
        );
        Object.defineProperty(Element.prototype, "getBBox", {
            configurable: true,
            value(this: Element) {
                const [x, y, width, height] = (
                    this.getAttribute("data-bounds") ?? "0 0 0 0"
                )
                    .split(" ")
                    .map(Number);
                return { x, y, width, height };
            },
        });
        Object.defineProperty(Element.prototype, "getCTM", {
            configurable: true,
            value(this: Element) {
                const [e, f] = (this.getAttribute("data-translate") ?? "0 0")
                    .split(" ")
                    .map(Number);
                return { a: 1, b: 0, c: 0, d: 1, e, f };
            },
        });
        Object.defineProperty(Element.prototype, "getBoundingClientRect", {
            configurable: true,
            value(this: Element) {
                const [left, top, width, height] = (
                    this.getAttribute("data-bounds") ?? "0 0 0 0"
                )
                    .split(" ")
                    .map(Number);
                const [translateX, translateY] = (
                    this.getAttribute("data-translate") ?? "0 0"
                )
                    .split(" ")
                    .map(Number);
                return {
                    x: left + translateX,
                    y: top + translateY,
                    left: left + translateX,
                    top: top + translateY,
                    right: left + translateX + width,
                    bottom: top + translateY + height,
                    width,
                    height,
                    toJSON: () => ({}),
                };
            },
        });

        try {
            const { container } = render(
                <GraphView
                    result={multipleGraphs}
                    emptyMessage="Build a graph"
                />,
            );
            const selector = screen.getByRole("combobox", {
                name: "Focused function graph",
            });
            expect(selector.textContent).toContain("first()");
            expect(selector.textContent).toContain("second()");

            const viewport = screen.getByTestId("graph-viewport");
            fireEvent.doubleClick(viewport, { clientX: 70, clientY: 70 });
            expect((selector as HTMLSelectElement).value).toBe("1");
            expect(
                container.querySelector("svg")?.getAttribute("viewBox"),
            ).toBe("38 38 64 64");
            fireEvent.keyDown(viewport, { key: "Escape" });
            expect((selector as HTMLSelectElement).value).toBe("all");
            expect(
                container.querySelector("svg")?.getAttribute("viewBox"),
            ).toBe("0 0 100 100");

            fireEvent.change(selector, { target: { value: "0" } });
            fireEvent.doubleClick(viewport, { clientX: 120, clientY: 120 });
            expect((selector as HTMLSelectElement).value).toBe("all");
        } finally {
            if (originalGetBBox) {
                Object.defineProperty(
                    Element.prototype,
                    "getBBox",
                    originalGetBBox,
                );
            } else {
                Reflect.deleteProperty(Element.prototype, "getBBox");
            }
            if (originalGetCTM) {
                Object.defineProperty(
                    Element.prototype,
                    "getCTM",
                    originalGetCTM,
                );
            } else {
                Reflect.deleteProperty(Element.prototype, "getCTM");
            }
            if (originalGetBoundingClientRect) {
                Object.defineProperty(
                    Element.prototype,
                    "getBoundingClientRect",
                    originalGetBoundingClientRect,
                );
            } else {
                Reflect.deleteProperty(
                    Element.prototype,
                    "getBoundingClientRect",
                );
            }
        }
    });

    it("ignores an unmodified wheel and resets for a new graph", async () => {
        const { rerender } = render(
            <GraphView result={firstGraph} emptyMessage="Build a graph" />,
        );
        const viewport = screen.getByTestId("graph-viewport");
        const event = new WheelEvent("wheel", {
            bubbles: true,
            cancelable: true,
            deltaY: -100,
        });
        viewport.dispatchEvent(event);
        expect(event.defaultPrevented).toBe(false);

        fireEvent.click(screen.getByRole("button", { name: "Zoom in" }));
        rerender(
            <GraphView
                result={{ ...firstGraph, renderId: 2 }}
                emptyMessage="Build a graph"
            />,
        );
        await waitFor(() =>
            expect(
                screen.getByRole("status", { name: "Graph zoom" }).textContent,
            ).toBe("100%"),
        );
    });
});
