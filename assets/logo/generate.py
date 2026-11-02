#!/usr/bin/env python3
# Copyright (c) 2026, kyka-zavr.
"""Generates the Infinity-hyprland logo concepts (SVG). Text is converted to outlines, so the files need no fonts.

    python3 assets/logo/generate.py            # writes the .svg files next to this script
"""
import math
import os
import sys

from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = "/usr/share/fonts/Adwaita"

# palette (the default dark theme of the settings)
BG = "#121217"
SURFACE = "#1f1f26"
TEXT = "#f4f5fa"
SUB = "#a9afc2"
ACCENT = "#5c8fff"
ACCENT2 = "#3fd6c5"


# ---------------------------------------------------------------- geometry
def lemniscate(cx, cy, a, n=720):
    """points of a Bernoulli lemniscate (the infinity sign), width 2a"""
    pts = []
    for i in range(n):
        t = 2 * math.pi * i / n
        d = 1 + math.sin(t) ** 2
        pts.append((cx + a * math.cos(t) / d * 1.0, cy + a * math.sin(t) * math.cos(t) / d * 1.55))
    return pts


def path_from_points(pts, close=True):
    """a smooth closed path through the points (Catmull-Rom to cubic Bezier)"""
    n = len(pts)
    step = max(1, n // 48)
    p = pts[::step]
    m = len(p)
    d = f"M{p[0][0]:.2f},{p[0][1]:.2f}"
    for i in range(m):
        p0, p1, p2, p3 = p[(i - 1) % m], p[i], p[(i + 1) % m], p[(i + 2) % m]
        c1 = (p1[0] + (p2[0] - p0[0]) / 6, p1[1] + (p2[1] - p0[1]) / 6)
        c2 = (p2[0] - (p3[0] - p1[0]) / 6, p2[1] - (p3[1] - p1[1]) / 6)
        d += f" C{c1[0]:.2f},{c1[1]:.2f} {c2[0]:.2f},{c2[1]:.2f} {p2[0]:.2f},{p2[1]:.2f}"
    return d + (" Z" if close else "")


def text_path(text, font_file, size, x, y, spacing=0.0):
    """outlines of a string: (svg path data, width). y is the baseline."""
    font = TTFont(os.path.join(FONTS, font_file))
    gs, cmap, upm = font.getGlyphSet(), font.getBestCmap(), font["head"].unitsPerEm
    scale = size / upm
    out, cursor = [], x
    for ch in text:
        name = cmap.get(ord(ch))
        if name is None:
            cursor += size * 0.6
            continue
        pen = SVGPathPen(gs)
        gs[name].draw(TransformPen(pen, (scale, 0, 0, -scale, cursor, y)))
        out.append(pen.getCommands())
        cursor += gs[name].width * scale + spacing
    return " ".join(c for c in out if c), cursor - x - spacing


def svg(w, h, body, defs=""):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}">\n'
            f'<defs>{defs}</defs>\n{body}\n</svg>\n')


def mix(a, b, f):
    """a color between two #rrggbb colors"""
    ca, cb = [int(a[i:i + 2], 16) for i in (1, 3, 5)], [int(b[i:i + 2], 16) for i in (1, 3, 5)]
    return "#" + "".join(f"{round(x + (y - x) * f):02x}" for x, y in zip(ca, cb))


GRAD = (f'<linearGradient id="g" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="{ACCENT}"/>'
        f'<stop offset="1" stop-color="{ACCENT2}"/></linearGradient>')


# ---------------------------------------------------------------- the five marks (256 x 256, no background)
def mark1():
    """a bold lemniscate with a window sitting at the crossing"""
    d = path_from_points(lemniscate(128, 128, 96))
    win = (f'<rect x="104" y="104" width="48" height="48" rx="13" fill="{BG}" stroke="{TEXT}" stroke-width="9"/>'
           f'<path d="M115,121 H141" stroke="{TEXT}" stroke-width="6" stroke-linecap="round"/>')
    return (f'<path d="{d}" fill="none" stroke="url(#g)" stroke-width="22" stroke-linejoin="round" stroke-linecap="round"/>' + win, GRAD)


def mark2():
    """a viewfinder (the screen) over a smaller infinity: a view over an infinite plane"""
    d = path_from_points(lemniscate(128, 128, 64))
    c, L, r, o = TEXT, 58, 16, 30
    k = (f'<path d="M{o},{o+L} V{o+r} Q{o},{o} {o+r},{o} H{o+L}" />'
         f'<path d="M{256-o-L},{o} H{256-o-r} Q{256-o},{o} {256-o},{o+r} V{o+L}" />'
         f'<path d="M{256-o},{256-o-L} V{256-o-r} Q{256-o},{256-o} {256-o-r},{256-o} H{256-o-L}" />'
         f'<path d="M{o+L},{256-o} H{o+r} Q{o},{256-o} {o},{256-o-r} V{256-o-L}" />')
    return (f'<g fill="none" stroke="{c}" stroke-width="12" stroke-linecap="round" stroke-linejoin="round">{k}</g>'
            f'<path d="{d}" fill="none" stroke="url(#g)" stroke-width="15" stroke-linejoin="round" stroke-linecap="round"/>', GRAD)


