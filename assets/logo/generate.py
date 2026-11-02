#!/usr/bin/env python3
# Copyright (c) 2026, kyka-zavr.
"""Generates the Infinity-hyprland logo concepts (SVG): black and white, no gradients. Text is converted to outlines,
so the files need no fonts.

    python3 assets/logo/generate.py        # writes the .svg files next to this script (needs fonttools, Adwaita Sans)
"""
import math
import os
import sys

from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf"

# two schemes: a dark plate with a white mark, and a light plate with a black mark
DARK = {"name": "", "plate": "#0b0b0c", "fg": "#f6f6f6", "dim": "#8c8c8c", "faint": "#3a3a3c"}
LIGHT = {"name": "-light", "plate": "#f6f6f6", "fg": "#0b0b0c", "dim": "#76767a", "faint": "#c4c4c8"}


# ---------------------------------------------------------------- geometry
def lemniscate(cx, cy, a, n=720, tall=1.7):
    """points of a lemniscate (the infinity sign), width 2a, rounded lobes"""
    pts = []
    for i in range(n):
        t = 2 * math.pi * i / n
        d = 1 + math.sin(t) ** 2
        pts.append((cx + a * math.cos(t) / d, cy + a * math.sin(t) * math.cos(t) / d * tall))
    return pts


def path_from_points(pts):
    """a smooth closed path through the points (Catmull-Rom to cubic Bezier)"""
    step = max(1, len(pts) // 60)
    p = pts[::step]
    m = len(p)
    d = f"M{p[0][0]:.2f},{p[0][1]:.2f}"
    for i in range(m):
        p0, p1, p2, p3 = p[(i - 1) % m], p[i], p[(i + 1) % m], p[(i + 2) % m]
        c1 = (p1[0] + (p2[0] - p0[0]) / 6, p1[1] + (p2[1] - p0[1]) / 6)
        c2 = (p2[0] - (p3[0] - p1[0]) / 6, p2[1] - (p3[1] - p1[1]) / 6)
        d += f" C{c1[0]:.2f},{c1[1]:.2f} {c2[0]:.2f},{c2[1]:.2f} {p2[0]:.2f},{p2[1]:.2f}"
    return d + " Z"


_FONTS = {}


def _font(weight):
    if weight not in _FONTS:
        f = TTFont(FONT)
        _FONTS[weight] = instancer.instantiateVariableFont(f, {"wght": weight, "opsz": 32}, inplace=False)
    return _FONTS[weight]


def text_path(text, weight, size, x, y, spacing=0.0):
    """outlines of a string: (svg path data, width). y is the baseline."""
    font = _font(weight)
    gs, cmap, upm = font.getGlyphSet(), font.getBestCmap(), font["head"].unitsPerEm
    scale = size / upm
    out, cursor = [], x
    for ch in text:
        name = cmap.get(ord(ch))
        if name is None:
            cursor += size * 0.5
            continue
        pen = SVGPathPen(gs)
        gs[name].draw(TransformPen(pen, (scale, 0, 0, -scale, cursor, y)))
        out.append(pen.getCommands())
        cursor += gs[name].width * scale + spacing
    return " ".join(c for c in out if c), cursor - x - spacing


def svg(w, h, body):
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}">\n{body}\n</svg>\n'


# ---------------------------------------------------------------- the five marks (256 x 256, drawn on the plate)
def mark1(c):
    """a bold lemniscate with a window sitting at the crossing"""
    d = path_from_points(lemniscate(128, 128, 98, tall=1.6))
    return (f'<path d="{d}" fill="none" stroke="{c["fg"]}" stroke-width="21" stroke-linejoin="round" stroke-linecap="round"/>'
            f'<rect x="107" y="107" width="42" height="42" rx="14" fill="{c["plate"]}" stroke="{c["fg"]}" stroke-width="8"/>'
            f'<path d="M118,122 H138" stroke="{c["fg"]}" stroke-width="6" stroke-linecap="round"/>')


def mark2(c):
    """a viewfinder around a small infinity sign: the screen is a view over an unbounded plane"""
    d = path_from_points(lemniscate(128, 128, 64, tall=1.6))
    L, r, o = 56, 26, 34
    k = (f'<path d="M{o},{o+L} V{o+r} Q{o},{o} {o+r},{o} H{o+L}"/>'
         f'<path d="M{256-o-L},{o} H{256-o-r} Q{256-o},{o} {256-o},{o+r} V{o+L}"/>'
         f'<path d="M{256-o},{256-o-L} V{256-o-r} Q{256-o},{256-o} {256-o-r},{256-o} H{256-o-L}"/>'
         f'<path d="M{o+L},{256-o} H{o+r} Q{o},{256-o} {o},{256-o-r} V{256-o-L}"/>')
    return (f'<g fill="none" stroke="{c["dim"]}" stroke-width="13" stroke-linecap="round" stroke-linejoin="round">{k}</g>'
            f'<path d="{d}" fill="none" stroke="{c["fg"]}" stroke-width="16" stroke-linejoin="round" stroke-linecap="round"/>')


