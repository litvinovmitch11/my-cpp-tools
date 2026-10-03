import { useState } from "react";
import { Editor } from "./editor/Editor";
import { GraphView } from "./graph/GraphView";
import { useGraphviz } from "./graph/useGraphviz";

import { INITIAL_CODE, SAMPLE_DOT } from "./examples/demo";

export function App() {
    const [code, setCode] = useState(INITIAL_CODE);
    const { status, result, error, render, restart } = useGraphviz();

    const onRender = () => render(SAMPLE_DOT);

    return (
        <div className="app">
            <div className="toolbar">
                <span className="title">my-cpp-tools</span>
                <button
                    className="primary"
                    onClick={onRender}
                    disabled={status !== "ready"}
                >
                    {status === "loading"
                        ? "Загрузка Graphviz…"
                        : "Показать пример AST"}
                </button>
                {status === "error" && (
                    <button onClick={restart}>Повторить загрузку</button>
                )}
                {error && (
                    <span className="error" role="alert">
                        {error}
                    </span>
                )}
            </div>

            <div className="split">
                <div className="pane">
                    <div className="pane-header">Source</div>
                    <div className="pane-body">
                        <Editor value={code} onChange={setCode} />
                    </div>
                </div>

                <div className="pane">
                    <div className="pane-header">
                        Пример AST — не зависит от текста редактора
                    </div>
                    <div className="pane-body">
                        <GraphView result={result} />
                    </div>
                </div>
            </div>
        </div>
    );
}
