# Tool disclosure

## OpenAI ChatGPT / Codex — scaffold generation, 10 November 2026
Generated repository layout, CMake configuration, C++ declarations/common contracts,
placeholder translation units and CLI, contract smoke check, workflow/requirements templates,
and derivation outlines from the course PDF. No studied data structure, oracle arithmetic,
differential harness, adversarial generator or benchmark algorithm was implemented.

Human review/changes afterwards: TODO team must state what was reviewed and changed;
do not claim changes before they happen. Interface choices are proposed and require team review.

Other tools used for scaffold setup: Python (file generation/archive), Git (repository setup),
CMake 4.4.4 and GCC/CTest (build and scaffold checks). See docs/SETUP_VALIDATION.md.
GitHub connector: inspected existing tree and created setup branch for the scaffold.
Changes after initial generation: retained original README introduction and ignore rules; added explicit pipeline-exit checks and repository continuity guidance.

## Ongoing disclosure
Each member adds tools used, affected files, generated parts and subsequent human changes.
Declared AI use costs no marks under the PDF; understanding and explainability are assessed in viva.


## ChatGPT — Architect v1 interface review, 12 November 2026
Drafted an Architect contract review, proposed CLI/parser conventions, acceptance decisions and a sign-off checklist. Edited INTERFACE.md, README.md and app/TASKS.md; added docs/ARCHITECT_REVIEW.md. No algorithm modules, shared public headers or CMake source targets were modified as part of this milestone. All proposed decisions require human/team review; no approval or remote Git push is claimed. The Architect should record their subsequent edits and decisions here.

## ChatGPT — Architect Part 2 workload core, 12 November 2026
Generated `app/module.cpp` implementations for read_workload, write_workload and replay; added `tests/app_test.cpp`, `tests/cases/architect_sample.txt`, a test-linked `app_core` CMake target, and related documentation updates. Used CMake/CTest to compile and execute tests. Human author/Architect must review parser resource guards, input format and operation timing semantics before merge; team contract still requires sign-off. No Structure A/B, Oracle, Adversary or Analyst source implementations were changed.
