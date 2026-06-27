#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cctype>

using namespace std;

// Carga un archivo FASTA y retorna la secuencia concatenada (solo ACGT)
// Ignora lineas de encabezado (empiezan con >) y caracteres no-ACGT
string load_fasta(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "No se pudo abrir: " << filepath << endl;
        return "";
    }

    string result, line;
    while (getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '>')
            continue;
        for (char c : line) {
            c = toupper(c);
            if (c == 'A' || c == 'C' || c == 'G' || c == 'T')
                result += c;
        }
    }
    return result;
}

// Cronometro simple basado en chrono
struct Timer {
    chrono::high_resolution_clock::time_point t_start;

    void start() { t_start = chrono::high_resolution_clock::now(); }

    double elapsed_seconds() const {
        auto t_end = chrono::high_resolution_clock::now();
        return chrono::duration<double>(t_end - t_start).count();
    }
};
