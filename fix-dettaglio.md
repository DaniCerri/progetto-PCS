Tutto ciò che ho toccato, in dettaglio. Due gruppi: **refactor** (committato, `746fd99`) e **robustezza** (in locale, non committato).

---

# GRUPPO 1 — Refactor matrici/cicli (committato)

## 1. Bug latente De Pina — la radice

**Cos'era.** `build_matrices` riempiva la matrice di incidenza **B** camminando ogni ciclo nodo per nodo:

```cpp
T cur = (first.from() == last.from() || first.from() == last.to())
            ? first.from() : first.to();        // euristica nodo di partenza
for (const auto& e : cycle) {
    const int dir = (e.from() == cur) ? +1 : -1; // segno
    const T nxt = (e.from() == cur) ? e.to() : e.from();
    cur = nxt;                                    // avanza
}
```

Due assunzioni implicite: (a) `first`/`last` condividono il nodo di chiusura; (b) ogni arco successivo è **incidente** a `cur`.

**Perché rompeva.** I due cycle-finder davano gli archi in ordini diversi:
- **DFS**: archi in ordine di **cammino** (adiacenti) → assunzioni vere.
- **De Pina**: archi in ordine **lessicografico** (è un set di incidenza, vedi `path_buffer` riempito scorrendo il bool vector per indice). Su maglie ≥4 archi l'ordine lex **non è un cammino**.

Quando un arco non era incidente a `cur`, il ramo `else` scattava comunque (`dir=-1`, `nxt=e.from()`): `cur` **si teletrasportava** su un nodo non collegato → da lì i segni erano spazzatura.

**Esempio quadrato** (nodi 1-2-3-4), lex `(1,2)(1,4)(2,3)(3,4)`:
- start: `(1,2)`/`(3,4)` non condividono nulla → `cur=2`
- `(1,2)`: 1≠2 → −1, cur→1
- `(1,4)`: 1=1 → +1, cur→4
- `(2,3)`: 2≠4 → **teletrasporto**, −1, cur→2
- `(3,4)`: 3≠2 → −1, cur→3

Fine `cur=3 ≠ start=2` → il ciclo **non chiude**. Colonna di B incoerente (non è ±la vera) → BᵀRB sbagliata → correnti/tensioni sbagliate.

**Perché "latente"** (mai visto): default = DFS; De Pina solo con arg `depina`. E se le maglie sono triangoli (3 archi tra 3 nodi formano sempre un cammino qualunque ordine), l'ordine lex coincide per caso → bug dorme. Si sveglia solo con `depina` **e** maglia ≥4 archi.

**Nota tecnica importante**: un'inversione *pulita* di un'intera maglia (tutta al contrario) sarebbe **innocua** — colonna B e termine noto v(j) cambiano segno insieme → equazione ×(−1) → stessa soluzione. Il problema era che i segni erano **incoerenti** (misti), non un flip pulito.

## 2. La regola della consegna che abilita il fix

PDF: due orientamenti distinti.
- **Arco**: verso di riferimento **fisso** = nodo minore→maggiore. Già forzato dal costruttore `UnidirectedEdge` (`from<to`).
- **Maglia**: data dall'**ordine dei nodi** nel ciclo `C_j=(v0,v1,...)`.

`B_ij = +1` se la maglia percorre l'arco nel suo verso (basso→alto), `−1` se opposto, `0` se assente.

Conseguenza: se la maglia percorre `a→b`, il segno è **solo** `(a<b)?+1:−1`. Comparazione O(1), niente euristica, niente walk.

**Ma**: serve comunque la **sequenza ordinata di nodi**. Da un set di archi non ordinato non puoi assegnare i segni — andando intorno al loop alcuni archi vanno per forza alto→basso, e quale "torna indietro" lo sai solo conoscendo l'ordine ciclico.

## 3. Cicli come sequenze di nodi

Cambiata la rappresentazione: `vector<vector<UnidirectedEdge<T>>>` → `vector<vector<T>>` (sequenza nodi, senza duplicato di chiusura; la maglia chiude in **wrap-around**).

