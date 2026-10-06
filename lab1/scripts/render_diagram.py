"""Рендер диаграммы деятельности: dot -> SVG -> правка фигур send/receive -> PNG.

Graphviz не умеет UML-фигуры «передача сигнала» (пятиугольник-стрелка)
и «приём события» (прямоугольник с вырезом слева), поэтому такие узлы
рисуются как box с class="send"/"recv", а затем их контур заменяется.
"""
import re
import subprocess
import sys
from pathlib import Path

DIAGRAMS = Path(__file__).resolve().parent.parent / "diagrams"
NOTCH = 12.0  # глубина выреза/острия, pt


def reshape(svg: str) -> str:
    def fix_node(m: re.Match) -> str:
        kind, body = m.group(1), m.group(2)

        def fix_poly(pm: re.Match) -> str:
            pts = [tuple(map(float, p.split(","))) for p in pm.group(2).split()]
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
            ym = (y0 + y1) / 2
            if kind == "send":
                new = [(x0, y0), (x1 - NOTCH, y0), (x1, ym), (x1 - NOTCH, y1), (x0, y1)]
            else:
                new = [(x0, y0), (x1, y0), (x1, y1), (x0, y1), (x0 + NOTCH, ym)]
            new.append(new[0])
            return pm.group(1) + " ".join(f"{x:.2f},{y:.2f}" for x, y in new) + pm.group(3)

        body = re.sub(r'(<polygon[^>]*points=")([^"]+)(")', fix_poly, body, count=1)
        return m.group(0)[: m.start(2) - m.start(0)] + body + "</g>"

    return re.sub(r'<g id="[^"]*" class="node (send|recv)">(.*?)</g>', fix_node, svg, flags=re.S)


def main() -> None:
    name = sys.argv[1] if len(sys.argv) > 1 else "activity_diagram"
    dot = DIAGRAMS / f"{name}.dot"
    svg_path = DIAGRAMS / f"{name}.svg"
    png_path = DIAGRAMS / f"{name}.png"
    svg = subprocess.run(["dot", "-Tsvg", str(dot)], check=True, capture_output=True, text=True).stdout
    svg_path.write_text(reshape(svg), encoding="utf-8")
    subprocess.run(["rsvg-convert", "-z", "2", "-b", "white", "-o", str(png_path), str(svg_path)], check=True)
    print(png_path)


if __name__ == "__main__":
    main()
