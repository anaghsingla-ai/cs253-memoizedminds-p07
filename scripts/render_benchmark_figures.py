import argparse
import csv
from collections import defaultdict
import html
import math
from pathlib import Path
import statistics
import pymupdf as fitz

ROOT = Path(__file__).resolve().parents[1]
COLORS = {"structA": "#087f8c", "structB": "#d47026"}

def text(x, y, value, size=12, color="#203247"):
    return f'<text x="{x:.2f}" y="{y:.2f}" font-family="sans-serif" font-size="{size}" fill="{color}">{html.escape(str(value))}</text>'

def line(x1, y1, x2, y2, color="#dce3e8", width=1, dash=""):
    dashed = f' stroke-dasharray="{dash}"' if dash else ""
    return f'<line x1="{x1:.2f}" y1="{y1:.2f}" x2="{x2:.2f}" y2="{y2:.2f}" stroke="{color}" stroke-width="{width}"{dashed}/>'

def canvas(title, height=560):
    return [f'<svg xmlns="http://www.w3.org/2000/svg" width="960" height="{height}" viewBox="0 0 960 {height}">',
            '<rect width="100%" height="100%" fill="white"/>', text(45, 34, title, 21, "#123247")]

def series(svg, points, color, dash=""):
    if dash:
        for (x1,y1),(x2,y2) in zip(points,points[1:]):
            distance = math.hypot(x2-x1,y2-y1)
            if distance == 0:
                continue
            offset = 0
            while offset < distance:
                end = min(offset+6,distance)
                svg.append(line(x1+(x2-x1)*offset/distance,y1+(y2-y1)*offset/distance,
                                x1+(x2-x1)*end/distance,y1+(y2-y1)*end/distance,color,2))
                offset += 10
        return
    values = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
    svg.append(f'<polyline points="{values}" fill="none" stroke="{color}" stroke-width="2"/>')
    if not dash:
        for x, y in points:
            svg.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="4" fill="{color}"/>')

def write_figure(svg, output, stem):
    path = output / (stem + ".svg")
    path.write_text("\n".join(svg + ["</svg>"]), encoding="utf-8")
    with fitz.open(path) as drawing:
        drawing.get_page_pixmap(0, matrix=fitz.Matrix(1.5, 1.5)).save(output / (stem + ".png"))

def fit_constants(first, second, times):
    aa = sum(x*x for x in first)
    bb = sum(x*x for x in second)
    ab = sum(x*y for x, y in zip(first, second))
    ay = sum(x*y for x, y in zip(first, times))
    by = sum(x*y for x, y in zip(second, times))
    determinant = aa*bb-ab*ab
    if determinant <= 0:
        raise ValueError("Cannot fit a singular counter model")
    return ((bb*ay-ab*by)/determinant, (aa*by-ab*ay)/determinant)