**DFS** (`dfs_based.hpp`) — `find_path` già cammina nodo per nodo. Ricostruisco la sequenza dal cammino ordinato dell'albero:
```cpp
T cur = edge.from();
nodes.push_back(cur);
for (const auto& e : path_buffer) {      // archi albero, ordinati e adiacenti
    cur = (e.from() == cur) ? e.to() : e.from();
    nodes.push_back(cur);                 // ...fino a edge.to()
}
// la corda (edge.to()->edge.from()) chiude in wrap-around, NON la aggiungo
```
Qui il walk è **sicuro**: gli archi dell'albero sono genuinamente ordinati e adiacenti (a differenza del set lex di De Pina).

**De Pina** (`de_pina.hpp` + `dijkstra.hpp`) — fix **alla sorgente**: Dijkstra sul grafo sollevato calcola già un cammino minimo, e la catena `pred` **è ordinata**. Prima veniva appiattita nel bool vector (`find_incidence_vector`), buttando l'ordine. Ora estraggo direttamente la sequenza nodi:
```cpp
// extract_cycle_nodes: cammino pred da v- a v+, proietto sui nodi originali
auto current = end_node;
while (true) {
    nodes.push_back(current.first);       // proietto (T,bool) -> T
    if (current == start_node) break;
    current = pred.at(current);
}
std::reverse(nodes.begin(), nodes.end());
if (nodes.front() == nodes.back()) nodes.pop_back();  // v+ e v- proiettano sullo stesso v
```
Il bookkeeping GF(2) di De Pina (prodotto scalare `C[i]·S[j]` mod 2) ha ancora bisogno dell'incidenza: la ricavo dalla sequenza con `cycle_to_incidence`. Il resto dell'algoritmo (XOR di `S[j]` con `S[i]`) invariato.

## 4. build_matrix riscritto

```cpp
const std::set<UnidirectedEdge<T>> edge_set = graph.all_edges();  // lex-ordinato
// righe: resistori in ordine lessicografico
for (const auto& e : edge_set)
    if (e.get_component().is_resistor()) { row_of[e]=...; resistor_branches.push_back(e); }

for (size_t j = 0; j < n; ++j) {
    const auto& nodes = fundamental_cycles_out[j];
    const size_t k = nodes.size();
    for (size_t pos = 0; pos < k; ++pos) {
        const T a = nodes[pos], b = nodes[(pos+1)%k];   // wrap-around chiude
        const int dir = (a < b) ? +1 : -1;              // REGOLA PDF, O(1)
        const UnidirectedEdge<T> key(a, b);             // costruttore normalizza
        auto it = edge_set.find(key);                   // O(log m), risale al componente
        const Component& c = it->get_component();
        if (c.is_resistor()) B(row_of.at(key), j) += dir;
        else v(j) += (c.get_positive_node() == b ? +1 : -1) * c.get_value();
    }
}
```
Sparito: euristica nodo di partenza, walk con teletrasporto, ricostruzione doppia. Le **righe** vengono dall'ordine lessicografico (`std::set`), come da consegna.

**Termine noto generatore**: usciamo dall'arco in `b`; se `b` è il nodo positivo → attraversato `−`→`+` → contributo `+`. Identico alla logica vecchia, solo con `b` esplicito invece di `nxt`.

## 5. Lista di adiacenza nel grafo

`UnidirectedGraph` ora: `vector<edges>` (verità, ordine inserimento) **+** `map<T, vector<size_t>> adj` (nodo → indici archi).

| Metodo | Prima | Dopo |
|--------|-------|------|
| `neighbours(u)` | O(m) scan tutti archi | O(deg) via `adj[u]` |
| `incident_edges(u)` | O(m) | O(deg) |
| `add_edge` dup-check | O(m) su tutti | O(deg) su `adj[from]` |

`all_edges()` resta `std::set` lex (contratto righe matrice). `operator-`/`graph_visit` ricostruiscono via `add_edge` → adj coerente automaticamente.

