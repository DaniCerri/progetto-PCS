#!/usr/bin/env python3
"""
dot_to_schemdraw.py
Converte un .tikz.dot (topologia-only) in SVG + HTML interattivo dello schema
circuitale via schemdraw.

Pipeline (--layout auto, default):
  1. se il circuito e' un FAN (un rail adiacente a tutti, gli altri in cammino)
     -> layout bus-bar: rail come barra orizzontale, derivazioni verticali =
     rettangolo con N-1 maglie (detect_fan/fan_layout).
  2. altrimenti posizioni planari crossing-free: embedding di Tutte con faccia
     esterna su RETTANGOLO (anello squadrato) + spread anti-sovrapposizione;
     poi: se NON c'e' un mozzo ad alto grado, snap a griglia + allineamento
     coordinate ai vicini per minimizzare le pieghe (gridify); se c'e' un mozzo
     (wheel) lo si centra sul baricentro dei vicini (raggi corti e simmetrici).
     -> 0 incroci, 0 fili sovrapposti. Richiede networkx+numpy e grafo
     planare/biconnesso; altrimenti ripiega su neato+manhattan (--layout grid).
  3. routing ibrido: angoli retti dove il grado del nodo lo permette,
     diagonali sugli hub (grado > 4) -> hybrid_routes().
  4. emissione via schemdraw (Resistor, SourceV, Line, Dot) -> SVG.
  5. overlay maglie (cicli) + HTML con toggle per-maglia e pan/zoom.

Uso:
    ./dot_to_schemdraw.py <file.tikz.dot> [-o output.svg]
        [--layout auto|ortho|planar|grid] [--engine neato|sfdp|...] [--no-html]
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

import schemdraw
import schemdraw.elements as elm

try:
    import networkx as nx
    import numpy as np
    _HAS_PLANAR = True
except ImportError:
    _HAS_PLANAR = False


EDGE_RE = re.compile(r'^\s*(\S+)\s*--\s*(\S+)\s*\[(.*)\];\s*$')
ATTR_RE = re.compile(r'(\w+)\s*=\s*"([^"]*)"')

# Palette cicli (mesh). Condivisa tra overlay SVG e legenda HTML.
PALETTE = ["#d33", "#36c", "#2a8", "#c80", "#86c", "#a44"]


def parse_dot(path: Path):
    edges = []
    with path.open() as fh:
        for line in fh:
            m = EDGE_RE.match(line)
            if not m:
                continue
            edges.append((m.group(1), m.group(2),
                          dict(ATTR_RE.findall(m.group(3)))))
    return edges


LAYOUT_ATTR_RE = re.compile(r'\blayout\s*=\s*\w+\s*;?')


def run_layout_plain(path: Path, engine: str = "neato"):
    """Calcola posizioni nodi via Graphviz <engine> -Tplain.

    L'attributo `layout=...` eventualmente presente nel .dot viene strippato:
    altrimenti vince sul binario invocato e il flag --engine sarebbe ignorato.
    Engine utili per schemi: neato (compatto), sfdp (meno incroci), dot
    (layered), circo (anelli singoli)."""
    if shutil.which(engine) is None:
        sys.exit(f"Errore: '{engine}' non trovato. Installa graphviz.")
    src = LAYOUT_ATTR_RE.sub('', path.read_text())
    proc = subprocess.run([engine, "-Tplain"], input=src,
                          check=True, capture_output=True, text=True)
    raw = {}
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[0] == "node":
            raw[parts[1]] = (float(parts[2]), float(parts[3]))
    return raw


def snap_to_grid(raw, base_scale=10.0, min_sep=3):
    scale = base_scale
    snapped = {}
    for _ in range(40):
        snapped = {n: (round(x * scale), round(y * scale))
                   for n, (x, y) in raw.items()}
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
    return frozenset([s[0], s[1]])


def seg_len(s):
    (x1, y1), (x2, y2) = s
    return abs(x2 - x1) + abs(y2 - y1)


def segs_overlap(s1, s2):
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


def seg_through(seg, pts):
    (x1, y1), (x2, y2) = seg
    for (px, py) in pts:
        if (px, py) in (seg[0], seg[1]):
            continue
        if x1 == x2 and px == x1 and min(y1, y2) < py < max(y1, y2):
            return True
        if y1 == y2 and py == y1 and min(x1, x2) < px < max(x1, x2):
            return True
    return False


def route_edge(pa, pb, used_segs, used_comp, used_wire, nodes):
    xa, ya = pa
    xb, yb = pb
    if xa == xb or ya == yb:
        return [(pa, pb)]
    cands = [
        [(pa, (xb, ya)), ((xb, ya), pb)],
        [(pa, (xa, yb)), ((xa, yb), pb)],
    ]
    for dy in (-1, 1, -2, 2):
        for base in (ya, yb):
            yh = base + dy
            cands.append([(pa, (xa, yh)), ((xa, yh), (xb, yh)), ((xb, yh), pb)])
    for dx in (-1, 1, -2, 2):
        for base in (xa, xb):
            xh = base + dx
            cands.append([(pa, (xh, ya)), ((xh, ya), (xh, yb)), ((xh, yb), pb)])

    cleaned = []
    for r in cands:
        c = [s for s in r if s[0] != s[1]]
        if c:
            cleaned.append(c)

    best, best_sc = None, None
    for r in cleaned:
        reuse = sum(1 for s in r if seg_key(s) in used_segs)
        oc = sum(1 for s in r for u in used_comp if segs_overlap(s, u))
        cr = sum(1 for s in r if seg_through(s, nodes))
        ow = sum(1 for s in r for u in used_wire if segs_overlap(s, u))
        tot = sum(seg_len(s) for s in r)
        sc = (reuse, oc, cr, ow, tot)
        if best is None or sc < best_sc:
            best, best_sc = r, sc
    return best


def compute_routes(edges, positions):
    """Calcola routing manhattan per ogni arco.
    Ritorna dict {(a,b): (attrs, segs, comp_idx)} con (a,b) come nell'input."""
    used_segs = set()
    used_comp = []
    used_wire = []
    nodes = list(positions.values())

    # Ordine: archi rigidi (allineati su asse) prima, poi L-routes.
    def is_aligned(e):
        a, b, _ = e
        if a not in positions or b not in positions:
            return False
        pa, pb = positions[a], positions[b]
        return pa[0] == pb[0] or pa[1] == pb[1]

    sorted_edges = [e for e in edges if is_aligned(e)] + \
                   [e for e in edges if not is_aligned(e)]

    out = {}
    for (a, b, attrs) in sorted_edges:
        if a not in positions or b not in positions:
            continue
        pa, pb = positions[a], positions[b]
        segs = route_edge(pa, pb, used_segs, used_comp, used_wire, nodes)
        for s in segs:
            used_segs.add(seg_key(s))
        ci = max(range(len(segs)), key=lambda i: seg_len(segs[i]))
        for i, s in enumerate(segs):
            (used_comp if i == ci else used_wire).append(s)
        out[(a, b)] = (attrs, segs, ci)
    return out


