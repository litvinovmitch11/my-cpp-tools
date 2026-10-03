import { loader } from "@monaco-editor/react";
import * as monaco from "monaco-editor/editor/editor.api";
import "monaco-editor/features/bracketMatching/register.js";
import "monaco-editor/features/codeEditor/register.js";
import "monaco-editor/features/comment/register.js";
import "monaco-editor/features/find/register.js";
import "monaco-editor/features/wordOperations/register.js";
import "monaco-editor/languages/definitions/cpp/register.js";
import EditorWorker from "monaco-editor/editor/editor.worker?worker";

self.MonacoEnvironment = {
    getWorker: () => new EditorWorker(),
};

loader.config({ monaco });
