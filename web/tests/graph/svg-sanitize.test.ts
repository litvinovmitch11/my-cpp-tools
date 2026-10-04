import { describe, expect, it } from "vitest";
import { sanitizeSvg } from "../../src/graph/svg-sanitize";

describe("sanitizeSvg", () => {
    it("keeps safe graph markup", () => {
        const svg = sanitizeSvg(
            '<svg viewBox="0 0 10 10"><text>FunctionDecl main</text></svg>',
        );

        expect(svg).toContain("<svg");
        expect(svg).toContain("FunctionDecl main");
        expect(svg).toContain("viewBox");
    });

    it("removes scripts, event handlers, links, and foreign objects", () => {
        const svg = sanitizeSvg(`
            <svg xmlns="http://www.w3.org/2000/svg">
                <script>alert(1)</script>
                <a href="javascript:alert(1)"><text>unsafe</text></a>
                <rect onclick="alert(1)" />
                <foreignObject><div>unsafe HTML</div></foreignObject>
            </svg>
        `);

        expect(svg).not.toMatch(/script|foreignObject|onclick|href=/i);
        expect(svg).toContain("unsafe");
    });
});