def mark3():
    """two linked window frames"""
    def frame(x, y, s, col):
        return (f'<rect x="{x}" y="{y}" width="{s}" height="{s}" rx="30" fill="none" stroke="{col}" stroke-width="16"/>'
                f'<path d="M{x+20},{y+40} H{x+s-20}" stroke="{col}" stroke-width="9" stroke-linecap="round"/>')
    body = (f'<g>{frame(22, 72, 112, "url(#g)")}</g>'
            f'<g>{frame(122, 72, 112, "url(#g2)")}</g>'
            # the overlap drawn once more on top of the left frame so the two look interlocked
            f'<clipPath id="c"><rect x="122" y="72" width="12" height="112"/></clipPath>'
            f'<g clip-path="url(#c)">{frame(22, 72, 112, "url(#g)")}</g>')
    defs = (f'<linearGradient id="g" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="{ACCENT}"/><stop offset="1" stop-color="{ACCENT2}"/></linearGradient>'
            f'<linearGradient id="g2" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="{ACCENT2}"/><stop offset="1" stop-color="{ACCENT}"/></linearGradient>')
    return body, defs


def mark4():
    """a fading dot grid with floating windows: the infinite plane"""
    dots = []
    for gy in range(11):
        for gx in range(11):
            x, y = 28 + gx * 20, 28 + gy * 20
            dist = math.hypot(x - 128, y - 128) / 128
            if dist < 1.0:
                dots.append(f'<circle cx="{x}" cy="{y}" r="{2.6 * (1.1 - dist * 0.7):.2f}" fill="{SUB}" opacity="{max(0.0, 0.95 - dist):.2f}"/>')
    win = (f'<rect x="78" y="86" width="104" height="80" rx="14" fill="{SURFACE}" stroke="url(#g)" stroke-width="9"/>'
           f'<path d="M96,108 H164" stroke="url(#g)" stroke-width="7" stroke-linecap="round"/>'
           f'<rect x="146" y="132" width="84" height="64" rx="12" fill="{BG}" stroke="{TEXT}" stroke-width="6" opacity="0.55"/>'
           f'<rect x="30" y="52" width="66" height="50" rx="10" fill="{BG}" stroke="{TEXT}" stroke-width="5" opacity="0.3"/>')
    return "".join(dots) + win, GRAD


def mark5():
    """tiles that line up into an infinity sign: tiling and the infinite canvas in one"""
    pts = lemniscate(128, 128, 98, n=1600)
    acc, lens = 0.0, [0.0]
    for i in range(1, len(pts) + 1):
        a, b = pts[i - 1], pts[i % len(pts)]
        acc += math.hypot(b[0] - a[0], b[1] - a[1])
        lens.append(acc)
    n = 26
    tiles = []
    for k in range(n):
        target = acc * (k + 0.5) / n
        i = next(j for j in range(len(lens) - 1) if lens[j + 1] >= target)
        p, q = pts[i], pts[(i + 1) % len(pts)]
        ang = math.degrees(math.atan2(q[1] - p[1], q[0] - p[0]))
        t = abs(p[0] - 128) / 98                   # 0 at the crossing, 1 at the far ends
        s_ = 9 + 19 * t ** 0.8
        f = max(0.0, min(1.0, (p[0] - 30) / 196))   # color runs from left to right
        col = mix(ACCENT, ACCENT2, f)
        tiles.append(f'<rect x="{-s_/2:.2f}" y="{-s_/2:.2f}" width="{s_:.2f}" height="{s_:.2f}" rx="{max(3.0, s_ * 0.28):.1f}" fill="{col}" '
                     f'transform="translate({p[0]:.2f},{p[1]:.2f}) rotate({ang:.1f})"/>')
    defs = (f'<linearGradient id="gx" gradientUnits="userSpaceOnUse" x1="26" y1="0" x2="230" y2="0"><stop offset="0" stop-color="{ACCENT}"/>'
            f'<stop offset="1" stop-color="{ACCENT2}"/></linearGradient>')
    return "".join(tiles), defs


MARKS = [("01-lemniscate", "Lemniscate", mark1), ("02-viewport", "Viewport", mark2), ("03-linked-windows", "Linked windows", mark3),
         ("04-infinite-grid", "Infinite grid", mark4), ("05-tiles", "Tiles to infinity", mark5)]


# ---------------------------------------------------------------- icon (rounded plate) and horizontal logo
def icon(mark):
    body, defs = mark()
    return svg(256, 256, f'<rect width="256" height="256" rx="56" fill="{BG}"/>\n{body}', defs)


def logo(mark):
    body, defs = mark()
    W, H = 760, 256
    big, _ = text_path("infinity", "AdwaitaMono-Bold.ttf", 92, 0, 0)
    d1, w1 = text_path("infinity", "AdwaitaMono-Bold.ttf", 92, 288, 140, spacing=-2)
    d2, w2 = text_path("hyprland", "AdwaitaMono-Regular.ttf", 38, 292, 196, spacing=11)
    text = (f'<path d="{d1}" fill="{TEXT}"/>'
            f'<path d="{d2}" fill="{ACCENT}"/>')
    plate = f'<rect width="{W}" height="{H}" rx="44" fill="{BG}"/>'
    return svg(W, H, f'{plate}\n<g transform="translate(14,0)">{body}</g>\n{text}', defs)


def main():
    for slug, title, fn in MARKS:
        for kind, build in (("icon", icon), ("logo", logo)):
            with open(os.path.join(HERE, f"{slug}-{kind}.svg"), "w") as f:
                f.write(build(fn))
    print("written:", len(MARKS) * 2, "files into", HERE)


if __name__ == "__main__":
    sys.exit(main())
