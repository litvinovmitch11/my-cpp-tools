import MonacoEditor from "@monaco-editor/react";

type Props = {
    value: string;
    onChange: (next: string) => void;
};

export function Editor({ value, onChange }: Props) {
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
                fontSize: 13,
                lineHeight: 20,
                scrollBeyondLastLine: false,
                renderLineHighlight: "line",
                padding: { top: 8 },
                fontFamily: "JetBrains Mono, Consolas, Menlo, monospace",
                fontLigatures: true,
            }}
        />
    );
}
