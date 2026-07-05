#include "FMIndex.hpp"
#include "utils.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <iomanip>

using namespace std;

// Test 1: Verificacion con texto pequeno contra el documento tecnico
void test_small() {
    cout << "TEST 1: Verificacion con GATAGACA" << endl;
    cout << string(40, '-') << endl;

    FMIndex fm;
    fm.build("GATAGACA", true);
    fm.print_info();

    fm.search_verbose("GA");   // Esperado: 2 ocurrencias en pos 0 y 4
    fm.search_verbose("TC");   // Esperado: 0 ocurrencias
    fm.search_verbose("ACA");  // Esperado: 1 ocurrencia en pos 5
    fm.search_verbose("A");    // Esperado: 4 ocurrencias
}

// Test 2: Genoma real o secuencia grande con benchmark
void test_genome() {
    cout << "\n\nTEST 2: Genoma de E. coli K-12" << endl;
    cout << string(40, '-') << endl;

    // Intentar cargar el archivo FASTA del genoma de E. coli
    string path = "../data/GCF_000005845.2_ASM584v2_genomic.fna";
    string text = load_fasta(path);

    if (text.empty()) {
        cout << "No se encontro el archivo FASTA en: " << path << endl;
        cout << "Generando secuencia aleatoria de 500,000 caracteres como alternativa..." << endl;
        mt19937 rng(42);
        const char bases[] = "ACGT";
        text.resize(500000);
        for (char& c : text) c = bases[rng() % 4];
    }

    cout << "Longitud del texto: " << text.size() << " caracteres" << endl;

    // Construccion del FM-Index
    Timer timer;
    timer.start();
    FMIndex fm;
    fm.build(text, true);
    double build_time = timer.elapsed_seconds();
    cout << "Tiempo total de construccion: " << fixed << setprecision(3)
         << build_time << " s" << endl;
    fm.print_info();

    // Query detallada con un fragmento extraido del propio texto
    string sample = text.substr(1000, 10);
    fm.search_verbose(sample);

    // Busqueda de patrones especificos
    cout << "\nBusqueda de patrones especificos:" << endl;
    vector<string> patterns = {"GATTACA", "ACGTACGT", "ATGATGATG"};

    // Agregar patrones del propio texto (garantizado que se encuentran)
    int test_positions[] = {0, 50000, 200000};
    for (int p : test_positions) {
        if (p + 20 <= (int)text.size())
            patterns.push_back(text.substr(p, 20));
    }

    for (auto& p : patterns) {
        timer.start();
        auto positions = fm.locate(p);
        double t = timer.elapsed_seconds();

        string display = p.substr(0, 24);
        if ((int)p.size() > 24) display += "...";
        cout << "  \"" << display << "\" -> " << positions.size()
             << " ocurrencias (" << scientific << setprecision(2)
             << t << " s)";
        if (!positions.empty() && positions.size() <= 5) {
            cout << " pos: ";
            for (int i = 0; i < (int)positions.size(); i++) {
                if (i > 0) cout << ", ";
                cout << positions[i];
            }
        }
        cout << endl;
    }

    // Benchmark con reads simulados
    cout << "\nBenchmark: reads simulados" << endl;
    int num_reads = 100000;
    int read_len = 150;

    if ((int)text.size() <= read_len) {
        read_len = max(10, (int)text.size() / 2);
    }

    mt19937 rng(42);
    uniform_int_distribution<int> dist(0, (int)text.size() - read_len - 1);

    vector<string> reads(num_reads);
    for (int i = 0; i < num_reads; i++)
        reads[i] = text.substr(dist(rng), read_len);

    // Benchmark count (solo contar ocurrencias, sin extraer posiciones)
    timer.start();
    long long total_occ = 0;
    for (auto& r : reads) total_occ += fm.count(r);
    double bench_time = timer.elapsed_seconds();

    cout << "  Reads: " << num_reads << " de largo " << read_len << endl;
    cout << "  Tiempo total: " << fixed << setprecision(3)
         << bench_time << " s" << endl;
    cout << "  Tiempo promedio por query: " << fixed << setprecision(2)
         << (bench_time / num_reads) * 1e6 << " us" << endl;
    cout << "  Ocurrencias totales: " << total_occ << endl;
    cout << "  Throughput: " << fixed << setprecision(0)
         << num_reads / bench_time << " queries/segundo" << endl;
}

int main() {
    test_small();
    test_genome();
    return 0;
}
