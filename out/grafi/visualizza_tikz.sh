#!/usr/bin/env bash
# visualizza_tikz.sh
# Pipeline completa: .tikz.dot -> .tex (CircuiTikZ) -> .pdf
#
# Uso: ./visualizza_tikz.sh [cartella_con_i_dot]

set -e

# di default i .dot sono nella cartella out (un livello sopra a questa)
DOT_DIR="${1:-..}"

# Verifica dipendenze
for cmd in neato python3 pdflatex; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "Comando '$cmd' non trovato. Installa le dipendenze:"
        echo "  sudo apt install graphviz python3 texlive-latex-extra texlive-pictures"
        exit 1
    fi
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONV="$SCRIPT_DIR/dot_to_tikz.py"

shopt -s nullglob
FILES=("$DOT_DIR"/*.tikz.dot)
if [ ${#FILES[@]} -eq 0 ]; then
    echo "Nessun file .tikz.dot in $DOT_DIR"
    exit 1
fi

for dotfile in "${FILES[@]}"; do
    base="${dotfile%.tikz.dot}"
    tex="${base}.tex"
    python3 "$CONV" "$dotfile" -o "$tex"
    pdflatex -interaction=nonstopmode -output-directory "$DOT_DIR" "$tex" > /dev/null
    echo "Generato: ${base}.pdf"
done

# Apri i pdf
for dotfile in "${FILES[@]}"; do
    pdffile="${dotfile%.tikz.dot}.pdf"
    [ -f "$pdffile" ] && xdg-open "$pdffile" &
done

echo "Fatto!"
