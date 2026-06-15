#!/usr/bin/env python3
"""
dot_to_html.py
Converte un .tikz.dot (topologia-only, prodotto da UnidirectedGraph::to_tikz_dot)
in un file HTML standalone con SVG inline che disegna lo schema circuitale.

Pipeline:
  1. `neato -Tplain` per le posizioni dei nodi
  2. snap-to-grid (interi, separazione L1 minima)
  3. manhattan routing con detour (riuso del codice di dot_to_tikz.py)
  4. emissione SVG: ogni segmento e' una <polyline>;
     i componenti (resistore zigzag, generatore due barre) sono <g> trasformati
     in posizione/rotazione sul segmento componente

Uso:
    ./dot_to_html.py <file.tikz.dot> [-o output.html] [--cell-size PX]
"""

import argparse
import math
import re
import shutil
import subprocess
import sys
from pathlib import Path


EDGE_RE = re.compile(r'^\s*(\S+)\s*--\s*(\S+)\s*\[(.*)\];\s*$')
ATTR_RE = re.compile(r'(\w+)\s*=\s*"([^"]*)"')


def parse_dot(path: Path):
    edges = []
    with path.open() as fh:
        for line in fh:
            m = EDGE_RE.match(line)
            if not m:
                continue
            a, b, raw = m.group(1), m.group(2), m.group(3)
            edges.append((a, b, dict(ATTR_RE.findall(raw))))
    return edges


def run_neato_plain(path: Path):
    if shutil.which("neato") is None:
        sys.exit("Errore: 'neato' non trovato. Installa graphviz.")
    proc = subprocess.run(
        ["neato", "-Tplain", str(path)],
        check=True, capture_output=True, text=True,
    )
    raw = {}
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[0] == "node":
            raw[parts[1]] = (float(parts[2]), float(parts[3]))
    return raw


def snap_to_grid(raw_positions, base_scale=10.0, min_sep=3):
    scale = base_scale
    for _ in range(40):
        snapped = {n: (round(x * scale), round(y * scale))
                   for n, (x, y) in raw_positions.items()}
        pts = list(snapped.values())
        unique = len(set(pts)) == len(pts)
        ok = True
        for i in range(len(pts)):
            for j in range(i + 1, len(pts)):
                if abs(pts[i][0] - pts[j][0]) + abs(pts[i][1] - pts[j][1]) < min_sep:
                    ok = False
                    break
            if not ok:
                break
        if unique and ok:
            return snapped
        scale *= 1.3
    return snapped


def seg_key(s):
    a, b = s
    return frozenset([a, b])


def seg_length(s):
    (x1, y1), (x2, y2) = s
    return abs(x2 - x1) + abs(y2 - y1)


def segments_overlap(s1, s2):
    (ax1, ay1), (ax2, ay2) = s1
    (bx1, by1), (bx2, by2) = s2
    if ay1 == ay2 and by1 == by2 and ay1 == by1:
        a_lo, a_hi = sorted([ax1, ax2])
        b_lo, b_hi = sorted([bx1, bx2])
        return max(a_lo, b_lo) < min(a_hi, b_hi)
    if ax1 == ax2 and bx1 == bx2 and ax1 == bx1:
        a_lo, a_hi = sorted([ay1, ay2])
        b_lo, b_hi = sorted([by1, by2])
        return max(a_lo, b_lo) < min(a_hi, b_hi)
    return False


def segment_passes_through(seg, points):
    (x1, y1), (x2, y2) = seg
    for (px, py) in points:
        if (px, py) == (x1, y1) or (px, py) == (x2, y2):
            continue
        if x1 == x2 and px == x1 and min(y1, y2) < py < max(y1, y2):
            return True
        if y1 == y2 and py == y1 and min(x1, x2) < px < max(x1, x2):
            return True
    return False


