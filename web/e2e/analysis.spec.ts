import { expect, test, type Page } from "@playwright/test";
import { resolve } from "node:path";

test("builds and renders an AST by default", async ({ page }) => {
    await page.goto("/");
    await setEditorCode(
        page,
        "int alpha_pipeline(int value) { return value + 42; }",
    );

    await expect(page.getByRole("radio", { name: "AST" })).toBeChecked();
    await expect(page.getByRole("button", { name: "Build AST" })).toBeEnabled();
    await page.keyboard.press("Control+Enter");

    const graph = page.getByTestId("analysis-graph");
    await expect(graph).toContainText("alpha_pipeline");
    const svg = graph.locator("svg");
    await expect(svg).toBeVisible();
    if (process.env.CAPTURE_README_SCREENSHOT === "true") {
        await page.screenshot({
            path: resolve("../docs/assets/ast-viewer.png"),
            fullPage: true,
        });
    }
    expect(
        await svg.evaluate((element) =>
            window
                .getComputedStyle(element)
                .getPropertyValue("background-color"),
        ),
    ).toBe("rgba(0, 0, 0, 0)");
    expect(
        await graph.evaluate((container) => {
            const renderedGraph = container.querySelector("svg");
            if (!renderedGraph) return false;

            const containerBounds = container.getBoundingClientRect();
            const graphBounds = renderedGraph.getBoundingClientRect();
            return (
                graphBounds.width <= containerBounds.width &&
                graphBounds.height <= containerBounds.height
            );
        }),
    ).toBe(true);

    const fittedBounds = await svg.boundingBox();
    expect(fittedBounds).not.toBeNull();
    await page.getByRole("button", { name: "Zoom in" }).click();
    await expect(page.getByRole("status", { name: "Graph zoom" })).toHaveText(
        "125%",
    );
    await expect
        .poll(async () => (await svg.boundingBox())?.width ?? 0)
        .toBeGreaterThan(fittedBounds?.width ?? 0);
    const pageWidthBeforeWheel = await page.evaluate(() => window.innerWidth);
    await page.getByTestId("graph-viewport").hover();
    await page.keyboard.down("Control");
    await page.mouse.wheel(0, -100);
    await page.keyboard.up("Control");
    await expect(page.getByRole("status", { name: "Graph zoom" })).toHaveText(
        "150%",
    );
    expect(await page.evaluate(() => window.innerWidth)).toBe(
        pageWidthBeforeWheel,
    );
    await page.getByRole("button", { name: "Fit" }).click();
    await expect(page.getByRole("status", { name: "Graph zoom" })).toHaveText(
        "100%",
    );

    expect(
        await page.evaluate(
            () => document.documentElement.scrollWidth <= window.innerWidth,
        ),
    ).toBe(true);
});

test("switches from AST to CFG without retaining the old graph", async ({
    page,
}) => {
    await page.goto("/");
    await setEditorCode(
        page,
        "int choose(int value) { if (value > 0) return 1; return 0; }",
    );

    await page.getByRole("button", { name: "Build AST" }).click();
    const graph = page.getByTestId("analysis-graph");
    await expect(graph).toContainText("IfStmt");

    await page.getByRole("radio", { name: "CFG" }).check();
    await expect(page.getByText("CFG graph")).toBeVisible();
    await expect(
        page.getByText("Enter C++ code and select Build CFG"),
    ).toBeVisible();
    await expect(graph).toHaveCount(0);

    await page.getByRole("button", { name: "Build CFG" }).click();
    const cfgGraph = page.getByTestId("analysis-graph");
    await expect(cfgGraph).toContainText("choose");
    await expect(cfgGraph).toContainText("Entry");
    await expect(cfgGraph).toContainText("Exit");
    await expect(cfgGraph).toContainText("true");
    await expect(cfgGraph).toContainText("false");
    await expect(cfgGraph).not.toContainText("IfStmt");
});

for (const kind of ["AST", "CFG"] as const) {
    test(`shows compiler diagnostics for invalid ${kind} source`, async ({
        page,
    }) => {
        await page.goto("/");
        if (kind === "CFG") {
            await page.getByRole("radio", { name: kind }).check();
        }
        await setEditorCode(page, "int broken(");

        const buildButton = page.getByRole("button", {
            name: `Build ${kind}`,
        });
        await expect(buildButton).toBeEnabled();
        await buildButton.click();

        const error = page.getByTestId("analysis-error");
        await expect(error).toContainText("source code could not be parsed");
        await expect(error).toContainText("input.cpp");
        await expect(buildButton).toBeEnabled();
    });
}

test("supports editor font size and word navigation", async ({ page }) => {
    await page.goto("/");
    await setEditorCode(page, "// alpha beta");

    const editor = page.getByTestId("source-editor");
    await page.keyboard.press("Control+ArrowLeft");
    await page.keyboard.insertText("X");
    await expect(editor).toContainText("// alpha Xbeta");

    const editorText = editor.locator(".view-lines");
    await expect(editorText).toHaveCSS("font-size", "15px");
    await page.getByRole("button", { name: "Increase font size" }).click();
    await expect(editorText).toHaveCSS("font-size", "16px");
    await expect(page.getByRole("status", { name: "Font size" })).toHaveText(
        "16px",
    );
});

async function setEditorCode(page: Page, code: string) {
    const editor = page.getByTestId("source-editor").locator(".monaco-editor");
    await expect(editor).toBeVisible();
    await editor.click();
    await page.keyboard.press("Control+A");
    await page.keyboard.insertText(code);
}
