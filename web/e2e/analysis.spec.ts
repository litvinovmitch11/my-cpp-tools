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
    await page
        .getByRole("button", { name: "Increase editor font size" })
        .click();
    await expect(editorText).toHaveCSS("font-size", "16px");
    await expect(page.getByRole("status", { name: "Font size" })).toHaveText(
        "16px",
    );
});

test("loads Monaco icons and zooms the active pane with the keyboard", async ({
    page,
}) => {
    await page.setViewportSize({ width: 720, height: 600 });
    await page.goto("/");
    const editor = page.getByTestId("source-editor");
    await editor.locator(".monaco-editor").click();
    await page.keyboard.press("Control+f");
    const findWidget = editor.locator(".find-widget");
    await expect(findWidget).toBeVisible();
    await findWidget.locator(".button.toggle").click();
    expect(
        await editor.evaluate((container) => {
            const widget = container.querySelector<HTMLElement>(".find-widget");
            if (!widget) return false;
            const containerBounds = container.getBoundingClientRect();
            const widgetBounds = widget.getBoundingClientRect();
            return (
                widgetBounds.left >= containerBounds.left &&
                widgetBounds.right <= containerBounds.right
            );
        }),
    ).toBe(true);
    const regexButton = findWidget.locator(".codicon-regex");
    await regexButton.hover();
    const hover = page.locator(".workbench-hover-container").last();
    await expect(hover).toBeVisible();
    await expect(hover).toContainText("Use Regular Expression");
    const hoverContents = hover.locator(".hover-contents");
    await expect(hoverContents).toBeVisible();
    const hoverVisibility = await hoverContents.evaluate((element) => {
        const bounds = element.getBoundingClientRect();
        const hoverContainer = element.closest<HTMLElement>(
            ".workbench-hover-container",
        );
        const hitTestElements = hoverContainer
            ? [
                  hoverContainer,
                  ...hoverContainer.querySelectorAll<HTMLElement>("*"),
              ]
            : [];
        const previousPointerEvents = hitTestElements.map((hitTestElement) => ({
            element: hitTestElement,
            value: hitTestElement.style.getPropertyValue("pointer-events"),
            priority:
                hitTestElement.style.getPropertyPriority("pointer-events"),
        }));
        for (const hitTestElement of hitTestElements) {
            hitTestElement.style.setProperty(
                "pointer-events",
                "auto",
                "important",
            );
        }
        const topmostElement = document.elementFromPoint(
            bounds.left + bounds.width / 2,
            bounds.top + bounds.height / 2,
        );
        for (const previous of previousPointerEvents) {
            if (previous.value) {
                previous.element.style.setProperty(
                    "pointer-events",
                    previous.value,
                    previous.priority,
                );
            } else {
                previous.element.style.removeProperty("pointer-events");
            }
        }
        return {
            inViewport:
                bounds.top >= 0 &&
                bounds.left >= 0 &&
                bounds.right <= window.innerWidth &&
                bounds.bottom <= window.innerHeight,
            topmostWithinHover:
                topmostElement !== null &&
                (topmostElement === element ||
                    element.contains(topmostElement)),
        };
    });
    expect(hoverVisibility).toMatchObject({
        inViewport: true,
        topmostWithinHover: true,
    });

    await findWidget.getByRole("textbox", { name: "Find" }).fill("int");
    for (const { buttonName, tooltipText } of [
        {
            buttonName: /Previous Match/,
            tooltipText: "Previous Match",
        },
        {
            buttonName: /Close/,
            tooltipText: "Close",
        },
    ]) {
        const button = findWidget.getByRole("button", { name: buttonName });
        await button.hover();
        const buttonHover = page.locator(".workbench-hover-container").last();
        await expect(buttonHover).toContainText(tooltipText);
        const buttonHitTest = await button.evaluate((element) => {
            const bounds = element.getBoundingClientRect();
            const topmostElement = document.elementFromPoint(
                bounds.left + bounds.width / 2,
                bounds.top + bounds.height / 2,
            );
            return {
                withinButton:
                    topmostElement === element ||
                    (topmostElement !== null &&
                        element.contains(topmostElement)),
                topmostClass:
                    topmostElement instanceof HTMLElement
                        ? topmostElement.className
                        : "",
                topmostTag: topmostElement?.tagName ?? "",
            };
        });
        expect(buttonHitTest).toMatchObject({ withinButton: true });
    }
    await expect
        .poll(() => page.evaluate(() => document.fonts.check("16px codicon")))
        .toBe(true);

    await page.keyboard.press("Escape");
    await page.keyboard.press("Control+=");
    await expect(editor.locator(".view-lines")).toHaveCSS("font-size", "16px");

    await page.getByRole("button", { name: "Build AST" }).click();
    const viewport = page.getByTestId("graph-viewport");
    await viewport.click();
    await page.keyboard.press("Control+=");
    await expect(page.getByRole("status", { name: "Graph zoom" })).toHaveText(
        "125%",
    );
});

