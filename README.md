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
Eigen) integrata in CTest. Si compila insieme al resto e si esegue con:
```sh
cd build
ctest --output-on-failure      # oppure ./Tests per l'output dettagliato
```
Per la sola consegna i test si possono escludere con `cmake .. -DBUILD_TESTS=OFF`.

Contenuto:
- `test/test_main.cpp` — harness con macro `CHECK`/`CHECK_NEAR`, ritorna codice
  d'errore != 0 se almeno un check fallisce.
- `test/netlists/` — netlist di input usate dai test.

Cosa viene verificato:
- **Golden (valori dalla specifica)**: netlist della sez. 7 del PDF (tensioni e
  correnti su tutti i resistori, per *entrambi* i metodi DFS e De Pina); esempio
  a 2 maglie della sez. 4 (moduli delle tensioni `10/11, 100/11, 120/11`); maglia
  singola in serie (`I = 12/(1+2) = 4 A`).
- **Proprietà / invarianti** (senza valori calcolati a mano): numero di maglie
  `= |E| - |V| + 1`; accordo tra DFS e De Pina sulle tensioni fisiche dei
  resistori; residuo del sistema `||B^T R B i - v|| ≈ 0`; stessa corrente in serie.
- **Robustezza del parser**: spazi multipli, tab e righe vuote non cambiano la
  soluzione (`messy_sec7.txt` vs `example_sec7.txt`).
- **Gestione errori**: riga malformata, tipo componente non valido e file mancante
  sollevano un'eccezione (niente crash silenzioso); un secondo componente sullo
  stesso arco (ramo in parallelo) viene rifiutato.