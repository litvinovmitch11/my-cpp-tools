import { expect, test, type Page } from "@playwright/test";

test("builds and renders an AST for the editor source", async ({ page }) => {
    await page.goto("/");
    await setEditorCode(
        page,
        "int alpha_pipeline(int value) { return value + 42; }",
    );

    await expect(
        page.getByRole("button", { name: "Построить AST" }),
    ).toBeEnabled();
    await page.keyboard.press("Control+Enter");

    const graph = page.getByTestId("ast-graph");
    await expect(graph).toContainText("alpha_pipeline");
    const svg = graph.locator("svg");
    await expect(svg).toBeVisible();
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
    await page.getByRole("button", { name: "Увеличить граф" }).click();
    await expect(
        page.getByRole("status", { name: "Масштаб графа" }),
    ).toHaveText("125%");
    await expect
        .poll(async () => (await svg.boundingBox())?.width ?? 0)
        .toBeGreaterThan(fittedBounds?.width ?? 0);
    const pageWidthBeforeWheel = await page.evaluate(() => window.innerWidth);
    await page.getByTestId("graph-viewport").hover();
    await page.keyboard.down("Control");
    await page.mouse.wheel(0, -100);
    await page.keyboard.up("Control");
    await expect(
        page.getByRole("status", { name: "Масштаб графа" }),
    ).toHaveText("150%");
    expect(await page.evaluate(() => window.innerWidth)).toBe(
        pageWidthBeforeWheel,
    );
    await page.getByRole("button", { name: "Вписать" }).click();
    await expect(
        page.getByRole("status", { name: "Масштаб графа" }),
    ).toHaveText("100%");

    expect(
        await page.evaluate(
            () => document.documentElement.scrollWidth <= window.innerWidth,
        ),
    ).toBe(true);
});

test("shows compiler diagnostics for invalid C++", async ({ page }) => {
    await page.goto("/");
    await setEditorCode(page, "int broken(");

    const buildButton = page.getByRole("button", { name: "Построить AST" });
    await expect(buildButton).toBeEnabled();
    await buildButton.click();

    const error = page.getByTestId("ast-error");
    await expect(error).toContainText("source code could not be parsed");
    await expect(error).toContainText("input.cpp");
    await expect(buildButton).toBeEnabled();
});

test("supports editor font size and word navigation", async ({ page }) => {
    await page.goto("/");
    await setEditorCode(page, "// alpha beta");

    const editor = page.getByTestId("source-editor");
    await page.keyboard.press("Control+ArrowLeft");
    await page.keyboard.insertText("X");
    await expect(editor).toContainText("// alpha Xbeta");

    const editorText = editor.locator(".view-lines");
    await expect(editorText).toHaveCSS("font-size", "15px");
    await page.getByRole("button", { name: "Увеличить шрифт" }).click();
    await expect(editorText).toHaveCSS("font-size", "16px");
    await expect(
        page.getByRole("status", { name: "Размер шрифта" }),
    ).toHaveText("16px");
});

async function setEditorCode(page: Page, code: string) {
    const editor = page.getByTestId("source-editor").locator(".monaco-editor");
    await expect(editor).toBeVisible();
    await editor.click();
    await page.keyboard.press("Control+A");
    await page.keyboard.insertText(code);
}
