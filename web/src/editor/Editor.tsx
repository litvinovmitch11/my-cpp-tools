import { lazy, Suspense } from "react";

const MonacoEditor = lazy(async () => {
    const [, editorModule] = await Promise.all([
        import("./monaco"),
        import("@monaco-editor/react"),
    ]);
    return { default: editorModule.default };
});

type Props = {
    value: string;
    onChange: (next: string) => void;
    fontSize: number;
};

export function Editor({ value, onChange, fontSize }: Props) {
    return (
        <Suspense
            fallback={<div className="editor-loading">Loading editor…</div>}
        >
            <MonacoEditor
                height="100%"
                defaultLanguage="cpp"
                theme="vs-dark"
                value={value}
                onChange={(next) => onChange(next ?? "")}
                options={{
                    automaticLayout: true,
                    fixedOverflowWidgets: true,
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
        </Suspense>
    );
}
