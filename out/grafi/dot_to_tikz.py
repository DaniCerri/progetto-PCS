#!/usr/bin/env python3
"""
dot_to_tikz.py
Converte un .dot "topologia-only" prodotto da UnidirectedGraph::to_tikz_dot
in un sorgente LaTeX/CircuiTikZ.

Pipeline interna:
  1. Esegue `neato -Tplain <file.dot>` per ottenere posizioni (x,y) dei nodi.
  2. Snap-to-grid: arrotonda le coordinate a interi, garantendo unicita'.
  3. Manhattan routing: ogni arco diagonale e' spezzato in due segmenti
     (uno orizzontale, uno verticale) con un bend intermedio. Il componente
     occupa il segmento piu' lungo; sull'altro viene disegnato un wire.
  4. Riparsa il .dot per estrarre gli attributi degli archi (comp,val,type,pos).
  5. Emette un documento standalone CircuiTikZ.

Uso:
    ./dot_to_tikz.py <file.tikz.dot> [-o output.tex] [--scale FATTORE]
"""

import argparse
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
            a, b, raw_attrs = m.group(1), m.group(2), m.group(3)
            attrs = dict(ATTR_RE.findall(raw_attrs))
            edges.append((a, b, attrs))
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
    """Snappa le coordinate a interi, scalando finche':
       - tutti i nodi sono distinti
       - le distanze L1 minime tra coppie di nodi sono >= min_sep
       (cosi' i bend e i wire non si sovrappongono)."""
    scale = base_scale
    for _ in range(40):
        snapped = {n: (round(x * scale), round(y * scale))
                   for n, (x, y) in raw_positions.items()}
        pts = list(snapped.values())
        unique = len(set(pts)) == len(pts)
        ok_sep = True
        for i in range(len(pts)):
            for j in range(i + 1, len(pts)):
                if abs(pts[i][0] - pts[j][0]) + abs(pts[i][1] - pts[j][1]) < min_sep:
                    ok_sep = False
                    break
            if not ok_sep:
                break
        if unique and ok_sep:
            return snapped
        scale *= 1.3
    return snapped  # fallback


def seg_key(s):
    a, b = s
    return frozenset([a, b])


def segments_overlap(s1, s2):
    """True se due segmenti H o V hanno overlap piu' che puntuale."""
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
    """True se il segmento (assunto H o V) passa nell'interno di uno dei punti."""
    (x1, y1), (x2, y2) = seg
    for (px, py) in points:
        if (px, py) == (x1, y1) or (px, py) == (x2, y2):
            continue
        if x1 == x2 and px == x1 and min(y1, y2) < py < max(y1, y2):
            return True
        if y1 == y2 and py == y1 and min(x1, x2) < px < max(x1, x2):
            return True
    return False


def route_edge(pa, pb, used_segments_set, used_components, used_wires, all_nodes):
    """Ritorna i segmenti per andare da pa a pb. Genera anche routing a 3 segmenti
    (detour) per evitare overlap. Sceglie il routing col punteggio minore:
      - reuse: segmento gia' usato con stessi endpoint
      - over_comp: overlap collineare con un componente gia' piazzato (grave)
      - cross: segmento attraversa interno di un nodo
      - over_wire: overlap collineare con un wire (meno grave)
      - lunghezza totale del routing
    """
    xa, ya = pa
    xb, yb = pb
    if xa == xb or ya == yb:
        return [(pa, pb)]

    candidates = []
    # L classico in due varianti
    candidates.append([(pa, (xb, ya)), ((xb, ya), pb)])
    candidates.append([(pa, (xa, yb)), ((xa, yb), pb)])
    # Detour a 3 segmenti: bend orizzontale a quota yh
    for dy in (-1, 1, -2, 2):
        yh = ya + dy
        candidates.append([(pa, (xa, yh)), ((xa, yh), (xb, yh)), ((xb, yh), pb)])
        yh = yb + dy
        candidates.append([(pa, (xa, yh)), ((xa, yh), (xb, yh)), ((xb, yh), pb)])
    # Detour a 3 segmenti: bend verticale a x = xh
    for dx in (-1, 1, -2, 2):
        xh = xa + dx
        candidates.append([(pa, (xh, ya)), ((xh, ya), (xh, yb)), ((xh, yb), pb)])
        xh = xb + dx
        candidates.append([(pa, (xh, ya)), ((xh, ya), (xh, yb)), ((xh, yb), pb)])

    # Scarta routing con segmenti di lunghezza zero o duplicati
    cleaned = []
    for route in candidates:
        r = [s for s in route if s[0] != s[1]]
        if r:
            cleaned.append(r)

    best = None
    best_score = None
    for route in cleaned:
        reuse = sum(1 for s in route if seg_key(s) in used_segments_set)
        over_comp = sum(1 for s in route
                        for u in used_components
                        if segments_overlap(s, u))
        cross = sum(1 for s in route if segment_passes_through(s, all_nodes))
        over_wire = sum(1 for s in route
                        for u in used_wires
                        if segments_overlap(s, u))
        total_len = sum(seg_length(s) for s in route)
        score = (reuse, over_comp, cross, over_wire, total_len)
        if best is None or score < best_score:
            best = route
            best_score = score
    return best


