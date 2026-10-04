import { useEffect, useState } from "react";
import { Editor } from "./editor/Editor";
import { GraphView } from "./graph/GraphView";
import { useAstGraph } from "./graph/useAstGraph";

import { INITIAL_CODE } from "./examples/demo";

const minEditorFontSize = 11;
const maxEditorFontSize = 22;

export function App() {
    const [code, setCode] = useState(INITIAL_CODE);
    const [editorFontSize, setEditorFontSize] = useState(15);
    const { status, result, error, build, restart } = useAstGraph();

    const buttonLabel = getButtonLabel(status);

    useEffect(() => {
        const onKeyDown = (event: KeyboardEvent) => {
            if (
                event.key === "Enter" &&
                (event.ctrlKey || event.metaKey) &&
                status === "ready"
            ) {
                event.preventDefault();
                void build(code);
            }
        };

        window.addEventListener("keydown", onKeyDown, { capture: true });
        return () =>
            window.removeEventListener("keydown", onKeyDown, {
                capture: true,
            });
    }, [build, code, status]);

    return (
        <div className="app">
            <div className="toolbar">
                <span className="title">my-cpp-tools</span>
                <button
                    className="primary"
                    onClick={() => void build(code)}
                    disabled={status !== "ready"}
                    aria-keyshortcuts="Control+Enter Meta+Enter"
                    title="Ctrl/⌘ + Enter"
                >
                    {buttonLabel}
                </button>
                <span className="shortcut" aria-hidden="true">
                    Ctrl/⌘ + Enter
                </span>
                {status === "worker-error" && (
                    <button onClick={restart}>Retry initialization</button>
                )}
            </div>

            <div className="split">
                <div className="pane">
                    <div className="pane-header pane-header-with-controls">
                        <span>Source</span>
                        <div
                            className="font-size-controls"
                            aria-label="Editor font size"
                        >
                            <button
                                type="button"
                                aria-label="Decrease font size"
                                disabled={editorFontSize <= minEditorFontSize}
                                onClick={() =>
                                    setEditorFontSize((current) =>
                                        Math.max(
                                            minEditorFontSize,
                                            current - 1,
                                        ),
                                    )
                                }
                            >
                                A−
                            </button>
                            <output aria-label="Font size">
                                {editorFontSize}px
                            </output>
                            <button
                                type="button"
                                aria-label="Increase font size"
                                disabled={editorFontSize >= maxEditorFontSize}
                                onClick={() =>
                                    setEditorFontSize((current) =>
                                        Math.min(
                                            maxEditorFontSize,
                                            current + 1,
                                        ),
                                    )
                                }
                            >
                                A+
                            </button>
                        </div>
                    </div>
                    <div
                        className="pane-body editor-host"
                        data-testid="source-editor"
                    >
                        <Editor
                            value={code}
                            onChange={setCode}
                            fontSize={editorFontSize}
                        />
                    </div>
                </div>

                <div className="pane">
                    <div className="pane-header">AST graph</div>
                    <div className="pane-body graph-pane">
                        {error ? (
                            <div
                                className="error-panel"
                                role="alert"
                                data-testid="ast-error"
                            >
                                <strong>{error.message}</strong>
                                {error.diagnostics && (
                                    <pre>{error.diagnostics}</pre>
                                )}
                            </div>
                        ) : (
                            <GraphView
                                result={result}
                                emptyMessage={getEmptyMessage(status)}
                            />
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
}

function getButtonLabel(status: ReturnType<typeof useAstGraph>["status"]) {
    switch (status) {
        case "initializing":
            return "Loading Graphviz…";
        case "building":
            return "Building AST…";
        case "rendering":
            return "Rendering graph…";
        default:
            return "Build AST";
    }
}

function getEmptyMessage(status: ReturnType<typeof useAstGraph>["status"]) {
    if (status === "building") return "The backend is building the AST…";
    if (status === "rendering") return "Graphviz is rendering the graph…";
    return "Enter C++ code and select Build AST";
}
