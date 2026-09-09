#!/usr/bin/env python3
"""Composite the widget artwork onto each panel and write the result to
build/preview/, so the layout can be checked without launching Rack.

    python3 scripts/preview.py
    inkscape --export-type=png --export-dpi=150 build/preview/Flanger-dark.svg
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import genpanels as g

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
RES = os.path.join(ROOT, "res")
OUT = os.path.join(ROOT, "build", "preview")


def inner(name):
    with open(os.path.join(RES, name)) as f:
        return re.search(r"<svg[^>]*>(.*)</svg>", f.read(), re.S).group(1)


def place(body, cx, cy, w, h):
    return '<g transform="translate(%.3f,%.3f)">%s</g>' % (cx - w / 2, cy - h / 2, body)


def main():
    g.BOLD = g.Face("DejaVuSans-Bold.ttf")
    g.REG = g.Face("DejaVuSans.ttf")
    knob, trim, port = inner("Knob.svg"), inner("KnobSmall.svg"), inner("Port.svg")
    switch = inner("Switch3_1.svg")
    if not os.path.isdir(OUT):
        os.makedirs(OUT)

    for spec in g.SPECS:
        w = spec["hp"] * g.HP
        parts = []
        for y, _ in spec["knobrows"]:
            parts.append(place(knob, g.KNOB_X, y, 9.6, 9.6))
            parts.append(place(trim, g.TRIM_X, y, 6.05, 6.05))
            parts.append(place(port, g.CV_X, y, 8.03, 8.03))
        if "switch" in spec:
            parts.append(place(switch, g.KNOB_X, spec["switch"][0], 4.6, 9.6))
        for x, y, _, _ in spec.get("ports", []):
            parts.append(place(port, x, y, 8.03, 8.03))
        for x in g.io_positions(w):
            parts.append(place(port, x, spec["io"]["y"], 8.03, 8.03))
        for x, y in spec.get("lights", []):
            parts.append('<circle cx="%.3f" cy="%.3f" r="1.35" fill="#5ad24a"/>' % (x, y))

        for theme in g.THEMES:
            svg = g.panel(spec, theme).replace("</svg>", "\n".join(parts) + "\n</svg>")
            with open(os.path.join(OUT, "%s-%s.svg" % (spec["slug"], theme)), "w") as f:
                f.write(svg)
    print("wrote %s" % OUT)


if __name__ == "__main__":
    main()