def _straight_crossings(P, E, idx):
    """Conta incroci tra archi disegnati a linea retta (esclusi estremi
    condivisi). P: array Nx2, E: lista (a,b), idx: nodo->riga."""
    def ccw(a, b, c):
        return (c[1] - a[1]) * (b[0] - a[0]) - (b[1] - a[1]) * (c[0] - a[0])

    def hit(p1, p2, p3, p4):
        if (p1 == p3).all() or (p1 == p4).all() or \
           (p2 == p3).all() or (p2 == p4).all():
            return False
        return (ccw(p3, p4, p1) > 0) != (ccw(p3, p4, p2) > 0) and \
               (ccw(p1, p2, p3) > 0) != (ccw(p1, p2, p4) > 0)

    S = [(P[idx[a]], P[idx[b]]) for a, b in E]
    return sum(hit(*S[i], *S[j])
               for i in range(len(S)) for j in range(i + 1, len(S)))


def _min_node_edge(P, Eidx, N):
    """Distanza minima nodo->arco non incidente. Misura quanto un nodo (o un
    arco) e' vicino a sovrapporsi a un altro arco; 0 = sovrapposizione."""
    mn = 1e9
    for k in range(N):
        for i, j in Eidx:
            if k == i or k == j:
                continue
            a, b = P[i], P[j]
            d = b - a
            L2 = d @ d
            t = max(0.0, min(1.0, ((P[k] - a) @ d) / L2)) if L2 > 0 else 0.0
            mn = min(mn, float(np.hypot(*(P[k] - (a + t * d)))))
    return mn


# Sotto questo rapporto dist-nodo-arco / span il layout planare e' considerato
# degenere (fili/nodi quasi sovrapposti) e si ripiega sul grid.
_MIN_SEP_RATIO = 0.04


def _gridify(P, idx, nodes, E):
    """Snap a griglia intera + allinea le coordinate ai vicini per ridurre le
    pieghe: un arco con xa==xb o ya==yb sta su un asse -> 0 pieghe (resta
    dritto), gli altri richiedono una L. Ricerca locale greedy: ogni nodo prova
    a spostare x o y sul valore di un vicino, accetta solo se il numero di archi
    diagonali (= pieghe) cala e restano 0 incroci / nessun nodo o filo
    sovrapposto. Effetto: schema squadrato, dentro a rettangoli, poche pieghe.

    Ritorna array Nx2 su griglia intera (0 incroci) oppure None se degenere."""
    N = len(nodes)
    Eidx = [(idx[a], idx[b]) for a, b in E]
    nbr = {i: set() for i in range(N)}
    for i, j in Eidx:
        nbr[i].add(j)
        nbr[j].add(i)

    mn = P.min(axis=0)
    span = float((P.max(axis=0) - mn).max()) or 1.0
    # risoluzione griglia: alza finche' nessuna cella duplicata
    g = max(8, int(round(3 * np.sqrt(N))))
    Q = None
    for _ in range(8):
        cand = np.round((P - mn) / span * g)
        if len({(r[0], r[1]) for r in cand}) == N:
            Q = cand
            break
        g = int(g * 1.4)
    if Q is None or _straight_crossings(Q, E, idx) != 0:
        return None

    def bends(M):
        return sum(1 for i, j in Eidx
                   if M[i, 0] != M[j, 0] and M[i, 1] != M[j, 1])

    def ok(M):
        return (_straight_crossings(M, E, idx) == 0
                and _min_node_edge(M, Eidx, N) >= 0.5)

    occupied = {(r[0], r[1]) for r in Q}
    improved = True
    guard = 0
    while improved and guard < 50:
        improved = False
        guard += 1
        for i in range(N):
            cur = Q[i].copy()
            cur_key = (cur[0], cur[1])
            cells = set()
            for w in nbr[i]:
                cells.add((Q[w, 0], cur[1]))   # allinea x al vicino
                cells.add((cur[0], Q[w, 1]))    # allinea y al vicino
            best_key, best_b = cur_key, bends(Q)
            for cell in cells:
                if cell != cur_key and cell in occupied:
                    continue
                Q[i] = np.array(cell)
                if ok(Q):
                    b = bends(Q)
                    if b < best_b:
                        best_b, best_key = b, cell
                Q[i] = cur
            if best_key != cur_key:
                occupied.discard(cur_key)
                occupied.add(best_key)
                Q[i] = np.array(best_key)
                improved = True
    return Q


