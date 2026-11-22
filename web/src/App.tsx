import { useCallback, useEffect, useState, type CSSProperties } from "react";
import type { AnalysisKind } from "./api/analysis";
import { Editor } from "./editor/Editor";
import { GraphView } from "./graph/GraphView";
import {
    useAnalysisGraph,
    type AnalysisGraphStatus,
} from "./graph/useAnalysisGraph";
import { getZoomDirection } from "./keyboard/zoom";

import { INITIAL_CODE } from "./examples/demo";

const minEditorFontSize = 11;
const maxEditorFontSize = 22;
const minDiagnosticsFontSize = 11;
const maxDiagnosticsFontSize = 24;

export function App() {
    const [code, setCode] = useState(INITIAL_CODE);
    const [editorFontSize, setEditorFontSize] = useState(15);
    const [diagnosticsFontSize, setDiagnosticsFontSize] = useState(13);
    const { kind, selectKind, status, result, error, build, restart } =
        useAnalysisGraph();

    const buttonLabel = getButtonLabel(status, kind);
    const kindLabel = getKindLabel(kind);

    const changeEditorFontSize = useCallback((delta: number) => {
        setEditorFontSize((current) =>
            clamp(current + delta, minEditorFontSize, maxEditorFontSize),
        );
    }, []);
    const changeDiagnosticsFontSize = useCallback((delta: number) => {
        setDiagnosticsFontSize((current) =>
            clamp(
                current + delta,
                minDiagnosticsFontSize,
                maxDiagnosticsFontSize,
            ),
        );
    }, []);

    useEffect(() => {
        const onKeyDown = (event: KeyboardEvent) => {
            if (event.ctrlKey || event.metaKey) {
                const direction = getZoomDirection(event);
                const activeElement = document.activeElement;
                if (
                    direction !== 0 &&
                    activeElement?.closest('[data-font-target="editor"]')
                ) {
                    event.preventDefault();
                    changeEditorFontSize(direction);
                    return;
                }
                if (
                    direction !== 0 &&
                    activeElement?.closest('[data-font-target="diagnostics"]')
                ) {
                    event.preventDefault();
                    changeDiagnosticsFontSize(direction);
                    return;
                }
            }
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
    }, [build, changeDiagnosticsFontSize, changeEditorFontSize, code, status]);

    return (
        <div className="app">
            <div className="toolbar">
                <span className="title">my-cpp-tools</span>
                <fieldset
                    className="analysis-selector"
                    disabled={status !== "ready"}
                >
                    <legend>Analysis</legend>
                    <AnalysisOption
                        kind="ast"
                        selectedKind={kind}
                        onSelect={selectKind}
                    />
                    <AnalysisOption
                        kind="cfg"
                        selectedKind={kind}
                        onSelect={selectKind}
                    />
                </fieldset>
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
                <div className="pane" data-font-target="editor">
                    <div className="pane-header pane-header-with-controls">
                        <span>Source</span>
                        <FontSizeControls
                            label="Editor font size"
                            outputLabel="Font size"
                            value={editorFontSize}
                            minimum={minEditorFontSize}
                            maximum={maxEditorFontSize}
                            onChange={changeEditorFontSize}
                        />
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

                <div
                    className="pane"
                    data-font-target={error ? "diagnostics" : undefined}
                >
                    <div className="pane-header pane-header-with-controls">
                        <span>{kindLabel} graph</span>
                        {error && (
                            <FontSizeControls
                                label="Diagnostics font size"
                                outputLabel="Diagnostics font size"
                                value={diagnosticsFontSize}
                                minimum={minDiagnosticsFontSize}
                                maximum={maxDiagnosticsFontSize}
                                onChange={changeDiagnosticsFontSize}
                            />
                        )}
                    </div>
                    <div className="pane-body graph-pane">
                        {error ? (
                            <div
                                className="error-panel"
                                role="alert"
                                tabIndex={0}
                                data-testid="analysis-error"
                                style={
                                    {
                                        "--diagnostics-font-size": `${diagnosticsFontSize}px`,
                                    } as CSSProperties
                                }
                                onPointerDown={(event) =>
                                    event.currentTarget.focus({
                                        preventScroll: true,
                                    })
                                }
                            >
                                <strong>{error.message}</strong>
                                {error.diagnostics && (
                                    <pre>{error.diagnostics}</pre>
                                )}
                            </div>
                        ) : (
                            <GraphView
                                result={result}
                                emptyMessage={getEmptyMessage(status, kind)}
                            />
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
}

type FontSizeControlsProps = {
    label: string;
    outputLabel: string;
    value: number;
    minimum: number;
    maximum: number;
    onChange: (delta: number) => void;
};

function FontSizeControls({
    label,
    outputLabel,
    value,
    minimum,
    maximum,
    onChange,
}: FontSizeControlsProps) {
    return (
        <div className="font-size-controls" aria-label={label}>
            <button
                type="button"
                aria-label={`Decrease ${label.toLowerCase()}`}
                disabled={value <= minimum}
                onClick={() => onChange(-1)}
            >
                A−
            </button>
            <output aria-label={outputLabel}>{value}px</output>
            <button
                type="button"
                aria-label={`Increase ${label.toLowerCase()}`}
                disabled={value >= maximum}
                onClick={() => onChange(1)}
            >
                A+
            </button>
        </div>
    );
}

type AnalysisOptionProps = {
    kind: AnalysisKind;
    selectedKind: AnalysisKind;
    onSelect: (kind: AnalysisKind) => void;
};

function AnalysisOption({ kind, selectedKind, onSelect }: AnalysisOptionProps) {
    const label = getKindLabel(kind);
    return (
        <label className="analysis-option">
            <input
                type="radio"
                name="analysis-kind"
                value={kind}
                checked={selectedKind === kind}
                onChange={() => onSelect(kind)}
            />
            <span>{label}</span>
        </label>
    );
}

function getKindLabel(kind: AnalysisKind) {
    return kind.toUpperCase();
}

function getButtonLabel(status: AnalysisGraphStatus, kind: AnalysisKind) {
    const label = getKindLabel(kind);
    switch (status) {
        case "initializing":
            return "Loading Graphviz…";
        case "building":
            return `Building ${label}…`;
        case "rendering":
            return "Rendering graph…";
        default:
            return `Build ${label}`;
    }
}

function getEmptyMessage(status: AnalysisGraphStatus, kind: AnalysisKind) {
    const label = getKindLabel(kind);
    if (status === "building") return `The backend is building the ${label}…`;
    if (status === "rendering") return "Graphviz is rendering the graph…";
    return `Enter C++ code and select Build ${label}`;
}

function clamp(value: number, minimum: number, maximum: number) {
    return Math.min(maximum, Math.max(minimum, value));
}