def seg_length(s):
    (x1, y1), (x2, y2) = s
    return abs(x2 - x1) + abs(y2 - y1)


def emit_tikz(edges, positions, scale: float) -> str:
    out = []
    out.append(r"\documentclass[border=5pt]{standalone}")
    out.append(r"\usepackage[european]{circuitikz}")
    out.append(r"\begin{document}")
    out.append(rf"\begin{{circuitikz}}[scale={scale}]")

    used_segments = set()
    used_components = []
    used_wires = []
    all_nodes = list(positions.values())
    for a, b, attrs in edges:
        if a not in positions or b not in positions:
            continue
        pa = positions[a]
        pb = positions[b]
        name = attrs.get("comp", "?")
        val = attrs.get("val", "")
        ctype = attrs.get("type", "R")
        pos_node = attrs.get("pos", a)

        if ctype == "R":
            label_val = f"${val}\\,\\Omega$"
            tikz_comp = "R"
        else:
            label_val = f"${val}\\,V$"
            tikz_comp = "battery1"

        segments = route_edge(pa, pb, used_segments,
                              used_components, used_wires, all_nodes)
        for s in segments:
            used_segments.add(seg_key(s))
        # Componente sul segmento piu' lungo (simbolo piu' leggibile).
        comp_seg_idx = max(range(len(segments)),
                           key=lambda i: seg_length(segments[i]))

        for i, (s, e) in enumerate(segments):
            if i == comp_seg_idx:
                invert = (ctype == "V" and pos_node == a)
                opts = f"{tikz_comp}, l=${name}$, a={label_val}"
                if invert:
                    opts += ", invert"
                out.append(rf"  \draw ({s[0]},{s[1]}) to[{opts}] ({e[0]},{e[1]});")
                used_components.append((s, e))
            else:
                out.append(rf"  \draw ({s[0]},{s[1]}) -- ({e[0]},{e[1]});")
                used_wires.append((s, e))

    # Nodi del circuito (pallino + etichetta)
    for node_id, (x, y) in positions.items():
        out.append(rf"  \node[circ] at ({x},{y}) {{}};")
        out.append(rf"  \node[anchor=south east, font=\scriptsize] at ({x},{y}) {{{node_id}}};")

    out.append(r"\end{circuitikz}")
    out.append(r"\end{document}")
    return "\n".join(out) + "\n"


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("dotfile", type=Path, help="file .tikz.dot di input")
    p.add_argument("-o", "--output", type=Path,
                   help="file .tex di output (default: <dotfile>.tex)")
    p.add_argument("--scale", type=float, default=1.0,
                   help="fattore di scala per il disegno (default 1.0)")
    args = p.parse_args()

    if not args.dotfile.is_file():
        sys.exit(f"File non trovato: {args.dotfile}")

    raw = run_neato_plain(args.dotfile)
    positions = snap_to_grid(raw)
    edges = parse_dot(args.dotfile)
    tex = emit_tikz(edges, positions, args.scale)

    out = args.output or args.dotfile.with_suffix(".tex")
    out.write_text(tex)
    print(f"Generato: {out}")


if __name__ == "__main__":
    main()
