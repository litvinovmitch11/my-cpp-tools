import { useState } from "react";
import { Editor } from "./editor/Editor";
import { GraphView } from "./graph/GraphView";
import { useGraphviz } from "./graph/useGraphviz";

const INITIAL_CODE = `int choose(int x) {
    int y = x + 1;
    if (y > 0) return y;
    return 0;
}
`;

// Пока бэкенда нет — используем статичный DOT.
// Когда подключим Go — заменим на response.cfg.dot.
const SAMPLE_DOT = `digraph G {
  rankdir=TB;
  bgcolor="transparent";
  node [shape=box, fontname="Consolas", fontsize=11,
        style="filled", fillcolor="#2d2d30",
        color="#5a5a5a", fontcolor="#d4d4d4"];
  edge [color="#858585", fontcolor="#858585", fontsize=10];

  entry [label="entry", fillcolor="#094771", color="#4fc1ff"];
  b1    [label="int y = x + 1"];
  b2    [label="y > 0?"];
  b3    [label="return y"];
  b4    [label="return 0"];
  exit  [label="exit", shape=ellipse, fillcolor="#094771", color="#4fc1ff"];

  entry -> b1;
  b1    -> b2;
  b2    -> b3 [label="true"];
  b2    -> b4 [label="false"];
  b3    -> exit;
  b4    -> exit;
}`;

export function App() {
    const [code, setCode] = useState(INITIAL_CODE);
    const { ready, result, error, render } = useGraphviz();

    const onRender = () => render(SAMPLE_DOT);

    return (
        <div className="app">
            <div className="toolbar">
                <span className="title">my-cpp-tools</span>
                <button
                    className="primary"
                    onClick={onRender}
                    disabled={!ready}
                >
                    {ready ? "Render DOT" : "Loading Graphviz…"}
                </button>
                {error && <span className="error">{error}</span>}
            </div>

            <div className="split">
                <div className="pane">
                    <div className="pane-header">Source</div>
                    <div className="pane-body">
                        <Editor value={code} onChange={setCode} />
                    </div>
                </div>

                <div className="pane">
                    <div className="pane-header">Graph</div>
                    <div className="pane-body">
                        <GraphView result={result} />
                    </div>
                </div>
            </div>
        </div>
    );
}