**Perché lista e non matrice**: T è generico — il grafo sollevato di De Pina usa `pair<T,bool>` come nodo; la matrice avrebbe richiesto comunque `map<T,index>` + O(V²) memoria su grafo sparsissimo.

## 6. Component default ctor (fix di compilazione)

`UnidirectedEdge<T> key(a,b)` (costruttore 2-arg, usato per le chiavi di lookup) lascia `component` default-costruito. `Component` non aveva default ctor → errore di compilazione. Aggiunto:
```cpp
Component() : name(""), value(0.0), positive_node(0) {}
```
Le chiavi non leggono mai il componente; lo zero solo per pulizia.

## 7. Rimosso `find_incidence_vector`

Codice morto dopo che De Pina usa `extract_cycle_nodes`. Tolto da `dijkstra.hpp`.

---

# GRUPPO 2 — Robustezza (in locale, NON committato)

## 8. `visited` indicizzato per valore → crash su nodi non contigui

**Cos'era.** `find_path` (DFS):
```cpp
std::vector<bool> visited(num_nodes + 1, false);
visited[u] = true;          // u = VALORE del nodo
if (!visited[neighbor]) ...
```
`visited` dimensionato `num_nodes+1`, ma indicizzato col **valore** del nodo. Netlist con 5 nodi ma etichettati `{10,20,30,40,50}`: `visited[10]` su un vettore di dimensione 6 → **out-of-bounds** (UB, `vector<bool>::operator[]` non controlla i limiti) → crash o corruzione silenziosa.

**Perché conta**: la consegna non garantisce etichette contigue/1-based, e i docenti girano il **loro** input.

**Fix**: contenitore associativo indicizzato per valore.
```cpp
std::set<T> visited;
visited.insert(u);
if (!visited.contains(neighbor)) ...
```
**Provato**: netlist con nodi `{10,20,30,40,50}` → ora gira corretto, stessi valori R (le etichette sono solo nomi, la fisica non cambia).

## 9. Vettore incognite non inizializzato

**Cos'era.** `main.cpp`:
```cpp
Eigen::VectorXd i(essential_cycles.size());   // Eigen NON azzera
gradiente_coniugato(BtRB, v, i, 1e-10);
```
Eigen non inizializza la memoria → `i` contiene valori **indeterminati**. Il CG calcola `r = b - A*x` partendo da `x` garbage. Per una matrice SPD converge comunque, ma può partire lontanissimo (più iterazioni) o, con valori patologici (NaN/inf in memoria), non convergere.

**Fix**: `i.setZero();` → parte da `x0 = 0`.

## 10. CG senza cap iterazioni + divisione per zero

**Cos'era.**
```cpp
while(r.norm() > r_tol) {
    const double alpha_k = (...) / (p.transpose() * A * p).value();
    ...
}
```
Nessun limite di iterazioni → se non converge (matrice non SPD per un bug a monte, o residuo che non scende sotto `r_tol` per arrotondamento) → **loop infinito**. E il denominatore `pᵀAp` non è guardato.

**Fix**:
```cpp
const unsigned int max_iter = 2u * static_cast<unsigned int>(b.size()) + 50u;
while(r.norm() > r_tol && k < max_iter) {
    const double denom = (p.transpose() * A * p).value();
    if (denom == 0.0) break;          // direzione A-ortogonale: niente progresso
    const double alpha_k = (...) / denom;
    ...
    const double beta_k = (...) / denom;  // riuso, evito di ricalcolare pᵀAp
}
```
CG su SPD converge in ≤n passi → cap `2n+50` è margine ampio per gli arrotondamenti. Bonus: `denom` calcolato una volta sola invece di due.

---

# Verifica

Tutto buildato pulito nel container. Output su `Netlist.txt` = **PDF §7 esatto** (`R1 V=8 I=2 · R2 22/2.2 · R3 −6/−0.2 · R4 −28/−2.8 · R5 12/3`), **identico** tra `dfs` e `depina`. Conferma chiave: la terza maglia `2 1 3 5` ha **4 archi** — proprio il caso che svegliava il bug De Pina → ora corretto.
