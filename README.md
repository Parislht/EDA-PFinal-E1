# FM-Index con Wavelet Trees para Búsqueda Genómica

Motor de búsqueda de patrones sobre secuencias genómicas usando **FM-Index** y **Wavelet Trees**, con construcción del Suffix Array en tiempo lineal mediante **DC3** y una **demo visual web** servida desde el propio ejecutable en C++.

Proyecto final de **CS3014 – Estructura de Datos Avanzados**, UTEC 2026-1.

**Autores:**
- Joel David Miguel Fernandez – 202310186
- Paris Lenard Herrera Torres – 202310100

---

## 1. El problema

Alinear millones de fragmentos cortos (*reads*) contra un genoma de referencia. El genoma humano tiene ~3.2 × 10⁹ bases, y el Suffix Array clásico requiere ~16 GB de memoria y ~4,725 comparaciones por consulta. El FM-Index reduce el espacio a ~1.2 GB y las operaciones por consulta a ~300, sin almacenar el texto original ni el Suffix Array completo para consultar.

---

## 2. Componentes implementados

```
BitVector    →  rank1/rank0 y access en O(1); bloques de 64 bits con popcount portátil
WaveletTree  →  rank(i, c) en O(log σ); descompone el alfabeto por niveles de bitvectors
FMIndex      →  pipeline SA → BWT → Tabla C → Wavelet Tree
                backward search: O(m · log σ) por consulta
DC3          →  construcción del Suffix Array en O(n) (Kärkkäinen & Sanders, 2003)
```

### Novedades de esta entrega