def route_edge(pa, pb, used_set, used_components, used_wires, all_nodes):
    xa, ya = pa
    xb, yb = pb
    if xa == xb or ya == yb:
        return [(pa, pb)]
    candidates = [
        [(pa, (xb, ya)), ((xb, ya), pb)],
        [(pa, (xa, yb)), ((xa, yb), pb)],
    ]
    for dy in (-1, 1, -2, 2):
        for base_y in (ya, yb):
            yh = base_y + dy
            candidates.append([(pa, (xa, yh)), ((xa, yh), (xb, yh)), ((xb, yh), pb)])
    for dx in (-1, 1, -2, 2):
        for base_x in (xa, xb):
            xh = base_x + dx
            candidates.append([(pa, (xh, ya)), ((xh, ya), (xh, yb)), ((xh, yb), pb)])

    cleaned = []
    for route in candidates:
        r = [s for s in route if s[0] != s[1]]
        if r:
            cleaned.append(r)

    best, best_score = None, None
    for route in cleaned:
        reuse = sum(1 for s in route if seg_key(s) in used_set)
        over_c = sum(1 for s in route for u in used_components
                     if segments_overlap(s, u))
        cross = sum(1 for s in route if segment_passes_through(s, all_nodes))
        over_w = sum(1 for s in route for u in used_wires
                     if segments_overlap(s, u))
        tot = sum(seg_length(s) for s in route)
        score = (reuse, over_c, cross, over_w, tot)
        if best is None or score < best_score:
            best = route
            best_score = score
    return best


# ---------- SVG rendering ----------

def svg_resistor(x1, y1, x2, y2, name, value):
    """SVG <g> per resistore zigzag tra (x1,y1) e (x2,y2)."""
    dx = x2 - x1
    dy = y2 - y1
    length = math.hypot(dx, dy)
    angle = math.degrees(math.atan2(dy, dx))
    body_len = 40
    body_h = 12
    pad = (length - body_len) / 2
    zigzag_points = []
    n_zigs = 6
    zig_w = body_len / n_zigs
    for i in range(n_zigs + 1):
        zx = pad + i * zig_w
        zy = body_h / 2 if (i % 2 == 1) else -body_h / 2
        if i == 0 or i == n_zigs:
            zy = 0
        zigzag_points.append(f"{zx},{zy}")
    points_str = " ".join([f"0,0", f"{pad},0"] + zigzag_points + [f"{pad+body_len},0", f"{length},0"])
    # Label posizionata sopra il body
    label_x = length / 2
    label_y = -body_h - 4
    val_y = body_h + 14
    return f'''<g transform="translate({x1},{y1}) rotate({angle})">
    <polyline points="{points_str}" fill="none" stroke="black" stroke-width="1.6"/>
    <text x="{label_x}" y="{label_y}" text-anchor="middle" font-family="serif" font-style="italic" font-size="14" transform="rotate({-angle} {label_x} {label_y})">{name}</text>
    <text x="{label_x}" y="{val_y}" text-anchor="middle" font-family="serif" font-size="12" transform="rotate({-angle} {label_x} {val_y})">{value}&#937;</text>
  </g>'''


def svg_battery(x1, y1, x2, y2, name, value, invert):
    """SVG <g> per generatore di tensione (due barre, lunga=+, corta=-).
    Se invert=True, "+" sull'estremo (x1,y1) anziche' (x2,y2)."""
    dx = x2 - x1
    dy = y2 - y1
    length = math.hypot(dx, dy)
    angle = math.degrees(math.atan2(dy, dx))
    sep = 8
    long_h = 18
    short_h = 10
    # Posizione barre: centro
    cx = length / 2
    # Default: "+" verso x2 (lato finale). Barra lunga = +.
    long_x = cx + sep / 2
    short_x = cx - sep / 2
    if invert:
        long_x, short_x = short_x, long_x
    label_x = length / 2
    label_y = -long_h / 2 - 6
    val_y = long_h / 2 + 14
    # Marker + e -
    plus_x = long_x
    minus_x = short_x
    sign_y = -long_h / 2 - 4
    return f'''<g transform="translate({x1},{y1}) rotate({angle})">
    <line x1="0" y1="0" x2="{short_x}" y2="0" stroke="black" stroke-width="1.6"/>
    <line x1="{short_x}" y1="{-short_h/2}" x2="{short_x}" y2="{short_h/2}" stroke="black" stroke-width="1.8"/>
    <line x1="{long_x}" y1="{-long_h/2}" x2="{long_x}" y2="{long_h/2}" stroke="black" stroke-width="1.8"/>
    <line x1="{long_x}" y1="0" x2="{length}" y2="0" stroke="black" stroke-width="1.6"/>
    <text x="{plus_x}" y="{sign_y}" text-anchor="middle" font-family="serif" font-size="14" fill="red" transform="rotate({-angle} {plus_x} {sign_y})">+</text>
    <text x="{minus_x}" y="{sign_y}" text-anchor="middle" font-family="serif" font-size="14" fill="red" transform="rotate({-angle} {minus_x} {sign_y})">&#8722;</text>
    <text x="{label_x}" y="{label_y - 8}" text-anchor="middle" font-family="serif" font-style="italic" font-size="14" transform="rotate({-angle} {label_x} {label_y - 8})">{name}</text>
    <text x="{label_x}" y="{val_y}" text-anchor="middle" font-family="serif" font-size="12" transform="rotate({-angle} {label_x} {val_y})">{value} V</text>
  </g>'''