test("focuses one function graph from a multi-function result", async ({
    page,
}) => {
    await page.goto("/");
    await setEditorCode(
        page,
        "int first() { return 1; } int second() { return 2; }",
    );
    await page.getByRole("button", { name: "Build AST" }).click();

    const selector = page.getByRole("combobox", {
        name: "Focused function graph",
    });
    await expect(selector).toBeVisible();
    const svg = page.getByTestId("analysis-graph").locator("svg");
    const completeViewBox = await svg.getAttribute("viewBox");
    const controls = page.locator(".graph-controls");
    const viewport = page.getByTestId("graph-viewport");
    expect(
        await Promise.all([
            controls.boundingBox(),
            viewport.boundingBox(),
        ]).then(
            ([controlsBounds, viewportBounds]) =>
                controlsBounds !== null &&
                viewportBounds !== null &&
                controlsBounds.y + controlsBounds.height <= viewportBounds.y,
        ),
    ).toBe(true);
    const secondCluster = svg.locator("g.cluster").nth(1);
    const secondClusterBounds = await secondCluster.boundingBox();
    expect(secondClusterBounds).not.toBeNull();
    if (!secondClusterBounds) throw new Error("second graph is not visible");
    await page.mouse.dblclick(
        secondClusterBounds.x + secondClusterBounds.width / 2,
        secondClusterBounds.y + secondClusterBounds.height / 2,
    );
    await expect(selector).toHaveValue("1");
    await expect
        .poll(() => svg.getAttribute("viewBox"))
        .not.toBe(completeViewBox);
    await expect
        .poll(async () => {
            const [clusterBounds, viewportBounds] = await Promise.all([
                svg.locator("g.cluster").nth(1).boundingBox(),
                viewport.boundingBox(),
            ]);
            if (!clusterBounds || !viewportBounds) return false;
            return (
                clusterBounds.x < viewportBounds.x + viewportBounds.width &&
                clusterBounds.x + clusterBounds.width > viewportBounds.x &&
                clusterBounds.y < viewportBounds.y + viewportBounds.height &&
                clusterBounds.y + clusterBounds.height > viewportBounds.y
            );
        })
        .toBe(true);

    await viewport.press("Escape");
    await expect(selector).toHaveValue("all");
    await expect(svg).toHaveAttribute("viewBox", completeViewBox ?? "");
});

test("scrolls and resizes long diagnostics", async ({ page }) => {
    await page.goto("/");
    await setEditorCode(
        page,
        Array.from(
            { length: 80 },
            (_, index) => `int broken_${index} = ;`,
        ).join("\n"),
    );
    await page.getByRole("button", { name: "Build AST" }).click();

    const diagnostics = page.getByTestId("analysis-error");
    await expect(diagnostics).toBeVisible();
    await expect
        .poll(() =>
            diagnostics.evaluate(
                (element) => element.scrollHeight > element.clientHeight,
            ),
        )
        .toBe(true);
    await diagnostics.click();
    await page.keyboard.press("Control+=");
    await expect(
        page.getByRole("status", { name: "Diagnostics font size" }),
    ).toHaveText("14px");
});

async function setEditorCode(page: Page, code: string) {
    const editor = page.getByTestId("source-editor").locator(".monaco-editor");
    await expect(editor).toBeVisible();
    await editor.click();
    await page.keyboard.press("Control+A");
    await page.keyboard.insertText(code);
}
