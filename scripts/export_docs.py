"""Regenerate shared PDF artifacts and precise SVG figures from tracked sources."""
import csv
from collections import defaultdict
import html
import io
import json
import math
from pathlib import Path
import re
import statistics
import pymupdf as fitz
from render_benchmark_figures import render

ROOT = Path(__file__).resolve().parents[1]
FIG = ROOT / "results/figures"
CSS = """
body {font-family: sans-serif; font-size: 10.4pt; line-height: 1.33; color: #203247;}
h1 {font-size: 19pt; color: #123247; margin: 0 0 12pt;}
h2 {font-size: 16pt; color: #123247; margin: 0 0 16pt;}
h3 {font-size: 11.5pt; color: #087f8c;}
p {margin: 0 0 9pt;} li {margin-bottom: 5pt;}
table {border-collapse: collapse; width: 100%; font-size: 8.5pt; margin: 12pt 0;}
th {background: #e6f2f4; color: #123247;}
td,th {padding: 6pt; border: .5pt solid #ccdade; text-align: left;}
code {font-family: monospace; font-size: 9pt; color: #166575;}
pre {background: #f0f5f7; padding: 8pt; font-size: 8pt; white-space: pre-wrap;}
"""
def inline(text):
    text = html.escape(text)
    text = re.sub(r"`([^`]+)`", r"<code>\1</code>", text)
    text = re.sub(r"\*\*([^*]+)\*\*", r"<b>\1</b>", text)
    return text

def markdown(text):
    lines, result, i = text.splitlines(), [], 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("<!--") or not line.strip():
            i += 1
            continue
        if line.startswith("```"):
            block = []
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                block.append(lines[i]); i += 1
            result.append("<pre>" + html.escape("\n".join(block)) + "</pre>")
            i += 1
        elif line.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                cells = [c.strip() for c in lines[i].strip().strip("|").split("|")]
                if not all(re.fullmatch(r"[-: ]+", c) for c in cells):
                    tag = "th" if not rows else "td"
                    rows.append("<tr>" + "".join(f"<{tag}>{inline(c)}</{tag}>" for c in cells) + "</tr>")
                i += 1
            result.append("<table>" + "".join(rows) + "</table>")
        elif line.startswith("#"):
            level = len(line) - len(line.lstrip("#"))
            result.append(f"<h{level}>" + inline(line[level:].strip()) + f"</h{level}>")
            i += 1
        elif line.startswith("- "):
            result.append("<p>&bull; " + inline(line[2:]) + "</p>")
            i += 1
        else:
            paragraph = [line]
            i += 1
            while i < len(lines) and lines[i].strip() and not lines[i].startswith(("#", "|", "- ", "```", "<!--")):
                paragraph.append(lines[i]); i += 1
            result.append("<p>" + inline(" ".join(paragraph)) + "</p>")
    return "".join(result)

