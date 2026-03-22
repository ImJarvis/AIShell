---
description: Enforce Senior Engineer architectural mindset and RCA-first methodology across all tasks.
---

# Senior Engineer Methodology

When generating code, analyzing bugs, or architecting solutions for the AIHollowShell project, you MUST adhere to the following principles:

## 1. Root Cause Analysis (RCA) First
- Never apply "band-aid" patches to immediate symptoms without tracing the bug back to its absolute origin.
- Analyze the blast radius of any modification. What else does this data structure touch? How does this affect memory lifecycles?

## 2. Eliminate Unnecessary Complexity
- If a feature is causing disproportionate instability compared to its value (e.g., background async polling for minor UI updates), critically evaluate if it should be redesigned or entirely deleted.
- Prefer deterministic, mathematically proven boundaries over "magic numbers" or arbitrary debouncing loops.

## 3. Platform-Native Constraints
- Always acknowledge the target OS constraints explicitly (e.g., `<windows.h>` macro pollution like `min`/`max`, strict MSVC `/GS` buffer overrun detections, ImGui internal layout boundary tracking).
- Ensure cross-platform logic (like parsing Unix `tldr` documentation) is strictly translated or constrained natively when operating within a Windows PowerShell environment.

## 4. Architectural Modularity
- Refrain from putting massive monolithic logic blocks inside central `Application.cpp` loops.
- Isolate functional areas into strictly typed header and implementation files.

By invoking this workflow, you commit to maintaining a highly robust, Senior-level standard of software engineering.
