#!/usr/bin/env python3
"""Generate the ten panel SVGs in res/, a dark and a light one per module.

Everything here is in millimetres, because that is what Rack's panel guide asks
for: 128.5 mm tall, a whole number of 5.08 mm HP wide, viewBox in the same
units. Label text is emitted as outlines because nanosvg (what Rack renders
with) drops <text> on the floor.

Geometry, all in mm, for a 10 HP panel 50.8 wide:

  parameter row    knob centre 11.8, attenuverter 25.4, CV jack 39.0
                   knob r 4.80, attenuverter r 3.025, jack r 4.015
                   so the gaps between widget edges are 5.78 and 6.57, and the
                   side margins are 7.00 and 7.79
  row label        left aligned at x 7.00, baseline 7.1 above the row centre,
                   which puts its bottom 2.3 clear of the top of the knob
  row pitch        17 where a module also needs a switch, 19 otherwise
  audio row        four jacks at 8.315, 19.705, 31.095, 42.485, pitch 11.39,
                   gap 3.36 between jack edges, margins 4.30
  bottom           jacks at y 113 end at 117.01, the L/R marks sit on a 120.71
                   baseline, the wordmark on 126.4, panel ends at 128.5

No screws, so the corners are free and rows can start at y 24.

Needs fonttools and a copy of DejaVu Sans. Run it from the repo root:

    python3 scripts/genpanels.py
"""

import os
import sys

from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.boundsPen import BoundsPen

HP = 5.08
HEIGHT = 128.5

# Contrast against each ground was checked rather than eyeballed. Nothing a
# user has to read sits below 4.5:1; the secondary colour is for tick marks and
# the wordmark only.
THEMES = {
    "dark": {
        "bg": "#101010",
        "title": "#b4b4b4",
        "label": "#9a9a9a",
        "faint": "#6f6f6f",
        "rule": "#3a3a3a",
    },
    "light": {
        "bg": "#edeae3",
        "title": "#2e2e2e",
        "label": "#3a3a3a",
        "faint": "#5f5f5f",
        "rule": "#c6c1b6",
    },
}

TITLE_CAP = 2.8
ROW_CAP = 2.0
PORT_CAP = 1.8
MARK_CAP = 1.4
FOOT_CAP = 1.55
TRACK = 0.09  # letter spacing as a fraction of the em

# Widget footprints. The knobs, jacks and switch are drawn in res/ by hand; the
# numbers here have to agree with those files.
KNOB_R = 4.800
TRIM_R = 3.025
PORT_R = 4.015
SW_W, SW_H = 4.600, 9.600

KNOB_X, TRIM_X, CV_X = 11.8, 25.4, 39.0
LABEL_X = 7.0

FONT_DIRS = [
    r"C:\Windows\Fonts",
    "/usr/share/fonts/truetype/dejavu",
    "/Library/Fonts",
]


def find_font(name):
    for d in FONT_DIRS:
        p = os.path.join(d, name)
        if os.path.exists(p):
            return p
    sys.exit("could not find %s; edit FONT_DIRS" % name)


class Face:
    def __init__(self, filename):
        self.font = TTFont(find_font(filename))
        self.glyphs = self.font.getGlyphSet()
        self.cmap = self.font.getBestCmap()
        self.upem = self.font["head"].unitsPerEm
        bp = BoundsPen(self.glyphs)
        self.glyphs[self.cmap[ord("H")]].draw(bp)
        self.cap = bp.bounds[3]

    def size_for_cap(self, cap_mm):
        return cap_mm * self.upem / self.cap

    def advance(self, text, size):
        w = sum(self.glyphs[self.cmap[ord(c)]].width for c in text)
        return w * size / self.upem + TRACK * size * max(0, len(text) - 1)

    def outline(self, text, size, x, y):
        pen = SVGPathPen(self.glyphs, ntos=lambda v: "%.3f" % v)
        s = size / self.upem
        for ch in text:
            g = self.glyphs[self.cmap[ord(ch)]]
            g.draw(TransformPen(pen, (s, 0, 0, -s, x, y)))
            x += g.width * s + TRACK * size
        return pen.getCommands()


BOLD = None
REG = None


def text(out, face, s, cap, x, y, fill, anchor="middle", maxw=None):
    size = face.size_for_cap(cap)
    w = face.advance(s, size)
    if maxw and w > maxw:
        size *= maxw / w
        w = maxw
    if anchor == "middle":
        x -= w / 2
    elif anchor == "end":
        x -= w
    out.append('  <path d="%s" fill="%s"/>' % (face.outline(s, size, x, y), fill))
    return w


def io_positions(w):
    """Four jacks across the bottom, evenly spaced."""
    margin = 4.30
    pitch = (w - 2 * margin - 2 * PORT_R) / 3.0
    return [margin + PORT_R + i * pitch for i in range(4)]