def mark3(c):
    """two window frames overlapping, linked into an infinity shape"""
    s, y = 112, 72
    f = lambda x: f'<rect x="{x}" y="{y}" width="{s}" height="{s}" rx="40" fill="none" stroke="{c["fg"]}" stroke-width="15"/>'
    return f(22) + f(122)


def mark4(c):
    """a fading dot grid with floating windows: the plane itself"""
    dots = []
    for gy in range(11):
        for gx in range(11):
            x, y = 28 + gx * 20, 28 + gy * 20
            dist = math.hypot(x - 128, y - 128) / 128
            if dist < 1.0:
                dots.append(f'<circle cx="{x}" cy="{y}" r="{3.0 * (1.1 - dist * 0.7):.2f}" fill="{c["dim"]}" opacity="{max(0.0, 1.0 - dist):.2f}"/>')
    wins = (f'<rect x="34" y="50" width="70" height="54" rx="16" fill="{c["plate"]}" stroke="{c["faint"]}" stroke-width="7"/>'
            f'<rect x="148" y="136" width="86" height="66" rx="18" fill="{c["plate"]}" stroke="{c["dim"]}" stroke-width="8"/>'
            f'<rect x="76" y="82" width="108" height="86" rx="22" fill="{c["plate"]}" stroke="{c["fg"]}" stroke-width="11"/>'
            f'<path d="M98,108 H162" stroke="{c["fg"]}" stroke-width="8" stroke-linecap="round"/>')
    return "".join(dots) + wins


def mark5(c):
    """tiles that line up into an infinity sign: tiling and the infinite canvas in one"""
    pts = lemniscate(128, 128, 98, n=1600)
    acc, lens = 0.0, [0.0]
    for i in range(1, len(pts) + 1):
        a, b = pts[i - 1], pts[i % len(pts)]
        acc += math.hypot(b[0] - a[0], b[1] - a[1])
        lens.append(acc)
    n, tiles = 28, []
    for k in range(n):
        target = acc * (k + 0.5) / n
        i = next(j for j in range(len(lens) - 1) if lens[j + 1] >= target)
        p, q = pts[i], pts[(i + 1) % len(pts)]
        ang = math.degrees(math.atan2(q[1] - p[1], q[0] - p[0]))
        t = abs(p[0] - 128) / 98                 # 0 at the crossing, 1 at the far ends
        s_ = 9 + 16 * t ** 0.7
        tiles.append(f'<rect x="{-s_/2:.2f}" y="{-s_/2:.2f}" width="{s_:.2f}" height="{s_:.2f}" rx="{s_ * 0.36:.1f}" fill="{c["fg"]}" '
                     f'transform="translate({p[0]:.2f},{p[1]:.2f}) rotate({ang:.1f})"/>')
    return "".join(tiles)


MARKS = [("01-lemniscate", "Lemniscate with a window", mark1), ("02-viewport", "Viewport", mark2), ("03-linked-windows", "Linked windows", mark3),
         ("04-infinite-grid", "Infinite grid", mark4), ("05-tiles", "Tiles to infinity", mark5)]


# ---------------------------------------------------------------- icon (rounded plate) and horizontal logo
def icon(mark, c):
    return svg(256, 256, f'<rect width="256" height="256" rx="70" fill="{c["plate"]}"/>\n{mark(c)}')


def logo(mark, c):
    W, H = 780, 256
    d1, _ = text_path("infinity", 760, 98, 286, 142, spacing=-1)
    d2, _ = text_path("hyprland", 420, 38, 291, 198, spacing=12)
    text = f'<path d="{d1}" fill="{c["fg"]}"/><path d="{d2}" fill="{c["dim"]}"/>'
    plate = f'<rect width="{W}" height="{H}" rx="64" fill="{c["plate"]}"/>'
    return svg(W, H, f'{plate}\n<g transform="translate(14,0)">{mark(c)}</g>\n{text}')


def main():
    for slug, _title, fn in MARKS:
        for scheme in (DARK, LIGHT):
            for kind, build in (("icon", icon), ("logo", logo)):
                with open(os.path.join(HERE, f"{slug}-{kind}{scheme['name']}.svg"), "w") as f:
                    f.write(build(fn, scheme))
    print("written:", len(MARKS) * 4, "files into", HERE)


if __name__ == "__main__":
    sys.exit(main())
