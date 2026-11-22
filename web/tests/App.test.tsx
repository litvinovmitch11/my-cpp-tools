import { fireEvent, render, screen } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { App } from "../src/App";

const mocks = vi.hoisted(() => ({
    build: vi.fn(),
    restart: vi.fn(),
    selectKind: vi.fn(),
    state: {
        kind: "ast" as "ast" | "cfg" | "cdg",
        status: "ready" as
            | "initializing"
            | "ready"
            | "building"
            | "rendering"
            | "worker-error",
        result: null as { renderId: number; svg: string } | null,
        error: null as { message: string; diagnostics?: string } | null,
    },
}));

vi.mock("../src/editor/Editor", () => ({
    Editor: ({
        value,
        onChange,
        fontSize,
    }: {
        value: string;
        onChange: (value: string) => void;
        fontSize: number;
    }) => (
        <textarea
            aria-label="C++ source"
            value={value}
            data-font-size={fontSize}
            onChange={(event) => onChange(event.target.value)}
        />
    ),
}));

vi.mock("../src/graph/GraphView", () => ({
    GraphView: ({ emptyMessage }: { emptyMessage: string }) => (
        <div>{emptyMessage}</div>
    ),
}));

vi.mock("../src/graph/useAnalysisGraph", () => ({
    useAnalysisGraph: () => ({
        ...mocks.state,
        build: mocks.build,
        restart: mocks.restart,
        selectKind: mocks.selectKind,
    }),
}));

describe("App", () => {
    beforeEach(() => {
        vi.clearAllMocks();
        mocks.state.kind = "ast";
        mocks.state.status = "ready";
        mocks.state.result = null;
        mocks.state.error = null;
    });

    it("selects AST by default and lets the user choose CFG or CDG", () => {
        render(<App />);

        expect(
            (screen.getByRole("radio", { name: "AST" }) as HTMLInputElement)
                .checked,
        ).toBe(true);
        fireEvent.click(screen.getByRole("radio", { name: "CFG" }));
        expect(mocks.selectKind).toHaveBeenCalledWith("cfg");
        fireEvent.click(screen.getByRole("radio", { name: "CDG" }));
        expect(mocks.selectKind).toHaveBeenLastCalledWith("cdg");
        expect(mocks.build).not.toHaveBeenCalled();
    });

    it("uses CFG labels for the selected analysis", () => {
        mocks.state.kind = "cfg";
        render(<App />);

        expect(screen.getByRole("button", { name: "Build CFG" })).toBeTruthy();
        expect(screen.getByText("CFG graph")).toBeTruthy();
        expect(
            screen.getByText("Enter C++ code and select Build CFG"),
        ).toBeTruthy();
    });

    it("uses CDG labels for the selected analysis", () => {
        mocks.state.kind = "cdg";
        render(<App />);

        expect(screen.getByRole("button", { name: "Build CDG" })).toBeTruthy();
        expect(screen.getByText("CDG graph")).toBeTruthy();
        expect(
            screen.getByText("Enter C++ code and select Build CDG"),
        ).toBeTruthy();
    });

    it("builds the edited source from the button and keyboard shortcut", () => {
        render(<App />);
        const editor = screen.getByRole("textbox", { name: "C++ source" });
        fireEvent.change(editor, { target: { value: "int answer = 42;" } });

        fireEvent.click(screen.getByRole("button", { name: "Build AST" }));
        expect(mocks.build).toHaveBeenLastCalledWith("int answer = 42;");

        fireEvent.keyDown(window, { key: "Enter", ctrlKey: true });
        expect(mocks.build).toHaveBeenCalledTimes(2);
    });

    it("changes the editor font size within the toolbar", () => {
        render(<App />);
        const editor = screen.getByRole("textbox", { name: "C++ source" });
        expect(editor.getAttribute("data-font-size")).toBe("15");

        fireEvent.click(
            screen.getByRole("button", {
                name: "Increase editor font size",
            }),
        );
        expect(editor.getAttribute("data-font-size")).toBe("16");
        fireEvent.click(
            screen.getByRole("button", {
                name: "Decrease editor font size",
            }),
        );
        expect(editor.getAttribute("data-font-size")).toBe("15");
    });

    it("uses Ctrl plus and minus for the active editor", () => {
        render(<App />);
        const editor = screen.getByRole("textbox", { name: "C++ source" });
        editor.focus();

        fireEvent.keyDown(editor, { key: "+", ctrlKey: true });
        expect(editor.getAttribute("data-font-size")).toBe("16");
        fireEvent.keyDown(editor, { key: "-", ctrlKey: true });
        expect(editor.getAttribute("data-font-size")).toBe("15");
    });

    it.each([
        ["initializing", "Loading Graphviz…"],
        ["building", "Building AST…"],
        ["rendering", "Rendering graph…"],
    ] as const)("disables build while status is %s", (status, label) => {
        mocks.state.status = status;
        render(<App />);

        const button = screen.getByRole("button", { name: label });
        expect((button as HTMLButtonElement).disabled).toBe(true);
        expect(
            (
                screen.getByRole("group", {
                    name: "Analysis",
                }) as HTMLFieldSetElement
            ).disabled,
        ).toBe(true);
        fireEvent.keyDown(window, { key: "Enter", metaKey: true });
        expect(mocks.build).not.toHaveBeenCalled();
    });

    it("shows diagnostics and lets the user restart Graphviz", () => {
        mocks.state.status = "worker-error";
        mocks.state.error = {
            message: "Graphviz failed",
            diagnostics: "worker diagnostics",
        };
        render(<App />);

        expect(screen.getByRole("alert").textContent).toContain(
            "worker diagnostics",
        );
        fireEvent.click(
            screen.getByRole("button", { name: "Retry initialization" }),
        );
        expect(mocks.restart).toHaveBeenCalledOnce();
    });

    it("resizes diagnostics with controls and Ctrl plus", () => {
        mocks.state.error = {
            message: "Analysis failed",
            diagnostics: "long diagnostics",
        };
        render(<App />);

        const diagnostics = screen.getByRole("alert");
        expect(diagnostics.getAttribute("style")).toContain("13px");
        fireEvent.click(
            screen.getByRole("button", {
                name: "Increase diagnostics font size",
            }),
        );
        expect(diagnostics.getAttribute("style")).toContain("14px");

        diagnostics.focus();
        fireEvent.keyDown(diagnostics, { key: "+", ctrlKey: true });
        expect(diagnostics.getAttribute("style")).toContain("15px");
    });
});