def svg_plots():
    FIG.mkdir(parents=True, exist_ok=True)
    rows = list(csv.DictReader((ROOT / "results/raw/width_sweep.csv").open()))
    groups = defaultdict(list)
    for row in rows:
        groups[(row["structure"], int(row["width"]))].append(row)
    widths = sorted({w for _, w in groups})
    assert all(len(v) >= 5 for v in groups.values())
    def start(title, height=480):
        return [f'<svg xmlns="http://www.w3.org/2000/svg" width="920" height="{height}" viewBox="0 0 920 {height}">',
                '<rect width="100%" height="100%" fill="white"/>',
                f'<text x="50" y="32" font-family="sans-serif" font-size="20" fill="#123247">{title}</text>']
    def text(x,y,t,size=12,color="#203247"):
        return f'<text x="{x}" y="{y}" font-family="sans-serif" font-size="{size}" fill="{color}">{html.escape(str(t))}</text>'
    def line(x1,y1,x2,y2,color="#ccd6dd",width=1):
        return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{width}"/>'
    colors = {"structA":"#087f8c","structB":"#e07a33"}
    s=start("Elapsed time vs range width - mean and sample SD")
    x=lambda w: 85+755*math.log10(w)/5
    y=lambda t: 365-1000*t
    for tick in [0,.05,.1,.15,.2,.25,.3]:
        s += [line(85,y(tick),850,y(tick)),text(30,y(tick)+4,f"{tick:.2f}")]
    for w in widths:
        s += [line(x(w),365,x(w),372),text(x(w)-12,391,str(w),11)]
    for name,color in colors.items():
        points=[]
        for w in widths:
            vals=[float(r["elapsed_seconds"]) for r in groups[(name,w)]]
            mean,sd=statistics.mean(vals),statistics.stdev(vals)
            xx,yy=x(w),y(mean)
            points.append(f"{xx},{yy}")
            s += [line(xx,y(mean-sd),xx,y(mean+sd),color,2),
                  line(xx-5,y(mean-sd),xx+5,y(mean-sd),color,2),
                  line(xx-5,y(mean+sd),xx+5,y(mean+sd),color,2),
                  f'<circle cx="{xx}" cy="{yy}" r="4" fill="{color}"/>']
        s.append(f'<polyline points="{" ".join(points)}" fill="none" stroke="{color}" stroke-width="2"/>')
    s += [text(85,60,"Elapsed seconds per 100000 operations"),text(340,419,"Range width (elements, logarithmic axis)"),
          text(620,63,"A - lazy segment tree",13,colors["structA"]),
          text(620,84,"B - square-root blocks",13,colors["structB"]),
          text(50,453,"n=q=100000; seed 253; 5 runs/structure/width; bars = one sample SD.",11),
          text(50,470,"Source: results/raw/width_sweep.csv | Script: scripts/export_docs.py",10),"</svg>"]
    (FIG / "width_sweep_time.svg").write_text("\n".join(s),encoding="utf-8")
    s=start("Operation counts - units remain separate",650)
    panels=[("structA","nodes_visited","A: tree-node visits"),("structA","push_downs","A: parent push events"),
            ("structB","full_blocks","B: full-block accesses"),("structB","partial_elements","B: raw-element scans")]
    for index,(name,key,title) in enumerate(panels):
        left=65+(index%2)*450; top=85+(index//2)*255
        vals=[statistics.mean(int(r[key]) for r in groups[(name,w)]) for w in widths]
        maxv=max(vals) or 1
        points=[]
        s.append(text(left,top-15,title,15))
        for tick in [0,.5,1]:
            yy=top+170*(1-tick)
            s += [line(left,yy,left+365,yy),text(left-45,yy+4,f"{maxv*tick/1e6:.1f}M",10)]
        for w,value in zip(widths,vals):
            xx=left+365*math.log10(w)/5; yy=top+170*(1-value/maxv)
            points.append(f"{xx},{yy}")
            s += [f'<circle cx="{xx}" cy="{yy}" r="3" fill="{colors[name]}"/>',
                  text(xx-10,top+190,str(w),9)]
        s.append(f'<polyline points="{" ".join(points)}" fill="none" stroke="{colors[name]}" stroke-width="2"/>')
    s += [text(260,606,"Range width (elements, logarithmic axis)",12),
          text(50,628,"Each counter is per 100000 operations; raw file and script as in Figure 1.",11),"</svg>"]
    (FIG / "width_sweep_counters.svg").write_text("\n".join(s),encoding="utf-8")

def add_markdown_page(doc,text,label,css=CSS):
    page=doc.new_page(width=595,height=842)
    page.draw_rect(fitz.Rect(0,0,595,11),color=None,fill=(.03,.5,.55))
    page.insert_text((48,40),label,fontsize=8,color=(.25,.4,.45))
    spare,scale=page.insert_htmlbox(fitz.Rect(48,63,547,788),markdown(text),
                                  css=css,scale_low=.76)
    if spare < 0:
        raise RuntimeError(f"Page did not fit: {label}")
    if scale < .88:
        print(f"Layout scaled to {scale:.3f}: {label}")
    return page

def add_svg_page(doc,name,title):
    page=doc.new_page(width=595,height=842)
    page.insert_text((40,55),title,fontsize=18,color=(.07,.2,.28))
    svg=fitz.open(FIG/name)
    rendered=fitz.open("pdf",svg.convert_to_pdf())
    page.show_pdf_page(fitz.Rect(25,100,570,730),rendered,0,keep_proportion=True)

def numbered_save(doc,path):
    for i,page in enumerate(doc):
        page.insert_text((48 if page.rect.width < 900 else 850,page.rect.height-25),f"{i+1}/{len(doc)}",
                         fontsize=8,color=(.3,.42,.48))
    path.parent.mkdir(parents=True,exist_ok=True)
    doc.save(path,garbage=4,deflate=True)
    print(path.relative_to(ROOT),len(doc),"pages")

def report_pdf():
    doc=fitz.open()
    text=(ROOT/"REPORT.md").read_text(encoding="utf-8")
    parts=re.split(r"(?=^## \d+\.)",text,flags=re.M)
    intro=parts.pop(0)
    for index,part in enumerate(parts):
        if index==0: part=intro+"\n"+part
        for chunk in part.split("<!-- page -->"):
            add_markdown_page(doc,chunk,"PROJECT 7 - TEAM REPORT")
        if part.startswith("## 11."):
            add_svg_page(doc,"full_scale_time.svg","Figure 1 - Full-scale runtime and spread")
            add_svg_page(doc,"full_scale_counters.svg","Figure 2 - Full-scale counted work")
            add_svg_page(doc,"predicted_vs_measured.svg","Figure 3 - Calibrated model and measurements")
    numbered_save(doc,ROOT/"REPORT.pdf")

def d5_pdf():
    doc=fitz.open()
    for part in (ROOT/"docs/derivations/D5.md").read_text(encoding="utf-8").split("<!-- page -->"):
        add_markdown_page(doc,part,"ANAGH SINGLA | 250001006 | D5",
                          CSS.replace("10.4pt","9.8pt").replace("1.33","1.25"))
    assert len(doc)==2
    numbered_save(doc,ROOT/"docs/derivations/D5_250001006.pdf")

def slides_pdf():
    slides=json.loads((ROOT/"docs/slides/slides.json").read_text(encoding="utf-8"))
    doc=fitz.open()
    for s in slides:
        page=doc.new_page(width=960,height=540)
        page.draw_rect(page.rect,color=None,fill=(.96,.98,.985))
        page.draw_rect(fitz.Rect(0,0,16,540),color=None,fill=(.03,.5,.55))
        page.insert_text((50,45),s["subtitle"],fontsize=12,color=(.02,.44,.5))
        page.insert_textbox(fitz.Rect(50,65,915,150),s["title"],fontsize=30,
                            fontname="hebo",color=(.07,.2,.28))
        y=155
        if s.get("diagram")=="tree":
            boxes=[(410,150,550,195,"root: own tag"),(210,225,350,270,"left: older tag"),(610,225,750,270,"right: older tag")]
            for x1,y1,x2,y2,label in boxes:
                page.draw_rect(fitz.Rect(x1,y1,x2,y2),color=(.03,.5,.55),fill=(.9,.95,.96))
                page.insert_text((x1+8,y1+27),label,fontsize=15)
            page.draw_line((480,195),(280,225),color=(.03,.5,.55))
            page.draw_line((480,195),(680,225),color=(.03,.5,.55))
            y=300
        if s.get("diagram")=="blocks":
            for i in range(6):
                x=70+i*135
                page.draw_rect(fitz.Rect(x,160,x+120,225),color=(.03,.5,.55),
                               fill=(.95,.78,.6) if i in [0,5] else (.83,.94,.93))
                page.insert_text((x+14,198),"partial" if i in [0,5] else "full tag",fontsize=17)
            y=265
        if s.get("figure"):
            svg=fitz.open(FIG/s["figure"])
            pdf=fitz.open("pdf",svg.convert_to_pdf())
            page.show_pdf_page(fitz.Rect(55,135,905,420),pdf,0)
            y=432
        size=14 if s.get("figure") else (17 if s.get("diagram") else 20)
        gap=24 if size==14 else (31 if size==17 else 48)
        for line in s["body"]:
            rect=fitz.Rect(55,y,915,y+gap)
            spare=page.insert_textbox(rect,"- "+line,fontsize=size,color=(.12,.2,.28))
            if spare<0:
                raise RuntimeError("Slide line does not fit: "+line)
            y+=gap
        page.insert_text((55,515),s.get("footer","Source: tracked project code, raw CSV and docs/VALIDATION.md"),fontsize=9,color=(.3,.42,.48))
    numbered_save(doc,ROOT/"docs/slides/slides.pdf")

def main():
    svg_plots()
    render(ROOT/"results/raw/full_scale_width_sweep.csv", FIG)
    report_pdf()
    d5_pdf()
    slides_pdf()

if __name__=="__main__":
    main()


