import argparse
from pathlib import Path

def export_width(source, destination, width):
    with Path(source).open(encoding="utf-8") as inp, Path(destination).open("w", encoding="utf-8", newline="\n") as out:
        header = [inp.readline() for _ in range(4)]
        if header[0].strip() != "CS253_V1":
            raise ValueError("Expected CS253_V1 input")
        n, q = map(int, header[3].split())
        if n < 1 or width < 1:
            raise ValueError("Positive array size and width required")
        width = min(width, n)
        out.writelines(header)
        out.write(inp.readline())
        for _ in range(q):
            fields = inp.readline().split()
            if len(fields) not in (3, 5) or fields[0] not in ("U", "Q"):
                raise ValueError("Incomplete or invalid base workload")
            left, right = map(int, fields[1:3])
            if right - left != width:
                left = left % (n - width + 1)
                right = left + width
            fields[1:3] = [str(left), str(right)]
            out.write(" ".join(fields) + "\n")
        if inp.read().strip():
            raise ValueError("Unexpected trailing input")
    return width

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Export the exact per-width input used by bench/module.cpp")
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--width", type=int, required=True)
    args = parser.parse_args()
    width = export_width(args.source, args.destination, args.width)
    print(f"Exported width {width}: {args.destination}")

