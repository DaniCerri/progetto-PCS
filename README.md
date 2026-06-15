# Cartella condivisa per il progetto PCS di Polito anno accademico 2025/2026

Il codice è fatto per essere eseguito dentro il container Docker fornito dal docente. 


## Struttura
- input/ Cartella con i file .txt con le netlist
- source/ Contiene il codice sorgente .cpp
- out/ Risultati
- test/ Suite di test automatici (vedi sotto)
- CMakeLists.txt  File per la compilazione

## Compilazione ed esecuzione
```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
./Eseguibile <file_input> [file_output] [dfs|depina] [-v]
```
Argomenti opzionali in qualsiasi ordine: `dfs`/`depina` sceglie l'algoritmo dei
cicli (default `dfs`), `-v` stampa i dump diagnostici (matrici, maglie, iterazioni)
su `stderr`, qualunque altro token è il file di output. Su `stdout` resta solo la
lista delle tensioni/correnti sui resistori, nel formato della specifica.

## Test
La cartella `test/` contiene una suite automatica (nessun framework esterno, solo
Eigen) integrata in CTest, divisa per area funzionale: un eseguibile per area, un
test CTest per area. Si compila insieme al resto ed eseguono tutti con:
```sh
cd build
make            # (ri)compila anche i test
make test       # esegue tutte le suite (parser, graph, cycles, solver)
ctest --output-on-failure   # idem, con dettaglio dei check falliti
./test_solver               # in alternativa, una singola suite
```
Per la sola consegna i test si possono escludere con `cmake .. -DBUILD_TESTS=OFF`.

Struttura:
- `test/test_harness.hpp`, harness condiviso: macro `CHECK`/`CHECK_NEAR`, helper
  `load`/`solve`/`throws`; `report()` ritorna codice != 0 se un check fallisce.
- `test/test_parser.cpp`: parsing della netlist.
- `test/test_graph.cpp`: struttura dati `UnidirectedGraph`.
- `test/test_cycles.cpp`: individuazione delle maglie (DFS e De Pina).
- `test/test_solver.cpp`: sistema lineare e tensioni.
- `test/netlists/`: netlist di input usate dai test.

Cosa viene verificato:
- **parser**: robustezza a spazi/tab/righe vuote (`messy_sec7.txt` vs
  `example_sec7.txt`), classificazione tipo R/V, errori (riga malformata, tipo non
  valido, file mancante) che sollevano eccezione invece di crashare.
- **graph**: normalizzazione `from<to`, inserimento automatico dei nodi, rifiuto
  di archi duplicati (rami in parallelo), `neighbours`/`incident_edges`,
  numerazione archi, differenza fra grafi (co-albero).
- **cycles**: numero di maglie `= |E| - |V| + 1`; ogni maglia è un cammino chiuso
  valido (archi consecutivi esistenti, chiusura wrap-around); accordo fra DFS e
  De Pina sulle grandezze fisiche dei resistori.
- **solver**: golden dalla specifica, sez. 7 del PDF (V e I su tutti i resistori,
  entrambi i metodi), esempio a 2 maglie (moduli `10/11, 100/11, 120/11`), maglia
  singola in serie (`I = 12/(1+2) = 4 A`); residuo `||B^T R B i - v|| ≈ 0`.

## Uso di strumenti di IA
Per lo sviluppo ci siamo avvalsi di strumenti di intelligenza artificiale per due
parti accessorie del progetto:
- le funzioni di **rappresentazione grafica dei circuiti** (serializzazione in
  formato DOT/GraphViz, `source/unidirected_graph/dot_serializer.hpp`);
- alcune **netlist di test** in `test/netlists/`.

Il nucleo algoritmico (parser, grafo, individuazione delle maglie, assemblaggio
delle matrici e gradiente coniugato) è stato sviluppato da noi.