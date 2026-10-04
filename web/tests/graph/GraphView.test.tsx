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