- **DC3 (algoritmo skew):** construcción del Suffix Array en tiempo lineal O(n), seleccionable por parámetro. Se mantiene *prefix doubling* (O(n log²n)) como método alternativo para comparar ambos empíricamente. DC3 resulta hasta **9× más rápido** a escala de decenas de millones de bases.
- **Demo visual web:** interfaz servida directamente desde el ejecutable C++ mediante la librería header-only [cpp-httplib](https://github.com/yhirose/cpp-httplib). Permite construir el índice sobre una cadena y visualizar todas las estructuras finales (SA, BWT, Tabla C, Wavelet Tree), además de animar el backward search paso a paso.
- **Serialización JSON** (`json_export.hpp`) de las estructuras y de la traza de búsqueda, sin modificar el camino de ejecución de las consultas (`count`/`locate` permanecen puros para no contaminar los benchmarks).
- **Benchmarks a escala real:** validación desde *E. coli* K-12 (4.6M bases) hasta el cromosoma 1 humano (230M bases).

---

## 3. Estructura del proyecto

```
ProyectoFinal/
├── src/
│   ├── main.cpp            # Tests de verificación (GATAGACA, E. coli)
│   ├── benchmark_main.cpp  # Benchmarks: DC3 vs Prefix Doubling, throughput, escala real
│   ├── server_main.cpp     # Servidor HTTP de la demo visual
│   ├── FMIndex.hpp          # Clase orquestadora del pipeline
│   ├── WaveletTree.hpp      # Wavelet Tree sobre la BWT
│   ├── BitVector.hpp        # Bitvector con rank O(1)
│   ├── DC3.hpp              # Construcción del Suffix Array en O(n)
│   ├── json_export.hpp      # Serialización de estructuras y trazas a JSON
│   └── utils.hpp            # Carga de archivos FASTA y cronómetro
├── web/
│   ├── index.html          # Interfaz de la demo
│   ├── style.css           # Estilos
│   └── app.js              # Lógica del frontend (fetch + animación)
├── third_party/
│   └── httplib.h           # cpp-httplib (servidor HTTP header-only)
├── data/
│   └── GCF_000005845.2_ASM584v2_genomic.fna   # Genoma de E. coli K-12
├── Docs/                   # Informe y documentación
├── Imgs/                   # Capturas de resultados
└── CMakeLists.txt
```

Los genomas humanos (`chr1.fa`, `chr21.fa`) **no** se incluyen en el repositorio por su tamaño; se descargan con los comandos de la sección 6.

---

## 4. Dependencias

- **Compilador C++20** (probado con MinGW/GCC en Windows).
- **CMake** ≥ 3.20.
- **cpp-httplib** (incluido en `third_party/httplib.h`, no requiere instalación).
- No se requieren librerías externas adicionales.

---

## 5. Compilación y ejecución

El proyecto genera **tres ejecutables** independientes.

### Con CMake (línea de comandos)

```bash
# Configurar en modo Release (importante para los benchmarks)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Con CLion

Abrir el proyecto, seleccionar el perfil **Release** (no Debug) para medir rendimiento, y elegir el ejecutable deseado en el selector de configuración.

### Los tres ejecutables

| Ejecutable       | Fuente               | Propósito                                                        |
|------------------|----------------------|-----------------------------------------------------------------|
| `ProyectoFinal`  | `main.cpp`           | Tests de verificación (GATAGACA + E. coli)                      |
| `benchmark`      | `benchmark_main.cpp` | Benchmarks de construcción y búsqueda a distintas escalas       |
| `server`         | `server_main.cpp`    | Servidor de la demo visual                                       |

**Nota:** compilar en **Release** es esencial para los benchmarks. En Debug, la construcción del Suffix Array puede ser decenas de veces más lenta.

---

## 6. Descarga de los genomas

### E. coli K-12 (incluido)

Ya está en `data/GCF_000005845.2_ASM584v2_genomic.fna`. No requiere descarga.

### Cromosomas humanos (opcionales, para benchmarks de escala real)

Se descargan desde el UCSC Genome Browser (assembly GRCh38/hg38). Los comandos siguientes son para **PowerShell en Windows**.

**Cromosoma 21 (~40M bases, ~1 GB de RAM):**

```powershell
cd data
curl.exe -L -o chr21.fa.gz "https://hgdownload.soe.ucsc.edu/goldenpath/hg38/chromosomes/chr21.fa.gz"

$in  = "chr21.fa.gz"; $out = "chr21.fa"
$inStream  = [System.IO.File]::OpenRead((Resolve-Path $in))
$gzip      = New-Object System.IO.Compression.GzipStream($inStream, [System.IO.Compression.CompressionMode]::Decompress)
$outStream = [System.IO.File]::Create((Join-Path (Get-Location) $out))
$gzip.CopyTo($outStream)
$gzip.Close(); $inStream.Close(); $outStream.Close()
Remove-Item chr21.fa.gz
```

**Cromosoma 1 (~230M bases, ~6 GB de RAM):**

```powershell
cd data
curl.exe -L -o chr1.fa.gz "https://hgdownload.soe.ucsc.edu/goldenpath/hg38/chromosomes/chr1.fa.gz"

$in  = "chr1.fa.gz"; $out = "chr1.fa"
$inStream  = [System.IO.File]::OpenRead((Resolve-Path $in))
$gzip      = New-Object System.IO.Compression.GzipStream($inStream, [System.IO.Compression.CompressionMode]::Decompress)
$outStream = [System.IO.File]::Create((Join-Path (Get-Location) $out))
$gzip.CopyTo($outStream)
$gzip.Close(); $inStream.Close(); $outStream.Close()
Remove-Item chr1.fa.gz
```

En **Linux/macOS** basta con:

```bash
cd data
wget https://hgdownload.soe.ucsc.edu/goldenpath/hg38/chromosomes/chr21.fa.gz
gunzip chr21.fa.gz
```

El cargador de FASTA (`load_fasta`) filtra automáticamente las N's (bases desconocidas) y normaliza a mayúsculas, así que los archivos de UCSC funcionan sin preprocesamiento.

---

## 7. Uso de la demo visual

1. Compilar y ejecutar el target **`server`**.
2. Abrir el navegador en **http://localhost:8080** (no abrir `index.html` directamente con doble clic; debe accederse por el servidor).
3. Escribir una cadena de ADN corta (ej. `GATAGACA`) y pulsar **Construir FM-Index** para ver las estructuras finales.
4. Escribir un patrón (ej. `GA`) y pulsar **Buscar** para animar el backward search paso a paso.

La demo está pensada para cadenas cortas; los genomas grandes se usan en el ejecutable `benchmark`.

---

## 8. Experimentos incluidos en `benchmark`

1. **Construcción del SA:** DC3 vs Prefix Doubling a tamaños crecientes.
2. **Throughput de búsqueda:** `count` sobre 100,000 reads de largo 150.
3. **Escala real:** *E. coli* K-12, cromosoma 21 y cromosoma 1 humanos.

Para activar los cromosomas humanos, descomentar las líneas correspondientes en `main()` de `benchmark_main.cpp` una vez descargados los archivos.

---

## 9. Referencias principales

- Ferragina, P. & Manzini, G. (2000). Opportunistic data structures with applications. *FOCS*.
- Burrows, M. & Wheeler, D. J. (1994). A block-sorting lossless data compression algorithm. *Technical Report 124, DEC SRC*.
- Grossi, R., Gupta, A. & Vitter, J. S. (2003). High-order entropy-compressed text indexes. *SODA*.
- Kärkkäinen, J. & Sanders, P. (2003). Simple linear work suffix array construction. *ICALP*.
- Manber, U. & Myers, E. W. (1993). Suffix arrays: A new method for on-line string searches. *SIAM J. Comput.*
- Datos genómicos: [NCBI](https://www.ncbi.nlm.nih.gov/nuccore/NC_000913.3) (E. coli), [UCSC Genome Browser](https://hgdownload.soe.ucsc.edu/goldenpath/hg38/chromosomes/) (cromosomas humanos).
