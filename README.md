
# CS253 Project 7 — Lazy Propagation and What Composes

**Team:** MemoizedMinds

## 1. Requirements

Install the following tools:

- MinGW-w64 GCC
- CMake 3.16 or newer
- Python 3.9 or newer
- GNU Make (MinGW Make)

Check each installation separately in PowerShell.

**Check GCC:**
```powershell
g++ --version
```

**Check CMake:**
```powershell
cmake --version
```

**Check MinGW Make:**
```powershell
mingw32-make --version
```

**Check Python:**
```powershell
python --version
```

> Important: The project uses `__int128_t`, which is supported by GCC and Clang but not MSVC. Use MinGW-w64 GCC on Windows.

---

## 2. Clone the Repository

Open PowerShell.

**Step 1 — Clone the repository:**

```powershell
git clone https://github.com/anaghsingla-ai/cs253-memoizedminds-p07.git
```

**Step 2 — Enter the project directory:**

```powershell
cd cs253-memoizedminds-p07
```

If you have already cloned the repository, navigate to its existing directory instead. Do not clone it again.

---

## 3. Configure the Project

Run this command from inside the project directory:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

Wait for CMake to finish successfully before proceeding.

---

## 4. Compile the Project

Run this command only after configuration succeeds:

```powershell
cmake --build build --parallel 2
```

Wait for compilation to finish.

> Important: Do not delete the `build` directory between configuration and compilation. If you delete it, you must run the configuration command again.

---

## 5. Run the Program

**Run the sample input using both structures:**

```powershell
.\build\calibration.exe --run tests/cases/architect_sample.txt --structure both
```

**Start the interactive program:**

```powershell
.\build\calibration.exe
```

---

## 6. Use the Windows Helper Script

The repository includes `run.ps1`.

**Run the default mode:**

```powershell
.\run.ps1
```

**Open the interactive menu:**

```powershell
.\run.ps1 -Mode Menu
```

**Run the available tests:**

```powershell
.\run.ps1 -Mode Tests
```

The helper script can configure and build the project when required.

---

## 7. Run the Evaluation Adapter

PowerShell does not support Bash-style input redirection using `<`. Use the following commands instead.

**Run Structure A:**

```powershell
Get-Content tests/cases/architect_sample.txt | python scripts/run_ops.py --structure A
```

**Run Structure B:**

```powershell
Get-Content tests/cases/architect_sample.txt | python scripts/run_ops.py --structure B
```

Alternatively, from Windows Command Prompt (`cmd`), you can use:

```cmd
python scripts/run_ops.py --structure A < tests\cases\architect_sample.txt
```

---

## 8. Run Tests

**Run the Windows test helper:**

```powershell
.\run.ps1 -Mode Tests
```

**Alternatively, run CTest directly:**

```powershell
ctest --test-dir build --output-on-failure
```

Bash-based scripts such as `scripts/run_all.sh` are intended for Linux, macOS, or WSL.

---

## 9. Input Format

The program accepts operations on an array of sensor readings.

An affine update applies:

\[
x \leftarrow ax+b
\]

to every element in the specified range.

Refer to `interface.md` for the exact input and output format.

---

## 10. Evaluation Interface

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

It accepts the selected implementation through the `--structure` argument.

---

## 11. Benchmarks and Performance

The project is designed to handle calibration tables containing up to one million sensor readings.

- Structure A uses lazy propagation in a segment tree.
- Structure B uses square-root decomposition with affine tags.
- The Oracle provides a reference implementation for correctness verification.

---

## 12. Repository Structure

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

## 13. Known Limitations

- MSVC is not supported because the project uses `__int128_t`.
- Windows users should use MinGW-w64 GCC or WSL.
- Bash-based scripts are intended for Linux, macOS, or WSL.
- PowerShell users should use `Get-Content` pipelines instead of Bash-style `<` input redirection.

---

## 14. References

- C++17 Documentation
- CMake Documentation
- Python Documentation
- Lazy Propagation and Segment Trees
- Square Root Decomposition
