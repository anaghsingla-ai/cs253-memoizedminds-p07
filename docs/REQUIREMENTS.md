# Acceptance checklist grounded in requirements PDF

Source: CS253_MA253_Lab_Projects.docx.pdf, pp. 1–4, 13–14.
Unchecked items remain outstanding; scaffold creation satisfies no algorithm requirement.

- [ ] Freeze six named roles at first checkpoint; instructor has GitHub read access.
- [ ] A: lazy segment tree with affine tags; sum modulo 998244353 for arbitrary gains/offsets.
- [ ] Exact min/max for gains -1,0,1; negative gains swap extrema.
- [ ] B: square-root decomposition, per-block affine tags and aggregates.
- [ ] Application replays calibration corrections/queries and reports results and throughput.
- [ ] Plain-array oracle; one million seeded differential operations, shrinking failures, invariant checkers.
- [ ] Adversaries: maximum push-downs, alternating -1, B-favouring patterns.
- [ ] Full scale >=1,000,000 elements AND >=1,000,000 operations for structures/application/benchmarks.
- [ ] Width sweep from single element to whole array; locate/explain crossover or explain absence.
- [ ] Compare predicted versus measured costs using push-down counts versus block touches.
- [ ] Every timing: mean of >=5 runs with spread; record every seed and machine processor/cache/compiler.
- [ ] Back cache claims with profiler output; do not infer them from wall-clock times alone.
- [ ] README, INTERFACE, REPORT, TOOLS complete; raw CSV and figure scripts in results/.
- [ ] D1–D6 submitted as separate authored PDFs under docs/derivations/ (Markdown outlines are not submission PDFs).
- [ ] Every member has genuine distributed commits and >=1 PR reviewed by another member.
- [ ] No library container substitutes for studied structures; all implementations in C++.
- [ ] One command runs tests, application and every benchmark.
- [ ] Final tagged release passes fresh-clone full-scale demo and hidden-test entry points.
- [ ] Presentation: <=10–12 slides; interface/invariants, both structure diagrams, member modules/derivations,
      adversaries, predictions versus measurements. PDF says 15 minutes total but also 3–4 minutes per member;
      these conflict for six members. Clarify scheduling with instructor before preparing timing.

Evaluation 17 November 2026; submission 21 November 2026. Project marks 30/student:
presentation 10, viva 5, project/documentation/GitHub 15. Viva below 2/5 caps project marks at 8/15.
Module hidden-test failure limits that member’s hidden-test credit. One or two last-minute bulk uploads
score zero on individual GitHub criterion. Declare AI use; every member must explain and modify own code.
