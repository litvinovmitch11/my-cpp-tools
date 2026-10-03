import "./monaco";
import MonacoEditor from "@monaco-editor/react";

type Props = {
    value: string;
    onChange: (next: string) => void;
    fontSize: number;
};

export function Editor({ value, onChange, fontSize }: Props) {
    return (
        <MonacoEditor
            height="100%"
            defaultLanguage="cpp"
            theme="vs-dark"
            value={value}
            onChange={(next) => onChange(next ?? "")}
            options={{
                automaticLayout: true,
                minimap: { enabled: false },
                fontSize,
                lineHeight: Math.round(fontSize * 1.5),
                scrollBeyondLastLine: false,
                renderLineHighlight: "line",
                padding: { top: 8 },
                fontFamily: "JetBrains Mono, Consolas, Menlo, monospace",
                fontLigatures: true,
            }}
        />
    );
}
