#!/usr/bin/env bash
# visualizza_grafi.sh
# Converte i file .tikz.dot in SVG + HTML interattivo (via schemdraw) e li
# apre nel browser. L'HTML ha toggle per-maglia e pan/zoom.
#
# Uso: ./visualizza_grafi.sh [cartella_con_i_dot]

set -e

# di default i .dot sono nella cartella out (un livello sopra a questa)
DOT_DIR="${1:-..}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_PY="$SCRIPT_DIR/.venv/bin/python"

if [ ! -x "$VENV_PY" ]; then
    echo "Venv non trovato in $SCRIPT_DIR/.venv"
    echo "Crealo con:  uv venv $SCRIPT_DIR/.venv && uv pip install --python $SCRIPT_DIR/.venv/bin/python schemdraw networkx numpy"
    exit 1
fi

if ! command -v neato &> /dev/null; then
    echo "'neato' non trovato. Installa graphviz: sudo apt install graphviz"
    exit 1
fi

CONV="$SCRIPT_DIR/dot_to_schemdraw.py"
# out/05_mega.cycles.txt out/05_mega.dot out/05_mega.tikz.dot
# prende tutti i .tikz.dot presenti nella cartella (uno per circuito)
shopt -s nullglob
FILES=("$DOT_DIR"/*.tikz.dot)
if [ ${#FILES[@]} -eq 0 ]; then
    echo "Nessun file .tikz.dot in $DOT_DIR (hai eseguito l'eseguibile?)"
    exit 1
fi

for path in "${FILES[@]}"; do
    base="${path%.tikz.dot}"
    svg="${base}.svg"
    "$VENV_PY" "$CONV" "$path" -o "$svg"
    echo "Generato: $svg"
done

# apre la versione HTML interattiva (toggle maglie + pan/zoom);
# se manca ripiega sull'SVG statico
for path in "${FILES[@]}"; do
    base="${path%.tikz.dot}"
    if [ -f "${base}.html" ]; then
        xdg-open "${base}.html" &
    elif [ -f "${base}.svg" ]; then
        xdg-open "${base}.svg" &
    fi
done

echo "Fatto!"
