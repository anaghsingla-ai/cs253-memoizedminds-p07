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
