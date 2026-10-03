import { useMemo } from "react";
import { sanitizeSvg } from "./svg-sanitize";
import type { LayoutResult } from "./types";

type Props = {
    result: LayoutResult | null;
};

export function GraphView({ result }: Props) {
    const cleanSvg = useMemo(
        () => (result ? sanitizeSvg(result.svg) : ""),
        [result],
    );

    if (!result) {
        return <div className="graph-empty">Нажмите «Показать пример AST»</div>;
    }

    return (
        <div
            className="graph-view"
            dangerouslySetInnerHTML={{ __html: cleanSvg }}
        />
    );
}
