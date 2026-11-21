# Final submission audit - 21 November 2026
Base inspected: a33c51f7383d3f27eb56679adadd16905d8fe386.
This is a shared handoff; it does not claim individual credit for another member.

## Completed on Anagh's review branch
- Added actual full-scale D5 A/B replay for all four families, both modes.
- Added exact maximum-push and aligned-block counter verification.
- Added deterministic evaluator adapter with independent Python CLI tests.
- Restored the exact D5_250001006.pdf path and strengthened the uneven-size proof.
- Added raw correctness CSVs, reproducible figures, report/PDF and shared slide sources.
- Added a full pipeline and artifact-path checker. Teammate algorithms are unchanged.
- All 20 CTest checks passed; all eight full-scale replays passed at n=q=1000000.
  See VALIDATION.md for source revisions and evidence.

## Individual handoffs: commit these through your own GitHub account
| Owner | Concrete work remaining | Verification |
|---|---|---|
| Naresh / D1 | Add a built-state check at the start of LazySegmentTree::update and ::query, including empty intervals. Reject invalid Mode in build consistently with the agreed contract. Rename d1_250001052.pdf to D1_250001052.pdf. Replace the D1.md outline with the source of your already-uploaded PDF. Open your own PR and obtain another member's review. | An unbuilt tree's query({0,0}) and update({0,0},{1,0}) must throw logic_error; after build({}), the same empty operations must work. Rerun CTest and evaluator tests. |
| Harshal / D2 | Review, name and finalize D2.md; export the missing D2_250001032.pdf (1-2 pages). Check mode validation in B::build and bounds for configurable block size. Obtain a review on your module PR. | Include composition order, noncommutativity, sum formula and negative-gain extrema proof. Normal and overridden block sizes must preserve invariants. |
| Bhuvaneesh / D3 | Rename d3_250001017.pdf to D3_250001017.pdf; put your name on its Markdown source. Sign off INTERFACE.md with both structure owners. Confirm whether the instructor published an evaluator format beyond the supplied PDFs and adapt if needed. Fix the menu uint64 input conversion if claiming full seed support. | The menu ask_integer currently reads long long and casts UINT64_MAX to long long; seed entry can fail. Test seed 0, 253 and large uint64 through the menu, not just the generator CLI. Rehearse a fresh final-tag clone. |
| Arush / D4 | Rename d4_250001008.pdf to D4_250001008.pdf. Extend lifecycle/mode contract tests to all structures so A's pre-build bug cannot pass again. Link the production differential/shrinking evidence in report section 7. | Run exact seed 253 and modular seed 42, 1000000 operations each. Verify checks do not mutate state/counters and shrunk failures remain deletion-minimal. |
| Ranveer / D6 | Rename d6_250041031.pdf to D6_250041031.pdf and put your name on D6.md. Commit a full-scale timing campaign at n=q=1000000 on one stated machine, >=5 repetitions/point, with seeds and raw logs. Recover the exact generator/command behind the historical width_sweep.csv. Add predicted/measured curves, locate the crossover more closely, and attach cache profiling if making cache claims. Open your own PR and obtain review. | Existing timings have 60 rows at n=q=100000. Figures are now regenerated, but no predictive fit or new timing results have been invented. Show the prediction commit predates its measurements. |
| Anagh / D5 | Explain the reset/probe proof, four targets, and the new replay verifier; obtain another member's review of this PR. | n=1000000 has maximum 39 pushes/probe and 19500000 across 500000 probes. All eight full-scale replays pass. |

The lifecycle and menu findings are source-review findings, not a claim that
an official hidden-test suite has been run. No examiner inputs are available.

## Team/release actions
1. Keep the repository private; it was private during this audit. Verify the
   instructor's collaborator access in repository settings; username was not supplied.
2. Every member must have their own commits and at least one PR reviewed by
   someone else. Review records queried for PRs 2,3,8,9,12,17,18,20 were empty.
   A merge or a closed PR is not proof of a submitted APPROVED review.
3. No checkpoint/final tag refs were returned by the tag-ref listing. Do not
   invent historical checkpoint dates or backdate commits. Ask the instructor
   how to handle missed checkpoints if necessary.
4. Finish the individual owner actions above, merge reviewed PRs, rerun the
   entire pipeline, and inspect the final PDFs/slides.
5. Check the REPORT section on beyond-lecture work and actual references; each
   member must confirm the sources used and the accuracy of their contribution.
6. Only then create an annotated final tag, push it and verify its full commit SHA.
   Submit team name, Project 7, repository URL and that SHA through the announced
   portal/email. This audit does not create a prematurely incomplete final tag.
7. History cannot now be made into work spread over earlier dates. Keep genuine
   history, avoid force pushes, and disclose the late additions honestly.

## Ownership and attribution
Shared artifacts package the existing team's implementations and measurements.
No D1/D2/D3/D4/D6 proof was authored, signed, or committed on another person's
behalf. A teammate must review and approve PR #21 before it is merged.