def planar_positions(edges, target_span=12.0, iters=600):
    """Layout planare crossing-free via embedding di Tutte (faccia esterna su
    poligono convesso + nodi interni al baricentro), seguito da spread
    force-directed con guardia anti-incrocio (la faccia esterna resta fissa).

    Ritorna dict nodo->(x,y) scalato a ~target_span, oppure None se il grafo
    non e' planare/biconnesso o mancano networkx/numpy."""
    if not _HAS_PLANAR:
        return None
    E = [(a, b) for a, b, _ in edges]
    G = nx.Graph()
    G.add_edges_from(E)
    if not nx.is_biconnected(G):
        return None  # Tutte garantisce planarita' solo su grafi biconnessi
    ok, emb = nx.check_planarity(G)
    if not ok:
        return None

    # Faccia piu' grande come contorno esterno. Tie-break deterministico
    # (esistono grafi con piu' facce della stessa dimensione massima): a parita'
    # di lunghezza scelgo la lessicograficamente minima per id ordinati, cosi' il
    # layout e' riproducibile run-to-run (niente dipendenza dall'hash dei nodi).
    seen = set()
    faces = []
    for u in emb:
        for v in emb[u]:
            if (u, v) not in seen:
                faces.append(emb.traverse_face(u, v, mark_half_edges=seen))
    outer = sorted(faces, key=lambda f: (-len(f), sorted(f)))[0]

    nodes = list(G.nodes())
    idx = {n: i for i, n in enumerate(nodes)}
    N = len(nodes)
    fixed = set(outer)

    # Tutte: risolvi il sistema baricentrico con la faccia esterna ancorata.
    # La faccia esterna sta su un RETTANGOLO (non un cerchio) cosi' l'anello
    # esce squadrato e il mozzo finisce al baricentro = centro. Piccolo bulge a
    # meta' lato per mantenere il poligono strettamente convesso (Tutte lo
    # richiede); gridify poi appiattisce sul rettangolo.
    A = np.zeros((N, N))
    bx = np.zeros(N)
    by = np.zeros(N)
    m = len(outer)
    for j, n in enumerate(outer):
        t = 4.0 * j / m
        side = int(t) % 4
        f = t - int(t)
        bb = 0.18 * np.sin(np.pi * f)
        if side == 0:
            ox, oy = -1 + 2 * f, -1 - bb
        elif side == 1:
            ox, oy = 1 + bb, -1 + 2 * f
        elif side == 2:
            ox, oy = 1 - 2 * f, 1 + bb
        else:
            ox, oy = -1 - bb, 1 - 2 * f
        A[idx[n], idx[n]] = 1
        bx[idx[n]], by[idx[n]] = ox, oy
    for n in nodes:
        if n in fixed:
            continue
        nb = list(G[n])
        A[idx[n], idx[n]] = len(nb)
        for w in nb:
            A[idx[n], idx[w]] -= 1
    try:
        P = np.column_stack([np.linalg.solve(A, bx), np.linalg.solve(A, by)])
    except np.linalg.LinAlgError:
        return None

    # Spread: hill-climb che MASSIMIZZA la distanza minima nodo-arco (cioe'
    # allontana fili e nodi dalle sovrapposizioni collineari che Tutte non
    # esclude) sotto il vincolo di 0 incroci. Forze: repulsione nodo-nodo,
    # molle sugli archi, repulsione nodo-arco. Lo step e' normalizzato (niente
    # blow-up) e si tiene il miglior layout incontrato. Solo nodi interni.
    Eidx = [(idx[a], idx[b]) for a, b in E]
    interior = [idx[n] for n in nodes if n not in fixed]
    if interior:
        best = P.copy()
        best_q = _min_node_edge(P, Eidx, N)
        step = 0.08
        for _ in range(iters):
            F = np.zeros_like(P)
            for i in range(N):
                d = P[i] - P
                r = np.hypot(d[:, 0], d[:, 1])
                r[i] = 1e9
                F[i] += (d * ((1.0 / (r * r)) / r)[:, None]).sum(axis=0)
            for i, k in Eidx:
                d = P[k] - P[i]
                r = np.hypot(*d) + 1e-9
                f = (r - 0.6) * 0.3 * d / r
                F[i] += f
                F[k] -= f
            for k in range(N):
                for i, j in Eidx:
                    if k == i or k == j:
                        continue
                    a, b = P[i], P[j]
                    d = b - a
                    L2 = d @ d
                    t = max(0.0, min(1.0, ((P[k] - a) @ d) / L2)) \
                        if L2 > 0 else 0.0
                    diff = P[k] - (a + t * d)
                    r = np.hypot(*diff) + 1e-9
                    if r < 0.6:
                        F[k] += diff / r * (0.5 / (r * r))
            newP = P.copy()
            for i in interior:
                nrm = np.hypot(*F[i])
                if nrm > 1e-9:
                    newP[i] = P[i] + step * F[i] / nrm  # passo normalizzato
            if np.isnan(newP).any():
                step *= 0.5
                continue
            if _straight_crossings(newP, E, idx) == 0:
                P = newP
                q = _min_node_edge(P, Eidx, N)
                if q > best_q:
                    best_q, best = q, P.copy()
            else:
                step *= 0.8
        P = best

    deg = {n: 0 for n in nodes}
    for a, b in E:
        deg[a] += 1
        deg[b] += 1
    has_hub = max(deg.values()) >= 5

    # Squadratura: snap a griglia + allineamento coordinate ai vicini per
    # minimizzare le pieghe (archi sugli assi). Solo se NON c'e' un mozzo ad alto
    # grado: su un wheel la griglia trasformerebbe il ventaglio in un groviglio
    # ortogonale. I wheel restano su Tutte rettangolare (anello squadrato) +
    # centraggio del mozzo (sotto). Se gridify fallisce, tieni il diagonale.
    Q = None if has_hub else _gridify(P, idx, nodes, E)
    on_grid = Q is not None
    if on_grid:
        P = Q

    # Centra i mozzi (grado alto): li sposto sul baricentro dei loro vicini cosi'
    # i raggi sono corti e simmetrici (wheel col mozzo al centro) invece di un
    # ventaglio sbilanciato. Solo se non introduce incroci/sovrapposizioni.
    sep = 0.5 if on_grid else 0.05
    for n in nodes:
        if deg[n] < 5:
            continue
        i = idx[n]
        c = P[[idx[w] for w in G[n]]].mean(axis=0)
        if on_grid:
            c = np.round(c)
        old = P[i].copy()
        P[i] = c
        dup = any(k != i and (P[k] == c).all() for k in range(N))
        if dup or _straight_crossings(P, E, idx) != 0 \
                or _min_node_edge(P, Eidx, N) < sep:
            P[i] = old

    xs, ys = P[:, 0], P[:, 1]
    span = max(xs.max() - xs.min(), ys.max() - ys.min()) or 1.0
    # Scarta layout degeneri (fili/nodi quasi sovrapposti) -> fallback grid
    if _min_node_edge(P, Eidx, N) / span < _MIN_SEP_RATIO:
        return None
    sc = target_span / span
    return {n: ((P[idx[n], 0] - xs.min()) * sc,
                (P[idx[n], 1] - ys.min()) * sc) for n in nodes}


