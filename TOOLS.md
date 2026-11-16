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

## OpenAI ChatGPT / Codex - Member 5 implementation, 14 November 2026
Generated the adversary generators, local replay exporter/CLI, module tests,
additive CMake targets, CI workflow, module documentation and D5 proof.
Inspected the professor's PDF and current main/part-e branches through GitHub.
Preserved shared interfaces and all teammate algorithms. Test-only reference
arithmetic is independent from the structures and does not replace the Oracle.
Human review/changes: pending Anagh's review; no human edits are claimed.

## OpenAI ChatGPT / Codex - Member 5 review, 16 November 2026
Reviewed the current main and part-e/adversary branches and the uploaded submission
guidelines. Renamed generator helpers/variables for readability; added a repeated
partial-update push-count regression test; updated the D5 derivation and exported
it as D5_250001006.pdf using the roll number supplied by Anagh. Added the PDF
export script and recorded verification in adversary/README.md and REPORT.md.
Used GCC 14.2.0, CMake 4.4.5, CTest, Python, ReportLab and Poppler for checks.
Human review/changes: Anagh supplied the roll number; code review remains pending.
Representative prompt: "See whether my D5 part is completed; finish anything
missing with easy C++ code and push it into my branch."
