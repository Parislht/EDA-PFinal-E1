#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include "FMIndex.hpp"
#include "utils.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <string>

using namespace std;

// Genera una secuencia de ADN aleatoria de longitud n
static string random_dna(int n, unsigned seed) {
    mt19937 rng(seed);
    const char bases[] = "ACGT";
    string s(n, 'A');
    for (auto& c : s) c = bases[rng() % 4];
    return s;
}

// EXPERIMENTO 1: Construccion del SA — DC3 vs Prefix Doubling
static void exp_construccion() {
    cout << "\n=== EXPERIMENTO 1: Construccion del SA (DC3 vs Prefix Doubling) ===\n";
    cout << left << setw(12) << "n"
         << setw(20) << "PrefixDoubling(s)"
         << setw(12) << "DC3(s)"
         << setw(10) << "speedup" << "\n";
    cout << string(54, '-') << "\n";

    for (int n : {100000, 500000, 1000000, 2000000, 4000000}) {
        string text = random_dna(n, 42);
        Timer t;

        t.start();
        FMIndex fm_pd;
        fm_pd.build(text, false, FMIndex::SAMethod::PrefixDoubling);
        double t_pd = t.elapsed_seconds();

        t.start();
        FMIndex fm_dc3;
        fm_dc3.build(text, false, FMIndex::SAMethod::DC3);
        double t_dc3 = t.elapsed_seconds();

        cout << left << setw(12) << n
             << setw(20) << fixed << setprecision(3) << t_pd
             << setw(12) << t_dc3
             << setw(10) << setprecision(2) << (t_pd / t_dc3) << "\n";
    }
}


// EXPERIMENTO 2: Throughput de busqueda (count) sobre un texto dado
static void exp_busqueda(const string& text, const string& etiqueta) {
    cout << "\n=== EXPERIMENTO 2: Throughput de busqueda [" << etiqueta << "] ===\n";

    Timer t;
    t.start();
    FMIndex fm;
    fm.build(text, false, FMIndex::SAMethod::DC3);
    double t_build = t.elapsed_seconds();
    cout << "Construccion (DC3): " << fixed << setprecision(3) << t_build << " s\n";

    int num_reads = 100000;
    int read_len = 150;
    if ((int)text.size() <= read_len) read_len = max(10, (int)text.size() / 2);

    mt19937 rng(7);
    uniform_int_distribution<int> dist(0, (int)text.size() - read_len - 1);
    vector<string> reads(num_reads);
    for (int i = 0; i < num_reads; i++)
        reads[i] = text.substr(dist(rng), read_len);

    // Medir solo count
    t.start();
    long long total_occ = 0;
    for (auto& r : reads) total_occ += fm.count(r);
    double t_search = t.elapsed_seconds();

    cout << "Reads: " << num_reads << " de largo " << read_len << "\n";
    cout << "Tiempo total: " << setprecision(3) << t_search << " s\n";
    cout << "Promedio por query: " << setprecision(2) << (t_search / num_reads) * 1e6 << " us\n";
    cout << "Throughput: " << setprecision(0) << (num_reads / t_search) << " queries/s\n";
    cout << "Ocurrencias totales: " << total_occ << "\n";
}

// EXPERIMENTO 3: Escala real — carga un FASTA y mide construccion + busqueda
static void exp_escala(const string& path, const string& etiqueta, bool run_pd = true) {
    cout << "\n=== EXPERIMENTO 3: Escala real [" << etiqueta << "] ===\n";
    cout << "Cargando " << path << " ...\n";

    string text = load_fasta(path);
    if (text.empty()) {
        cout << "No se pudo cargar el archivo (o esta vacio). Se omite.\n";
        return;
    }
    cout << "Longitud: " << text.size() << " bases\n";

    Timer t;

    // --- Construccion con Prefix Doubling (opcional) ---
    double t_pd = -1;
    if (run_pd) {
        t.start();
        FMIndex fm_pd;
        fm_pd.build(text, false, FMIndex::SAMethod::PrefixDoubling);
        t_pd = t.elapsed_seconds();
        cout << "Construccion Prefix Doubling: " << fixed << setprecision(3) << t_pd << " s\n";
    } else {
        cout << "Construccion Prefix Doubling: (omitida por tamano)\n";
    }

    // --- Construccion con DC3 ---
    t.start();
    FMIndex fm;
    fm.build(text, false, FMIndex::SAMethod::DC3);
    double t_dc3 = t.elapsed_seconds();
    cout << "Construccion DC3:             " << fixed << setprecision(3) << t_dc3 << " s\n";

    if (run_pd && t_dc3 > 0)
        cout << "Speedup DC3 vs PrefixDoubling: " << setprecision(2) << (t_pd / t_dc3) << "x\n";

    // --- Throughput de busqueda (sobre el indice DC3, que es identico) ---
    int num_reads = 100000, read_len = 150;
    mt19937 rng(7);
    uniform_int_distribution<int> dist(0, (int)text.size() - read_len - 1);
    vector<string> reads(num_reads);
    for (int i = 0; i < num_reads; i++)
        reads[i] = text.substr(dist(rng), read_len);

    t.start();
    long long total_occ = 0;
    for (auto& r : reads) total_occ += fm.count(r);
    double t_search = t.elapsed_seconds();

    cout << "Busqueda: " << num_reads << " reads de largo " << read_len << "\n";
    cout << "  Tiempo total: " << setprecision(3) << t_search << " s\n";
    cout << "  Promedio por query: " << setprecision(2) << (t_search / num_reads) * 1e6 << " us\n";
    cout << "  Throughput: " << setprecision(0) << (num_reads / t_search) << " queries/s\n";
    cout << "  Ocurrencias totales: " << total_occ << "\n";
}

int main() {
    cout << "############################################\n";
    cout << "#   BENCHMARK FM-Index + Wavelet Tree     #\n";
    cout << "#   (compilar en RELEASE para medir bien) #\n";
    cout << "############################################\n";

    // Experimento 1: comparacion de construccion
    exp_construccion();

    // Experimento 2: busqueda sobre secuencia sintetica de 4M
    exp_busqueda(random_dna(4000000, 99), "sintetico 4M");

    // Experimento 3: E. coli
    exp_escala("../data/GCF_000005845.2_ASM584v2_genomic.fna", "E. coli K-12", true);

    // Experimento 3b: cromosoma 21
    exp_escala("../data/chr21.fa", "Cromosoma 21 humano", true);

    // Experimento 3c: cromosoma 1
    exp_escala("../data/chr1.fa", "Cromosoma 1 humano", false);

    return 0;
}