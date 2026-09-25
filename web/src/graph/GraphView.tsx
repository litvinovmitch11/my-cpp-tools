import { useMemo } from "react";
import { sanitizeSvg } from "./svg-sanitize";
import type { LayoutResult } from "./useGraphviz";

type Props = {
    result: LayoutResult;
};

export function GraphView({ result }: Props) {
    const cleanSvg = useMemo(
        () => (result ? sanitizeSvg(result.svg) : ""),
        [result]
    );

    if (!result) {
        return <div className="graph-empty">нажмите «Render DOT»</div>;
    }

    return (
        <div
            className="graph-view"
            // eslint-disable-next-line react/no-danger
            dangerouslySetInnerHTML={{ __html: cleanSvg }}
        />
    );
}
