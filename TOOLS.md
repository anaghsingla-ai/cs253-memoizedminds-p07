# TOOLS.md

## 1. Common development tools

The following tools form the shared development environment for the project and were used by all six team members.

| Tool | Used by | Used for | Project areas affected |
|---|---|---|---|
| C++17 compiler (GCC) | All six members | Compiling and validating the C++17 implementations and tests | C++ source files, headers and test targets |
| MinGW-w64 | All six members | Providing the Windows GCC toolchain and build environment | C++ build configuration and build outputs |
| CMake | All six members | Configuring the project and generating build files | `CMakeLists.txt` and build directory |
| Visual Studio Code | All six members | Editing, navigating and reviewing source code, tests and documentation | Source code, tests and documentation |
| Git and GitHub | All six members | Version control, branches, commits, pushes and collaboration | Project repository and its history |
| Terminal / PowerShell | All six members | Running build commands, tests, scripts and the application | Build outputs, test results and generated files |

## 2. Team members and responsibilities

| Name | Project responsibility |
|---|---|
| Naresh | Structure A — Lazy Segment Tree |
| Harshal | Structure B — Square-Root Decomposition |
| Bhuvaneesh Vijayvargiya | Architect — application architecture, integration, interfaces and documentation |
| Arush Agrawal | Oracle — brute-force reference implementation and differential testing |
| Anagh Singla | Adversary — adversarial workloads and correctness stress testing |
| Ranveer | Analyst — benchmarking and performance analysis |

## 3. AI tools

AI assistance was used by **all six team members** during project development. The specific use varied by member and task.

| AI tool | Used by | Used for | Project areas affected | What the team did with the output |
|---|---|---|---|---|
| ChatGPT (OpenAI) | All six members: Naresh, Harshal, Bhuvaneesh Vijayvargiya, Arush Agrawal, Anagh Singla and Ranveer | Assistance with understanding the project requirements, discussing algorithms and design choices, explaining code, debugging, test-case ideas, benchmarking/analysis guidance and documentation | Different implementation modules, tests, benchmark analysis and documentation, depending on each member's task | Members reviewed and adapted suggestions. Code was compiled and tested where applicable; the team remains responsible for the submitted work. |

### Declaration

AI tools were used as assistants by all six team members. Their outputs were not treated as automatically correct. Team members reviewed and adapted AI-generated suggestions and are responsible for understanding, validating and explaining the submitted implementation. Any external material used in the project must be acknowledged according to the course submission guidelines.

Before submission, verify the role-specific tools in Section 2 and add any other tools actually used. Do not retain a tool entry if the named member did not use it.

## 4. Submission audit - 21 November 2026

| Tool / version | Used by | Purpose | Files | Verification |
|---|---|---|---|---|
| ChatGPT / Codex, 21 November 2026 | Anagh Singla | D5 replay checks, shared evaluator, audit, documentation and export scripts | adversary/replay_check.cpp, scripts/, tests/evaluator_test.py, README.md, REPORT.md, docs/ | GitHub CI, independent Python arithmetic, raw CSV statistics and PDF visual inspection |
| Python 3.14.5, PyMuPDF 2.0 | Anagh Singla | Shared PDF/SVG generation and local syntax checks | REPORT.pdf, D5 PDF, slides, figures | Page counts, extracted text and rendered previews |
| GitHub Actions Ubuntu runners | Anagh Singla | Build and execute correctness checks | CI logs and results/validation/ | 20 CTest checks and eight n=q=1000000 replays |

### Representative prompts
- 21 November 2026: "Complete the things missing for D5, check the whole group,
  add shared missing work, but tell the individual members what they must commit."
- 17 November 2026: "Use normal syntax with if else so I can understand my D5 code."
- 17 November 2026: "Explain maximum push-downs, alternating negative gain,
  block-friendly inputs and range-width sweep for my presentation."

The quoted prompt excerpts describe Anagh's assistance. Other members must add
their own dated representative prompts and confirm their own tool versions.
No tool wrote, signed or impersonated another member's individual derivation
during this audit. Shared report/slides summarize existing repository work.