def straight_routes(edges, positions):
    """Routing a linea retta: un solo segmento per arco. Stessa struttura di
    compute_routes() cosi' build_drawing()/inject_cycles() funzionano uguale."""
    out = {}
    for (a, b, attrs) in edges:
        if a not in positions or b not in positions:
            continue
        out[(a, b)] = (attrs, [(positions[a], positions[b])], 0)
    return out


def _ccw(a, b, c):
    return (c[1] - a[1]) * (b[0] - a[0]) - (b[1] - a[1]) * (c[0] - a[0])


def _seg_cross(p1, p2, p3, p4):
    """Incrocio proprio tra due segmenti generici (estremi condivisi esclusi)."""
    if p1 in (p3, p4) or p2 in (p3, p4):
        return False
    return (_ccw(p3, p4, p1) > 0) != (_ccw(p3, p4, p2) > 0) and \
           (_ccw(p1, p2, p3) > 0) != (_ccw(p1, p2, p4) > 0)


def _seg_overlap(p1, p2, p3, p4):
    """Due segmenti collineari che si sovrappongono (lunghezza > 0)."""
    if abs(_ccw(p1, p2, p3)) > 1e-9 or abs(_ccw(p1, p2, p4)) > 1e-9:
        return False
    dx, dy = p2[0] - p1[0], p2[1] - p1[1]

    def proj(p):
        return (p[0] - p1[0]) * dx + (p[1] - p1[1]) * dy

    a = sorted([proj(p1), proj(p2)])
    b = sorted([proj(p3), proj(p4)])
    return max(a[0], b[0]) < min(a[1], b[1]) - 1e-9


def _pt_on_seg(p, a, b):
    """p (non estremo) giace sul segmento a-b."""
    if p == a or p == b or abs(_ccw(a, b, p)) > 1e-9:
        return False
    return (min(a[0], b[0]) - 1e-9 <= p[0] <= max(a[0], b[0]) + 1e-9 and
            min(a[1], b[1]) - 1e-9 <= p[1] <= max(a[1], b[1]) + 1e-9)


def hybrid_routes(edges, positions, max_ortho_degree=4):
    """Angoli retti dove possibile, diagonali dove il grado non lo consente.

    Parte dal routing diagonale (posizioni planari: 0 incroci, 0 overlap) e
    prova a sostituire ogni arco con una L ortogonale, accettandola solo se non
    introduce incroci/sovrapposizioni/passaggi-su-nodo. Gli archi su nodi ad
    alto grado (hub) non vengono nemmeno tentati: 4 lati non bastano e le L si
    sovrapporrebbero -> restano diagonali. L'invariante 0 incroci/0 overlap del
    layout diagonale di partenza e' quindi preservata per costruzione."""
    deg = {}
    for a, b, _ in edges:
        deg[a] = deg.get(a, 0) + 1
        deg[b] = deg.get(b, 0) + 1
    nodes = list(positions.keys())
    keys = [(a, b) for a, b, _ in edges if a in positions and b in positions]
    segs = {(a, b): [(positions[a], positions[b])]
            for a, b, _ in edges if a in positions and b in positions}

    def conflict_free(cand, skip, endpoints):
        for s in cand:
            for k in keys:
                if k == skip:
                    continue
                for u in segs[k]:
                    if _seg_cross(s[0], s[1], u[0], u[1]) or \
                       _seg_overlap(s[0], s[1], u[0], u[1]):
                        return False
            for n in nodes:
                if positions[n] in endpoints:
                    continue
                if _pt_on_seg(positions[n], s[0], s[1]):
                    return False
        return True

    # Solo archi tra nodi a basso grado; piu' corti prima (piu' facili da
    # rendere ortogonali senza conflitti).
    cand = [(a, b, at) for a, b, at in edges
            if a in positions and b in positions
            and deg[a] <= max_ortho_degree and deg[b] <= max_ortho_degree]
    cand.sort(key=lambda e: (positions[e[0]][0] - positions[e[1]][0]) ** 2 +
                            (positions[e[0]][1] - positions[e[1]][1]) ** 2)
    for a, b, _ in cand:
        pa, pb = positions[a], positions[b]
        if abs(pa[0] - pb[0]) < 1e-9 or abs(pa[1] - pb[1]) < 1e-9:
            continue  # gia' ortogonale
        for corner in [(pb[0], pa[1]), (pa[0], pb[1])]:
            cand_segs = [(pa, corner), (corner, pb)]
            if conflict_free(cand_segs, (a, b), {pa, pb}):
                segs[(a, b)] = cand_segs
                break

    out = {}
    for a, b, attrs in edges:
        if (a, b) not in segs:
            continue
        ss = segs[(a, b)]
        ci = max(range(len(ss)), key=lambda i: seg_len(ss[i]))
        out[(a, b)] = (attrs, ss, ci)
    return out


def detect_fan(edges):
    """Riconosce la struttura 'fan': un nodo rail R adiacente a tutti gli altri,
    e gli altri formano un cammino semplice (1-2-3-4). E' il caso in cui il rail
    va disegnato come barra orizzontale (bus) con le derivazioni che scendono
    verticali -> rettangolo con N-1 maglie. Ritorna (rail, [nodi in ordine sul
    cammino]) oppure None."""
    if not _HAS_PLANAR:
        return None
    G = nx.Graph()
    G.add_edges_from([(a, b) for a, b, _ in edges])
    R = max(G.degree(), key=lambda x: x[1])[0]
    if G.degree(R) < 3:
        return None
    H = G.copy()
    H.remove_node(R)
    if H.number_of_nodes() == 0 or not nx.is_connected(H):
        return None
    degs = dict(H.degree())
    if any(d > 2 for d in degs.values()):
        return None  # cammino => grado massimo 2
    if any(not G.has_edge(R, n) for n in H.nodes()):
        return None  # rail deve toccare ogni nodo
    ends = [n for n, d in degs.items() if d == 1]
    if H.number_of_nodes() == 1:
        return R, list(H.nodes())
    if len(ends) != 2:
        return None
    order = [ends[0]]
    prev, cur = None, ends[0]
    while len(order) < H.number_of_nodes():
        nxt = [m for m in H[cur] if m != prev][0]
        order.append(nxt)
        prev, cur = cur, nxt
    return R, order