def panel(spec, theme_name):
    theme = THEMES[theme_name]
    w = spec["hp"] * HP
    out = ['<?xml version="1.0" encoding="UTF-8"?>']
    out.append(
        '<svg xmlns="http://www.w3.org/2000/svg" '
        'xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" '
        'width="%gmm" height="%gmm" viewBox="0 0 %g %g" version="1.1">' % (w, HEIGHT, w, HEIGHT)
    )
    out.append('  <rect width="%g" height="%g" fill="%s"/>' % (w, HEIGHT, theme["bg"]))

    text(out, BOLD, spec["title"], TITLE_CAP, w / 2, 10.0, theme["title"], maxw=w - 6)

    for y in spec.get("rules", []):
        out.append('  <rect x="6" y="%.3f" width="%.3f" height="0.28" fill="%s"/>'
                   % (y, w - 12, theme["rule"]))

    first = True
    for y, label in spec["knobrows"]:
        text(out, BOLD, label, ROW_CAP, LABEL_X, y - KNOB_R - 2.3, theme["label"], anchor="start")
        if first:
            text(out, BOLD, "ATT", 1.5, TRIM_X, y - KNOB_R - 2.3, theme["faint"])
            text(out, BOLD, "CV", 1.5, CV_X, y - KNOB_R - 2.3, theme["faint"])
            first = False

    if "switch" in spec:
        y, label, marks = spec["switch"]
        text(out, BOLD, label, ROW_CAP, LABEL_X, y - SW_H / 2 - 2.3, theme["label"], anchor="start")
        for i, m in enumerate(marks):
            text(out, BOLD, m, MARK_CAP, KNOB_X + SW_W / 2 + 1.7, y - 2.4 + 2.4 * i + 0.7,
                 theme["faint"], anchor="start")

    for x, y, label, kind in spec.get("ports", []):
        # A row label starting at the left margin reads the same as the knob
        # rows do; a centred one belongs to the jack it sits over.
        if kind == "rowin":
            text(out, BOLD, label, ROW_CAP, LABEL_X, y - PORT_R - 2.3, theme["label"], anchor="start")
        else:
            text(out, BOLD, label, PORT_CAP, x, y - PORT_R - 2.3, theme["label"])

    xs = io_positions(w)
    ioy = spec["io"]["y"]
    for i, label in enumerate(spec["io"]["groups"]):
        cx = (xs[2 * i] + xs[2 * i + 1]) / 2.0
        text(out, BOLD, label, PORT_CAP, cx, ioy - PORT_R - 2.3, theme["label"])
    for i, x in enumerate(xs):
        text(out, BOLD, "LR"[i % 2], MARK_CAP, x, ioy + PORT_R + 3.7, theme["faint"])

    text(out, REG, "VERIDICAL", FOOT_CAP, w / 2, 126.4, theme["faint"])

    out.append('  <g inkscape:groupmode="layer" inkscape:label="components" style="display:none">')

    def mark(x, y, colour, r=1.0):
        out.append('    <circle cx="%.3f" cy="%.3f" r="%g" fill="%s"/>' % (x, y, r, colour))

    for y, _ in spec["knobrows"]:
        mark(KNOB_X, y, "#ff0000")
        mark(TRIM_X, y, "#ff0000")
        mark(CV_X, y, "#00ff00")
    if "switch" in spec:
        mark(KNOB_X, spec["switch"][0], "#ff0000")
    for x, y, _, kind in spec.get("ports", []):
        mark(x, y, "#00ff00")
    for i, x in enumerate(xs):
        mark(x, ioy, "#0000ff" if spec["io"]["out"][i // 2] else "#00ff00")
    for x, y in spec.get("lights", []):
        mark(x, y, "#ff00ff", 0.5)
    out.append("  </g>")
    out.append("</svg>")
    return "\n".join(out) + "\n"


SPECS = [
    {
        "slug": "Flanger",
        "title": "FLANGER",
        "hp": 10,
        "knobrows": [(24, "RATE"), (46, "DEPTH"), (68, "FEEDBACK"), (90, "MIX")],
        "rules": [101.0],
        "io": {"y": 113, "groups": ["IN", "OUT"], "out": [False, True]},
    },
    {
        "slug": "Chorus",
        "title": "CHORUS",
        "hp": 10,
        "knobrows": [(24, "RATE"), (46, "DEPTH"), (68, "MIX")],
        "switch": (90, "VOICES", ["4", "3", "2"]),
        "rules": [101.0],
        "io": {"y": 113, "groups": ["IN", "OUT"], "out": [False, True]},
    },
    {
        "slug": "Phaser",
        "title": "PHASER",
        "hp": 10,
        "knobrows": [(24, "RATE"), (41, "DEPTH"), (58, "FEEDBACK"), (75, "MIX")],
        "switch": (92, "STAGES", ["8", "6", "4"]),
        "rules": [101.0],
        "io": {"y": 113, "groups": ["IN", "OUT"], "out": [False, True]},
    },
    {
        "slug": "RingMod",
        "title": "RING MOD",
        "hp": 10,
        "knobrows": [(26, "FREQ"), (56, "MIX")],
        "ports": [(CV_X, 86, "CARRIER", "rowin")],
        "rules": [101.0],
        "io": {"y": 113, "groups": ["IN", "OUT"], "out": [False, True]},
    },
    {
        "slug": "EnvFollower",
        "title": "ENV FOLLOWER",
        "hp": 10,
        "knobrows": [(24, "ATTACK"), (46, "RELEASE"), (68, "GAIN")],
        "ports": [(19.705, 91, "IN L", "in"), (31.095, 91, "IN R", "in")],
        "lights": [(14.01, 101.0)],
        "rules": [79.0],
        "io": {"y": 113, "groups": ["ENV", "GATE"], "out": [True, True]},
    },
]


def main():
    global BOLD, REG
    BOLD = Face("DejaVuSans-Bold.ttf")
    REG = Face("DejaVuSans.ttf")
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
    res = os.path.join(root, "res")
    for spec in SPECS:
        for theme in THEMES:
            name = "%s-%s.svg" % (spec["slug"], theme)
            with open(os.path.join(res, name), "w") as f:
                f.write(panel(spec, theme))
        print("%-14s %5.2f x %.1f mm" % (spec["slug"], spec["hp"] * HP, HEIGHT))


if __name__ == "__main__":
    main()
