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


## ChatGPT — Architect Parts 3–4, 14 November 2026
Implemented the Architect CLI and application integration in `app/main.cpp`; updated CMake targets to
link app_core with the existing Structure A/B implementations; added CLI integration checks; replaced
stale scaffold-status documentation; and made `--all` execute the available A/B sample smoke check
while exiting nonzero with explicit blockers for missing Oracle, Adversary and Analyst modules.
Authored the D3 proof in `docs/derivations/D3.md` and exported `D3.pdf` using ReportLab. The existing Structure A and
Structure B implementation files were not edited. The work was built and tested locally; the team must
review the CLI contract and proof, insert the named author's name, obtain six-owner interface approval,
and complete the missing owner modules before claiming final pipeline completion.

## Repository integration assistance — 17 November 2026

OpenAI ChatGPT assisted with read-only inspection of the remote branch histories and CI logs,
creating pre-integration safety refs, identifying and repairing the malformed Structure B
conflict-resolution edit, adding terminal menu and benchmark integration, updating regression
checks and reconciling report statistics against the committed CSV. GitHub Actions was used
to compile and run the CTest suite after changes. The repository owner/team must review the
resulting code and retain responsibility for the submitted implementation and derivations.

## ChatGPT / Codex - Adversary readability refactor, 17 November 2026

Requested by Anagh Singla (250001006): simplify D5 C++ with ordinary if/else,
named variables and clearer loops while preserving the team interface.
Generated edits affect adversary/module.cpp, adversary/main.cpp,
adversary/export.hpp and tests/adversary_test.cpp. Added
adversary/check_refactor.sh and extended .github/workflows/adversary.yml to compare
the rewritten CLI's output with commit 76e81c07d9fb41ae3719f1b4ac407ae3c84293aa.
Updated adversary/README.md, including removal of the stale deleted-export-script
command. Fixed the test harness's unconditional failure-file success message.
Shared headers, the D5 proof and all other owners' algorithms are preserved.

Validation is performed by GitHub Actions: a Release build, the CTest suite,
264 old/new workload comparisons and six invalid-input CLI checks. Any successful
run is evidenced by its Actions job; no local execution is claimed for this edit.
Human code review is still required and is not claimed here.