def fan_layout(edges, rail, order):
    """Layout bus-bar per un fan: derivazioni sulla fila y=1 (in ordine sul
    cammino), rail come barra orizzontale a y=0, rung verticali. Ritorna
    (positions, routes, extra_wires, rail_meta) dove extra_wires e' il filo del
    rail (Line aggiuntive) e rail_meta=(rail, rail_y, contacts) serve a
    inject_cycles per tracciare il tratto lungo il rail."""
    k = len(order)
    pos_idx = {n: i for i, n in enumerate(order)}
    positions = {n: (float(pos_idx[n]), 1.0) for n in order}
    positions[rail] = ((k - 1) / 2.0, 0.0)
    contacts = {n: (float(pos_idx[n]), 0.0) for n in order}

    routes = {}
    for (a, b, attrs) in edges:
        if rail in (a, b):
            n = b if a == rail else a
            x = float(pos_idx[n])
            routes[(a, b)] = (attrs, [((x, 1.0), (x, 0.0))], 0)  # rung verticale
        else:
            xa, xb = float(pos_idx[a]), float(pos_idx[b])
            routes[(a, b)] = (attrs, [((xa, 1.0), (xb, 1.0))], 0)  # fila in alto

    extra_wires = [((float(i), 0.0), (float(i + 1), 0.0)) for i in range(k - 1)]
    return positions, routes, extra_wires, (rail, 0.0, contacts)


def build_drawing(edges, positions, routes, unit=3.0, extra_wires=None):
    """Costruisce schemdraw.Drawing usando routing pre-calcolato.
    extra_wires: lista di segmenti (in coord grafo) disegnati come fili nudi
    (es. la barra del rail nel layout bus-bar)."""
    d = schemdraw.Drawing()

    def sp(p):
        return (p[0] * unit, p[1] * unit)

    # Fili extra (barra rail) per primi, sotto a tutto
    for (s, e) in (extra_wires or []):
        d += elm.Line().endpoints(sp(s), sp(e))

    # Wire prima (ordine input)
    for (a, b, _) in edges:
        if (a, b) not in routes:
            continue
        _, segs, ci = routes[(a, b)]
        for i, (s, e) in enumerate(segs):
            if i == ci:
                continue
            d += elm.Line().endpoints(sp(s), sp(e))

    # Componenti
    for (a, b, _) in edges:
        if (a, b) not in routes:
            continue
        attrs, segs, ci = routes[(a, b)]
        s, e = segs[ci]
        name = attrs.get("comp", "?")
        val = attrs.get("val", "")
        ctype = attrs.get("type", "R")
        pos_node = attrs.get("pos", a)
        ps, pe = sp(s), sp(e)

        if ctype == "R":
            d += (elm.Resistor()
                  .endpoints(ps, pe)
                  .label(name, loc='top')
                  .label(f'{val}Ω', loc='bottom'))
        else:
            if str(pos_node) == str(a):
                p1, p2 = pe, ps
            else:
                p1, p2 = ps, pe
            d += (elm.SourceV()
                  .endpoints(p1, p2)
                  .label(name, loc='top')
                  .label(f'{val} V', loc='bottom'))

    # Nodi
    for nid, p in positions.items():
        d += elm.Dot().at(sp(p)).label(str(nid), loc='left', color='green')

    return d


def read_cycles(path: Path):
    """Legge file cicli: una riga per ciclo, nodi separati da spazio."""
    if not path.is_file():
        return []
    cycles = []
    for line in path.read_text().splitlines():
        toks = line.split()
        if len(toks) >= 2:
            cycles.append(toks)
    return cycles


def signed_area(pts):
    """Signed area (shoelace). Cartesian y-up: >0 = CCW."""
    n = len(pts)
    s = 0.0
    for i in range(n):
        x1, y1 = pts[i]
        x2, y2 = pts[(i + 1) % n]
        s += x1 * y2 - x2 * y1
    return s / 2.0


def add_background(svg_text, color="white"):
    """Inserisce un <rect> di sfondo coprendo il viewBox."""
    m = re.search(r'viewBox="([\d.\-eE]+) ([\d.\-eE]+) ([\d.\-eE]+) ([\d.\-eE]+)"',
                  svg_text)
    if not m:
        return svg_text
    vx, vy, vw, vh = m.groups()
    rect = (f'<rect x="{vx}" y="{vy}" width="{vw}" height="{vh}" '
            f'fill="{color}"/>')
    # Inserisci subito dopo il tag <svg ...>
    end = svg_text.index('>', svg_text.index('<svg')) + 1
    return svg_text[:end] + rect + svg_text[end:]


