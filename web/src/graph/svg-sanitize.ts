import DOMPurify from "dompurify";

export function sanitizeSvg(svg: string): string {
    return DOMPurify.sanitize(svg, {
        USE_PROFILES: { svg: true, svgFilters: true },
        FORBID_TAGS: ["script", "foreignObject", "a"],
        FORBID_ATTR: ["xlink:href", "href"],
    });
}
