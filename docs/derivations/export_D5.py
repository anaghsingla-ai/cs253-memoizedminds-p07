"""Export the D5 Markdown proof. Requires: python -m pip install reportlab."""
from pathlib import Path
from html import escape
import re

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, PageBreak
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont


def main():
    folder = Path(__file__).resolve().parent
    output = folder / "D5_250001006.pdf"
    font = "Helvetica"
    bold = "Helvetica-Bold"
    fonts = Path("/usr/share/fonts/truetype/dejavu")
    if (fonts / "DejaVuSans.ttf").exists():
        pdfmetrics.registerFont(TTFont("D5Sans", str(fonts / "DejaVuSans.ttf")))
        pdfmetrics.registerFont(TTFont("D5Bold", str(fonts / "DejaVuSans-Bold.ttf")))
        font, bold = "D5Sans", "D5Bold"
    body = ParagraphStyle("body", fontName=font, fontSize=10,
                          leading=14, spaceAfter=7, alignment=TA_LEFT)
    heading = ParagraphStyle("heading", parent=body, fontName=bold,
                             fontSize=11, leading=14, spaceBefore=8, keepWithNext=True)
    title = ParagraphStyle("title", parent=heading, fontSize=17, leading=21)
    story = []
    for block in (folder / "D5.md").read_text().strip().split("\n\n"):
        if block.startswith("## Where this is used"):
            story.append(PageBreak())
        text = " ".join(block.splitlines())
        style = body
        if text.startswith("# "):
            text, style = text[2:], title
        elif text.startswith("## "):
            text, style = text[3:], heading
        text = escape(text)
        text = re.sub(r"`([^`]+)`", r'<font name="Courier">\1</font>', text)
        story.append(Paragraph(text, style))
    story.append(Spacer(1, 4))

    def footer(canvas, doc):
        canvas.setFont("Helvetica", 8)
        canvas.setFillColor(colors.HexColor("#444444"))
        canvas.drawString(42, 25, "Anagh Singla | 250001006 | Project 7 | D5")
        canvas.drawRightString(A4[0] - 42, 25, str(doc.page))

    SimpleDocTemplate(str(output), pagesize=A4, rightMargin=42, leftMargin=42,
                      topMargin=35, bottomMargin=40,
                      title="D5 - Maximum lazy push-downs",
                      author="Anagh Singla, 250001006").build(
                          story, onFirstPage=footer, onLaterPages=footer)
    print(output)


if __name__ == "__main__":
    main()
