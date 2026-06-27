# FM-Index con Wavelet Trees para Búsqueda Genómica

Motor de búsqueda de patrones sobre secuencias genómicas usando FM-Index y Wavelet Trees. Proyecto final de CS3014 - Estructura de Datos Avanzados, UTEC 2026-1.

## Problema

Buscar millones de fragmentos cortos (reads) dentro de un genoma de referencia. El Suffix Array clásico requiere ~16 GB para el genoma humano. El FM-Index reduce eso a ~1.2 GB y las operaciones por query de 4,725 a 300.

## Arquitectura

```
BitVector        →  rank1/rank0 en O(1), bloques de 64 bits con popcount
WaveletTree      →  rank(i, c) en O(log σ), divide alfabeto por niveles
FMIndex          →  SA (prefix doubling) → BWT → Tabla C → Wavelet Tree
                     backward search: O(m · log σ) por query
```

## Estructura del proyecto

```
ProyectoFinal/
├── src/
│   ├── main.cpp           # Tests y benchmarks
│   ├── FMIndex.hpp         # Clase principal, orquesta el pipeline
│   ├── WaveletTree.hpp     # Wavelet Tree sobre la BWT
│   ├── BitVector.hpp       # Bitvector con rank O(1)
│   └── utils.hpp           # Carga FASTA, timer
├── Docs/
├── GCF_000005845.2_ASM584v2_genomic.fna/
│   └── GCF_000005845.2_ASM584v2_genomic.fna   # Genoma E. coli K-12
└── CMakeLists.txt
```

## Compilación

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ProyectoFinal
```

Requiere C++20. Compatible con GCC, Clang y MSVC.

## Resultados (E. coli K-12, 4.6M bases)

| Métrica | Valor |
|---|---|
| Construcción del índice | 78 s |
| Tiempo promedio por query (m=150) | 38 μs |
| Throughput | 26,134 queries/s |
| 100K reads benchmark | 3.8 s |

## Dataset

Genoma de *Escherichia coli* str. K-12 substr. MG1655 (NC_000913.3), descargado del [NCBI](https://www.ncbi.nlm.nih.gov/nuccore/NC_000913.3). Archivo FASTA, 4,641,652 pares de bases.

## Autores

- Joel David Miguel Fernandez — 202310186
- Paris Lenard Herrera Torres — 202310100
- Ary Werner Aaron Rojas Durand — 202310366

Profesor: Luciano Arnaldo Romero Calla | Profesor: Victor Racso Galvan Oyola