def inject_cycles(svg_text, cycles, positions, routes, unit, rail=None):
    """Sovrappone ogni ciclo come polilinea colorata che traccia i lati
    effettivi del ciclo, con offset perp. per cicli che condividono lati.
    Le linee dei cicli vanno SOTTO ai componenti (subito dopo <svg>); le
    label vanno sopra (prima di </svg>). Frecce a fine di ogni segmento.
    rail=(rail_node, rail_y, contacts): nel layout bus-bar il nodo rail e' una
    barra; quando un ciclo passa per il rail si traccia il tratto orizzontale
    lungo la barra fra i due contatti dei vicini nel ciclo."""
    if not cycles:
        return svg_text

    import math

    dot_re = re.compile(r'<circle cx="([^"]+)" cy="([^"]+)" r="2\.6999999999999997"')
    dot_centers = [(float(m.group(1)), float(m.group(2)))
                   for m in dot_re.finditer(svg_text)]
    node_ids = list(positions.keys())
    if len(dot_centers) != len(node_ids):
        return svg_text

    palette = PALETTE

    n0, n1 = node_ids[0], node_ids[1]
    sd0 = (positions[n0][0] * unit, positions[n0][1] * unit)
    sd1 = (positions[n1][0] * unit, positions[n1][1] * unit)
    sv0, sv1 = dot_centers[0], dot_centers[1]
    if sd1[0] != sd0[0]:
        S = (sv1[0] - sv0[0]) / (sd1[0] - sd0[0])
    elif sd1[1] != sd0[1]:
        S = -(sv1[1] - sv0[1]) / (sd1[1] - sd0[1])
    else:
        return svg_text
    Tx = sv0[0] - sd0[0] * S
    Ty = sv0[1] + sd0[1] * S

    def to_svg(gx, gy):
        return (gx * unit * S + Tx, -gy * unit * S + Ty)

    # Mappa lato canonico -> [(cycle_idx, (a,b) come in ciclo)]
    edge_to_cycles = {}
    for ci, cyc in enumerate(cycles):
        n = len(cyc)
        for i in range(n):
            a, b = cyc[i], cyc[(i + 1) % n]
            if a not in positions or b not in positions:
                continue
            key = frozenset({a, b})
            edge_to_cycles.setdefault(key, []).append((ci, (a, b)))

    # Marker arrowhead per colore
    marker_defs = []
    for i, color in enumerate(palette):
        marker_defs.append(
            f'<marker id="cyarrow{i}" viewBox="0 0 10 10" refX="9" refY="5" '
            f'markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
            f'<path d="M 0 0 L 10 5 L 0 10 z" fill="{color}"/></marker>'
        )
    defs_block = "<defs>" + "".join(marker_defs) + "</defs>"

    gap = 5.0   # offset perp in pixel tra cicli sullo stesso lato
    inset = 14  # accorcia segmento ai vertici per non infilare freccia sui Dot
    under_parts = []  # linee colorate -> sotto i componenti
    over_parts = []   # label i_n -> sopra a tutto

    for ci, cyc in enumerate(cycles):
        color = palette[ci % len(palette)]
        marker_id = f"cyarrow{ci % len(palette)}"
        nodes_in = [n for n in cyc if n in positions]
        if len(nodes_in) < 2:
            continue

        n = len(cyc)
        for i in range(n):
            a, b = cyc[i], cyc[(i + 1) % n]
            if a not in positions or b not in positions:
                continue
            key = frozenset({a, b})
            slot_list = edge_to_cycles.get(key, [])
            if not slot_list:
                continue
            slot_idx = next((k for k, (c, _) in enumerate(slot_list) if c == ci), 0)
            n_slots = len(slot_list)
            offset = (slot_idx - (n_slots - 1) / 2.0) * gap

            # Routing salvato puo' essere in (a,b) o (b,a)
            if (a, b) in routes:
                _, segs, _ = routes[(a, b)]
            elif (b, a) in routes:
                _, segs, _ = routes[(b, a)]
            else:
                continue
            seg_iter = list(segs)
            # Oriento i segmenti nel verso a->b usando le posizioni REALI dei
            # nodi: il routing salvato (es. il rung del rail nel bus-bar) puo'
            # non seguire l'ordine della chiave (a,b), il che invertiva le frecce.
            pa, pb = positions.get(a), positions.get(b)
            if pa is not None and pb is not None:
                sx0, sy0 = seg_iter[0][0]
                d_a = abs(sx0 - pa[0]) + abs(sy0 - pa[1])
                d_b = abs(sx0 - pb[0]) + abs(sy0 - pb[1])
                if d_b < d_a:   # il segmento parte vicino a b -> e' al contrario
                    seg_iter = [(e, s) for (s, e) in reversed(seg_iter)]

            for k, (s_g, e_g) in enumerate(seg_iter):
                sx, sy = to_svg(*s_g)
                ex, ey = to_svg(*e_g)
                dx, dy = ex - sx, ey - sy
                L = math.hypot(dx, dy)
                if L == 0:
                    continue
                # perp normalizzato (SVG y-down): ruoto (dx,dy) di 90 CCW -> (-dy, dx)
                ox, oy = -dy / L * offset, dx / L * offset
                # Accorcia agli estremi del primo/ultimo segmento (vicino ai nodi)
                shrink_s = inset if k == 0 else 0
                shrink_e = inset if k == len(seg_iter) - 1 else 0
                ux, uy = dx / L, dy / L
                x1 = sx + ox + ux * shrink_s
                y1 = sy + oy + uy * shrink_s
                x2 = ex + ox - ux * shrink_e
                y2 = ey + oy - uy * shrink_e
                if (x2 - x1) ** 2 + (y2 - y1) ** 2 < 4:
                    continue
                marker_attr = f' marker-end="url(#{marker_id})"' if k == len(seg_iter) - 1 else ''
                under_parts.append(
                    f'<line class="cyc cyc{ci}" '
                    f'x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
                    f'stroke="{color}" stroke-width="3" stroke-opacity="0.6"'
                    f'{marker_attr} />'
                )

        # Tratto lungo la barra del rail (bus-bar): collega i contatti dei due
        # vicini del rail nel ciclo, chiudendo la maglia in orizzontale.
        if rail is not None:
            rail_node, _, contacts = rail
            for i in range(n):
                if cyc[i] != rail_node:
                    continue
                prev, nxt = cyc[(i - 1) % n], cyc[(i + 1) % n]
                if prev not in contacts or nxt not in contacts:
                    continue
                sx, sy = to_svg(*contacts[prev])
                ex, ey = to_svg(*contacts[nxt])
                dx, dy = ex - sx, ey - sy
                L = math.hypot(dx, dy)
                if L == 0:
                    continue
                ux, uy = dx / L, dy / L
                x1, y1 = sx + ux * inset, sy + uy * inset
                x2, y2 = ex - ux * inset, ey - uy * inset
                if (x2 - x1) ** 2 + (y2 - y1) ** 2 < 4:
                    continue
                under_parts.append(
                    f'<line class="cyc cyc{ci}" '
                    f'x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
                    f'stroke="{color}" stroke-width="3" stroke-opacity="0.6" '
                    f'marker-end="url(#{marker_id})" />'
                )

        # Label i_n al centroide
        cx_g = sum(positions[n][0] for n in nodes_in) / len(nodes_in)
        cy_g = sum(positions[n][1] for n in nodes_in) / len(nodes_in)
        cx, cy = to_svg(cx_g, cy_g)
        over_parts.append(
            f'<text class="cyc cyc{ci}" x="{cx}" y="{cy + 5}" '
            f'text-anchor="middle" '
            f'font-family="serif" font-style="italic" font-size="18" '
            f'font-weight="bold" fill="{color}" '
            f'stroke="white" stroke-width="3" paint-order="stroke">'
            f'i<tspan dy="4" font-size="13">{ci + 1}</tspan></text>'
        )

    # Inserisci defs + under sotto al contenuto, label sopra
    under_block = defs_block + "".join(under_parts)
    end_open = svg_text.index('>', svg_text.index('<svg')) + 1
    svg_text = svg_text[:end_open] + under_block + svg_text[end_open:]
    svg_text = svg_text.replace("</svg>", "".join(over_parts) + "</svg>", 1)
    return svg_text


