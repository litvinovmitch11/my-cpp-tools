import { useState } from "react";
import { Editor } from "./editor/Editor";

const INITIAL_CODE = `int choose(int x) {
    int y = x + 1;
    if (y > 0) return y;
    return 0;
}
`;

export function App() {
    const [code, setCode] = useState(INITIAL_CODE);

    return (
        <div className="app">
            <div className="toolbar">
                <span className="title">my-cpp-tools</span>
                <span className="counter">{code.length} символов</span>
            </div>

            <div className="editor-host">
                <Editor value={code} onChange={setCode} />
            </div>
        </div>
    );
}
