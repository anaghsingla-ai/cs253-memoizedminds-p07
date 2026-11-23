
# CS253 Project 7 — Lazy Propagation and What Composes

**Team:** MemoizedMinds

## Team Members

| Name | Roll Number |
|---|---|
| Anagh Singla | 250001006 |
| Arush Agrawal | 250001008 |
| Bhuvaneesh Vijayvargiya | 250001017 |
| Naresh Chaudhary | 250001052 |
| Ranveer Singh Thakur | 250041031 |

## Project Overview

This project implements and compares two data structures for efficiently applying range-based affine transformations to a calibration table containing up to one million sensor readings.

Each update applies the transformation:

\[
x \leftarrow ax+b
\]

The project implements:

- **Structure A:** Lazy Segment Tree
- **Structure B:** Square Root Decomposition
- **Oracle:** A brute-force implementation used for correctness verification

The implementations support range updates and range queries while maintaining correctness under large values using `__int128_t`.

---

## 1. Requirements

Before running the project, make sure the following tools are installed:

- C++ compiler supporting C++17
- CMake 3.16 or newer
- Python 3.9 or newer
- GNU Make / MinGW Make (Windows)

### Windows users

We recommend using **MinGW-w64 GCC** with CMake.

Check that the required tools are available:

```powershell
g++ --version
cmake --version
mingw32-make --version
python --version
```

If any command is not recognized, install the missing tool and ensure its `bin` directory is added to your Windows PATH.

> **Important:** This project uses `__int128_t`, which is supported by GCC and Clang but not MSVC. Use MinGW-w64 GCC on Windows rather than the Microsoft Visual C++ compiler.

---

## 2. Clone the Repository

Open PowerShell or a terminal and run:

```powershell
git clone https://github.com/anaghsingla-ai/cs253-memoizedminds-p07.git
cd cs253-memoizedminds-p07
```

If you have already cloned the repository, simply navigate to its directory.

---

## 3. Configure the Project

### Windows — PowerShell

Run:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

This creates the `build` directory and generates the build files.

### Linux / macOS

Run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

---

## 4. Compile the Project

After configuration succeeds, run:

```powershell
cmake --build build --parallel 2
```

This compiles the project and creates the executable inside the `build` directory.

**Important:** Do not delete the `build` directory between configuration and compilation. If you intentionally delete it, run the configuration command again before building.

---

## 5. Run the Program

### Option A: Run the sample input using both structures

On Windows PowerShell:

```powershell
.\build\calibration.exe --run tests/cases/architect_sample.txt --structure both
```

This runs the sample input using both Structure A and Structure B.

### Option B: Run using the interactive menu

```powershell
.\build\calibration.exe
```

Follow the instructions displayed in the terminal.

### Option C: Use the Windows helper script

The repository also includes `run.ps1`.

Run the default mode:

```powershell
.\run.ps1
```

Open the interactive menu:

```powershell
.\run.ps1 -Mode Menu
```

Run the available tests:

```powershell
.\run.ps1 -Mode Tests
```

The helper script can configure and build the project when required.

---

## 6. Run the Evaluation Adapter

The Python evaluation adapter can execute the implementations independently.

### Windows — PowerShell

PowerShell does not support Bash-style input redirection using `<`.

Use `Get-Content` and a pipeline instead.

**Structure A:**

```powershell
Get-Content tests/cases/architect_sample.txt | python scripts/run_ops.py --structure A
```

**Structure B:**

```powershell
Get-Content tests/cases/architect_sample.txt | python scripts/run_ops.py --structure B
```

### Alternative for Windows

If you prefer input redirection, use Command Prompt (`cmd`) instead of PowerShell:

```cmd
python scripts/run_ops.py --structure A < tests\cases\architect_sample.txt
```

### Linux / macOS / WSL

Use standard input redirection:

```bash
python3 scripts/run_ops.py --structure A < tests/cases/architect_sample.txt
```

```bash
python3 scripts/run_ops.py --structure B < tests/cases/architect_sample.txt
```

---

## 7. Run Tests

### Windows

```powershell
.\run.ps1 -Mode Tests
```

You can also run CTest directly:

```powershell
ctest --test-dir build --output-on-failure
```

### Linux / macOS

```bash
ctest --test-dir build --output-on-failure
```

The repository's Bash-based test scripts should be executed in Linux, macOS, or WSL rather than native PowerShell.

---

## 8. Input Format

The program accepts operations on an array of sensor readings.

Supported operations include:

- Range affine updates
- Range sum queries
- Range minimum queries
- Other operations specified by the evaluation interface

An affine update applies:

\[
x \leftarrow ax+b
\]

to every element in the specified range.

Refer to `interface.md` for the exact input and output format.

---

## 9. Evaluation Interface

The project provides three implementations:

| Implementation | Description |
|---|---|
| A | Lazy Segment Tree |
| B | Square Root Decomposition |
| Oracle | Brute-force reference implementation |

The evaluation adapter is located at:

```text
scripts/run_ops.py
```

It accepts the selected structure through the `--structure` argument.

---

## 10. Benchmarks and Performance

The project is designed to handle large calibration tables, including up to one million sensor readings.

The implementations use different approaches to support efficient range operations:

- Structure A uses lazy propagation in a segment tree.
- Structure B uses square-root decomposition with affine tags.
- The Oracle provides a straightforward reference for validating results.

---

## 11. Repository Structure

```text
.
├── CMakeLists.txt
├── README.md
├── interface.md
├── run.ps1
├── src/
├── include/
├── tests/
│   └── cases/
├── scripts/
│   ├── run_ops.py
│   └── run_all.sh
└── build/
```

The `build/` directory is generated during compilation and does not need to be committed.

---

## 12. Known Limitations

- MSVC is not supported because the project uses `__int128_t`.
- Windows users should use MinGW-w64 GCC or WSL.
- Bash-based scripts are intended for Linux, macOS, or WSL.
- PowerShell users should use the `Get-Content` pipeline instead of Bash-style `<` input redirection.

---

## 13. References

- C++17 Documentation
- CMake Documentation
- Python Documentation
- Lazy Propagation and Segment Trees
- Square Root Decomposition