def render(csv_path, output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    with Path(csv_path).open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    groups = defaultdict(list)
    for row in rows:
        groups[(row["structure"], int(row["width"]))].append(row)
    widths = sorted({key[1] for key in groups})
    if len({(r["n"], r["operations"], r["seed"], r["workload"]) for r in rows}) != 1:
        raise ValueError("A figure must describe one campaign")
    summary = {}
    for name in COLORS:
        summary[name] = []
        for width in widths:
            group = groups[name, width]
            if len(group) < 5:
                raise ValueError("At least five repetitions per plotted point are required")
            times = [float(r["elapsed_seconds"]) for r in group]
            summary[name].append(dict(width=width, mean=statistics.mean(times),
                                      sd=statistics.stdev(times), rows=group))
    n, q, seed = int(rows[0]["n"]), int(rows[0]["operations"]), rows[0]["seed"]
    raw = "results/raw/" + Path(csv_path).name
    source_caption = raw + " | scripts/render_benchmark_figures.py"
    models = {}
    for name, keys in [("structA", ("nodes_visited", "push_downs")),
                       ("structB", ("full_blocks", "partial_elements"))]:
        first = [statistics.mean(int(r[keys[0]]) for r in d["rows"]) for d in summary[name]]
        second = [statistics.mean(int(r[keys[1]]) for r in d["rows"]) for d in summary[name]]
        constants = fit_constants(first, second, [d["mean"] for d in summary[name]])
        models[name] = [constants[0]*x + constants[1]*y for x, y in zip(first, second)]
    for fitted, stem, title in [(False, "full_scale_time", "Full-scale runtime - mean and sample standard deviation"),
                                 (True, "predicted_vs_measured", "Measured runtime and calibrated cost model")]:
        svg = canvas(title)
        maximum = max(d["mean"]+d["sd"] for points in summary.values() for d in points)
        if fitted:
            maximum = max(maximum, *(max(v) for v in models.values()))
        step = 10 ** math.floor(math.log10(maximum / 4))
        ratio = maximum / (4*step)
        step *= next(v for v in [1, 2, 5, 10] if v >= ratio)
        ymax = math.ceil(maximum/step)*step
        xp = lambda width: 95+785*math.log10(width)/math.log10(max(widths))
        yp = lambda value: 395-285*value/ymax
        for tick in range(round(ymax/step)+1):
            value = tick*step
            svg += [line(95, yp(value), 880, yp(value)), text(45, yp(value)+4, f"{value:g}")]
        for width in widths:
            svg += [line(xp(width),395,xp(width),402), text(xp(width)-13,423,f"{width:,}",11)]
        svg += [text(95,82,"Elapsed seconds per replay"), text(350,456,"Range width (elements; logarithmic axis)")]
        for index, (name, color) in enumerate(COLORS.items()):
            svg += [text(560,64+19*index, "A: lazy segment tree" if name=="structA" else "B: square-root decomposition",12,color)]
            points = []
            for d in summary[name]:
                x, y = xp(d["width"]), yp(d["mean"])
                points.append((x, y))
                svg += [line(x,yp(d["mean"]-d["sd"]),x,yp(d["mean"]+d["sd"]),color,2),
                        line(x-5,yp(d["mean"]-d["sd"]),x+5,yp(d["mean"]-d["sd"]),color,2),
                        line(x-5,yp(d["mean"]+d["sd"]),x+5,yp(d["mean"]+d["sd"]),color,2)]
            series(svg, points, color)
            if fitted:
                series(svg, [(xp(w),yp(v)) for w,v in zip(widths,models[name])],color,"6 4")
        svg += [text(45,490,f"n={n:,}; q={q:,}; seed={seed}; 5 repetitions/structure/width; bars = one sample SD.",11)]
        svg += [text(45,513,"Dashed = least-squares fit on this same data, not an independent premeasurement prediction." if fitted else
                    "A wins at width 1 and from 1,000; B wins at 64. The samples bracket two crossings.",11)]
        svg += [text(45,539,source_caption,10)]
        write_figure(svg,output,stem)
    svg = canvas("Full-scale operation counts - separate units",720)
    panels = [("structA","nodes_visited","A: tree-node entries"),("structA","push_downs","A: parent push events"),
              ("structB","full_blocks","B: complete block accesses"),("structB","partial_elements","B: raw-element accesses")]
    for index,(name,key,title) in enumerate(panels):
        x0=75+455*(index%2)
        y0=100+265*(index//2)
        values = [statistics.mean(int(r[key]) for r in d["rows"]) for d in summary[name]]
        spreads = [statistics.stdev(int(r[key]) for r in d["rows"]) for d in summary[name]]
        maximum=max(v+s for v,s in zip(values,spreads)) or 1
        xp=lambda width: x0+355*math.log10(width)/math.log10(max(widths))
        yp=lambda value: y0+165*(1-value/maximum)
        svg.append(text(x0,y0-18,title,15))
        for part in [0,.5,1]:
            svg += [line(x0,yp(part*maximum),x0+355,yp(part*maximum)),
                    text(x0-52,yp(part*maximum)+4,f"{part*maximum/1e6:.1f}M",10)]
        for width,value,sd in zip(widths,values,spreads):
            x=xp(width)
            svg += [text(x-10,y0+185,str(width),9),line(x,yp(value-sd),x,yp(value+sd),COLORS[name],2)]
        series(svg,[(xp(w),yp(v)) for w,v in zip(widths,values)],COLORS[name])
    svg += [text(280,645,"Range width (elements; logarithmic axis)",12),
            text(45,673,"Counts per 1,000,000-operation replay. Five-run means; counter SD is zero for this deterministic input.",11),
            text(45,699,source_caption,10)]
    write_figure(svg,output,"full_scale_counters")
    return summary

if __name__ == "__main__":
    parser=argparse.ArgumentParser(description="Render reproducible full-scale figures without Matplotlib")
    parser.add_argument("csv",nargs="?",type=Path,default=ROOT/"results/raw/full_scale_width_sweep.csv")
    parser.add_argument("output",nargs="?",type=Path,default=ROOT/"results/figures")
    args=parser.parse_args()
    render(args.csv,args.output)
    print(f"Regenerated all three full-scale SVG/PNG figures from {args.csv}")