def emit_html(edges, positions, cell_size: int = 60, margin: int = 60) -> str:
    """Genera HTML standalone con SVG inline."""
    xs = [x for (x, _) in positions.values()]
    ys = [y for (_, y) in positions.values()]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    width = (maxx - minx) * cell_size + 2 * margin
    height = (maxy - miny) * cell_size + 2 * margin

    def to_px(p):
        x, y = p
        # Inverti y per avere "su" verso alto come ci si aspetta
        px = (x - minx) * cell_size + margin
        py = (maxy - y) * cell_size + margin
        return (px, py)

    used_segments = set()
    used_components = []
    used_wires = []
    all_nodes = list(positions.values())

    routed = []  # lista (edge_idx, segments, comp_seg_idx, name, val, ctype, pos_node, a, b)
    for idx, (a, b, attrs) in enumerate(edges):
        if a not in positions or b not in positions:
            continue
        pa = positions[a]
        pb = positions[b]
        segments = route_edge(pa, pb, used_segments,
                              used_components, used_wires, all_nodes)
        for s in segments:
            used_segments.add(seg_key(s))
        comp_seg_idx = max(range(len(segments)),
                           key=lambda i: seg_length(segments[i]))
        for i, s in enumerate(segments):
            if i == comp_seg_idx:
                used_components.append(s)
            else:
                used_wires.append(s)
        routed.append((idx, segments, comp_seg_idx, attrs, a, b))

    svg_parts = []
    # Wire prima, componenti dopo, nodi sopra
    for (idx, segments, comp_idx, attrs, a, b) in routed:
        for i, (s, e) in enumerate(segments):
            if i == comp_idx:
                continue
            sx, sy = to_px(s)
            ex, ey = to_px(e)
            svg_parts.append(
                f'<line x1="{sx}" y1="{sy}" x2="{ex}" y2="{ey}" '
                f'stroke="black" stroke-width="1.6"/>'
            )

    for (idx, segments, comp_idx, attrs, a, b) in routed:
        s, e = segments[comp_idx]
        sx, sy = to_px(s)
        ex, ey = to_px(e)
        name = attrs.get("comp", "?")
        val = attrs.get("val", "")
        ctype = attrs.get("type", "R")
        pos_node = attrs.get("pos", a)
        if ctype == "R":
            svg_parts.append(svg_resistor(sx, sy, ex, ey, name, val))
        else:
            invert = (pos_node == a)
            svg_parts.append(svg_battery(sx, sy, ex, ey, name, val, invert))

    # Nodi
    for node_id, p in positions.items():
        px, py = to_px(p)
        svg_parts.append(
            f'<circle cx="{px}" cy="{py}" r="4" fill="black"/>'
        )
        svg_parts.append(
            f'<text x="{px - 8}" y="{py - 8}" text-anchor="end" '
            f'font-family="serif" font-size="14" fill="#0a0">{node_id}</text>'
        )

    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" '
           f'width="{width}" height="{height}" '
           f'viewBox="0 0 {width} {height}">\n  '
           + "\n  ".join(svg_parts) + "\n</svg>")

    html = f"""<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="utf-8">
  <title>Circuito</title>
  <style>
    body {{ margin: 0; display: flex; justify-content: center; padding: 20px;
            font-family: sans-serif; background: #fafafa; }}
    svg {{ background: white; border: 1px solid #ccc; }}
  </style>
</head>
<body>
{svg}
</body>
</html>
"""
    return html


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("dotfile", type=Path)
    p.add_argument("-o", "--output", type=Path)
    p.add_argument("--cell-size", type=int, default=60,
                   help="pixel per unita' di griglia (default 60)")
    args = p.parse_args()

    if not args.dotfile.is_file():
        sys.exit(f"File non trovato: {args.dotfile}")

    raw = run_neato_plain(args.dotfile)
    positions = snap_to_grid(raw)
    edges = parse_dot(args.dotfile)
    html = emit_html(edges, positions, cell_size=args.cell_size)

    out = args.output or args.dotfile.with_suffix(".html")
    out.write_text(html)
    print(f"Generato: {out}")


if __name__ == "__main__":
    main()