def emit_interactive_html(svg_text, n_cycles, title="Circuito"):
    """Avvolge l'SVG (no-bg, con overlay cicli taggati .cyc/.cycN) in una
    pagina HTML standalone con:
      - toggle per-mesh (default: nessuna mesh visibile -> schema pulito)
      - hover sulla voce legenda -> evidenzia quella mesh temporaneamente
      - pan (drag) e zoom (rotella), reset con doppio click
    Nessuna dipendenza esterna: JS/CSS inline, funziona offline."""
    # id sul tag <svg> per agganciare il pan/zoom
    svg_text = re.sub(r'<svg\b', '<svg id="schematic"', svg_text, count=1)

    # Legenda: una voce per mesh, colore dalla palette condivisa
    legend_items = []
    for i in range(n_cycles):
        color = PALETTE[i % len(PALETTE)]
        legend_items.append(
            f'<button class="mesh-btn" data-cyc="{i}" '
            f'style="--c:{color}">'
            f'<span class="swatch"></span>i<sub>{i + 1}</sub></button>'
        )
    legend_html = "".join(legend_items)
    controls = ""
    if n_cycles:
        controls = f'''
  <div id="controls">
    <span class="lbl">Maglie:</span>
    {legend_html}
    <span class="sep"></span>
    <button id="btn-all">tutte</button>
    <button id="btn-none">nessuna</button>
  </div>'''

    return f'''<!DOCTYPE html>
<html lang="it">
<head>
<meta charset="utf-8">
<title>{title}</title>
<style>
  * {{ box-sizing: border-box; }}
  body {{ margin: 0; font-family: system-ui, sans-serif; background: #f4f4f6;
          height: 100vh; display: flex; flex-direction: column; }}
  #controls {{ flex: 0 0 auto; padding: 8px 12px; background: #fff;
               border-bottom: 1px solid #ddd; display: flex;
               flex-wrap: wrap; gap: 6px; align-items: center; }}
  #controls .lbl {{ font-weight: 600; margin-right: 4px; }}
  #controls .sep {{ flex: 0 0 12px; }}
  button {{ cursor: pointer; border: 1px solid #ccc; background: #fafafa;
            border-radius: 6px; padding: 4px 10px; font-size: 14px; }}
  button:hover {{ background: #eef; }}
  .mesh-btn {{ display: inline-flex; align-items: center; gap: 5px; }}
  .mesh-btn .swatch {{ width: 12px; height: 12px; border-radius: 3px;
                       background: var(--c); display: inline-block; }}
  .mesh-btn.active {{ background: var(--c); color: #fff; border-color: var(--c); }}
  .mesh-btn.active .swatch {{ background: #fff; }}
  #stage {{ flex: 1 1 auto; min-height: 0; overflow: hidden; position: relative;
            background: #fff; }}
  #stage svg {{ width: 100%; height: 100%; display: block;
                cursor: grab; touch-action: none; }}
  #stage svg.grabbing {{ cursor: grabbing; }}
  /* cicli nascosti di default: schema pulito all'apertura */
  #stage .cyc {{ display: none; }}
  #stage .cyc.on {{ display: inline; }}
  #hint {{ position: absolute; bottom: 8px; right: 12px; font-size: 12px;
           color: #888; background: rgba(255,255,255,.8); padding: 2px 6px;
           border-radius: 4px; pointer-events: none; }}
</style>
</head>
<body>
{controls}
  <div id="stage">
    {svg_text}
    <div id="hint">rotella: zoom · trascina: pan · doppio click: reset</div>
  </div>
<script>
(function() {{
  const svg = document.getElementById('schematic');
  if (!svg) return;

  // ---- pan / zoom via viewBox ----
  const vb0 = svg.getAttribute('viewBox').split(/[ ,]+/).map(Number);
  let vb = vb0.slice();
  const setVB = () => svg.setAttribute('viewBox', vb.join(' '));

  function clientToSvg(cx, cy) {{
    const r = svg.getBoundingClientRect();
    return [vb[0] + (cx - r.left) / r.width * vb[2],
            vb[1] + (cy - r.top) / r.height * vb[3]];
  }}
  svg.addEventListener('wheel', (e) => {{
    e.preventDefault();
    const [mx, my] = clientToSvg(e.clientX, e.clientY);
    const k = e.deltaY < 0 ? 0.88 : 1.137;
    vb[0] = mx - (mx - vb[0]) * k;
    vb[1] = my - (my - vb[1]) * k;
    vb[2] *= k; vb[3] *= k;
    setVB();
  }}, {{ passive: false }});

  let drag = null;
  svg.addEventListener('pointerdown', (e) => {{
    drag = {{ x: e.clientX, y: e.clientY }};
    svg.classList.add('grabbing');
    svg.setPointerCapture(e.pointerId);
  }});
  svg.addEventListener('pointermove', (e) => {{
    if (!drag) return;
    const r = svg.getBoundingClientRect();
    vb[0] -= (e.clientX - drag.x) / r.width * vb[2];
    vb[1] -= (e.clientY - drag.y) / r.height * vb[3];
    drag.x = e.clientX; drag.y = e.clientY;
    setVB();
  }});
  const endDrag = () => {{ drag = null; svg.classList.remove('grabbing'); }};
  svg.addEventListener('pointerup', endDrag);
  svg.addEventListener('pointercancel', endDrag);
  svg.addEventListener('dblclick', () => {{ vb = vb0.slice(); setVB(); }});

  // ---- toggle mesh ----
  const active = new Set();
  function applyCyc(i, on) {{
    svg.querySelectorAll('.cyc' + i).forEach(el => el.classList.toggle('on', on));
  }}
  function refresh() {{
    document.querySelectorAll('.mesh-btn').forEach(b => {{
      const i = +b.dataset.cyc;
      b.classList.toggle('active', active.has(i));
    }});
  }}
  document.querySelectorAll('.mesh-btn').forEach(b => {{
    const i = +b.dataset.cyc;
    b.addEventListener('click', () => {{
      if (active.has(i)) {{ active.delete(i); applyCyc(i, false); }}
      else {{ active.add(i); applyCyc(i, true); }}
      refresh();
    }});
    // hover: anteprima temporanea senza alterare lo stato attivo
    b.addEventListener('mouseenter', () => {{ if (!active.has(i)) applyCyc(i, true); }});
    b.addEventListener('mouseleave', () => {{ if (!active.has(i)) applyCyc(i, false); }});
  }});
  const allBtn = document.getElementById('btn-all');
  const noneBtn = document.getElementById('btn-none');
  if (allBtn) allBtn.addEventListener('click', () => {{
    document.querySelectorAll('.mesh-btn').forEach(b => {{
      const i = +b.dataset.cyc; active.add(i); applyCyc(i, true);
    }});
    refresh();
  }});
  if (noneBtn) noneBtn.addEventListener('click', () => {{
    active.forEach(i => applyCyc(i, false)); active.clear(); refresh();
  }});
}})();
</script>
</body>
</html>
'''


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("dotfile", type=Path)
    p.add_argument("-o", "--output", type=Path)
    p.add_argument("--unit", type=float, default=3.0,
                   help="fattore scala grid->schemdraw (default 3.0)")
    p.add_argument("--cycles", type=Path,
                   help="file cicli (default: <stem>.cycles.txt accanto al .dot)")
    p.add_argument("--engine", default="neato",
                   help="motore layout Graphviz (solo --layout grid): neato "
                        "(default, compatto), sfdp, dot, fdp, circo")
    p.add_argument("--layout", default="auto",
                   choices=["auto", "ortho", "planar", "grid"],
                   help="auto (default)=ortho se planare/biconnesso, altrimenti "
                        "grid; ortho: planare con angoli retti dove il grado lo "
                        "permette, diagonali sugli hub; planar: planare tutto "
                        "diagonale; grid: vecchio neato+manhattan")
    p.add_argument("--no-html", action="store_true",
                   help="non generare il file HTML interattivo")
    args = p.parse_args()

    if not args.dotfile.is_file():
        sys.exit(f"File non trovato: {args.dotfile}")

    edges = parse_dot(args.dotfile)

    positions = None
    extra_wires = None
    rail_meta = None
    # 1) bus-bar: se il circuito e' un fan (rail + cammino) lo disegno come
    #    barra orizzontale con derivazioni verticali -> rettangolo a maglie.
    if args.layout in ("auto", "ortho"):
        fan = detect_fan(edges)
        if fan is not None:
            rail, order = fan
            positions, routes, extra_wires, rail_meta = fan_layout(
                edges, rail, order)
            print(f"Layout: bus-bar (rail {rail}, {len(order)} derivazioni, "
                  f"{len(order) - 1} maglie)")
    if positions is None and args.layout in ("auto", "ortho", "planar"):
        positions = planar_positions(edges)
        if positions is not None:
            if args.layout == "planar":
                routes = straight_routes(edges, positions)
                print("Layout: planare crossing-free (Tutte, diagonale)")
            else:
                routes = hybrid_routes(edges, positions)
                n_ortho = sum(1 for _, ss, _ in routes.values() if len(ss) > 1
                              or ss[0][0][0] == ss[0][1][0]
                              or ss[0][0][1] == ss[0][1][1])
                print(f"Layout: planare crossing-free (ortho {n_ortho}/"
                      f"{len(routes)} archi, resto diagonale sugli hub)")
        elif args.layout in ("ortho", "planar"):
            sys.exit("Layout planare non disponibile: grafo non planare/"
                     "biconnesso, oppure mancano networkx/numpy.")
    if positions is None:
        raw = run_layout_plain(args.dotfile, args.engine)
        positions = snap_to_grid(raw)
        routes = compute_routes(edges, positions)
        print(f"Layout: grid (engine {args.engine})")

    d = build_drawing(edges, positions, routes, unit=args.unit,
                      extra_wires=extra_wires)
    if args.output:
        out = args.output
    else:
        stem = args.dotfile.name
        if stem.endswith(".tikz.dot"):
            stem = stem[:-len(".tikz.dot")]
        elif stem.endswith(".dot"):
            stem = stem[:-len(".dot")]
        out = args.dotfile.parent / f"{stem}.svg"
    d.save(str(out))

    if args.cycles:
        cycles_path = args.cycles
    else:
        stem = args.dotfile.name
        if stem.endswith(".tikz.dot"):
            stem = stem[:-len(".tikz.dot")]
        elif stem.endswith(".dot"):
            stem = stem[:-len(".dot")]
        cycles_path = args.dotfile.parent / f"{stem}.cycles.txt"
    cycles = read_cycles(cycles_path)
    txt = Path(out).read_text()
    if cycles:
        txt = inject_cycles(txt, cycles, positions, routes, args.unit,
                            rail=rail_meta)

    # versione senza sfondo (trasparente): scrivila prima di iniettare il rect
    out = Path(out)
    nobg_out = out.with_name(f"{out.stem}_nobg{out.suffix}")
    nobg_out.write_text(txt)
    svg_for_html = txt  # no-bg + cicli taggati: ideale per l'embed HTML

    # background per ultimo cosi' finisce subito dopo <svg> (sotto a tutto)
    txt = add_background(txt, "white")
    out.write_text(txt)
    if cycles:
        print(f"Generato: {out}  (+{len(cycles)} cicli)")
    else:
        print(f"Generato: {out}")
    print(f"Generato: {nobg_out}  (senza sfondo)")

    # HTML interattivo (toggle maglie + pan/zoom)
    if not args.no_html:
        html_out = out.with_suffix(".html")
        html = emit_interactive_html(svg_for_html, len(cycles),
                                     title=out.stem)
        html_out.write_text(html)
        print(f"Generato: {html_out}  (interattivo)")


if __name__ == "__main__":
    main()
