#pragma once
#include "FMIndex.hpp"
#include "WaveletTree.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <algorithm>
using namespace std;

// Utilidades para construir JSON
namespace jsonutil {

    // Escapa caracteres especiales de un string para JSON.
    // Escapa caracteres especiales de un string para JSON, incluyendo
    // cualquier caracter de control (< 0x20) que rompería el parseo.
    inline string escape(const string& s) {
        string out;
        out.reserve(s.size() + 2);
        for (unsigned char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (c < 0x20) {
                        // Cualquier otro caracter de control -> \u00XX
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    } else {
                        out += (char)c;
                    }
                    break;
            }
        }
        return out;
    }

    // Convierte un vector<int> a un arreglo JSON: [1,2,3]
    inline string arr(const vector<int>& v) {
        ostringstream os;
        os << "[";
        for (size_t i = 0; i < v.size(); i++) {
            if (i) os << ",";
            os << v[i];
        }
        os << "]";
        return os.str();
    }

    // Convierte un vector<uint8_t> (bits) a un arreglo JSON: [0,1,1,0]
    inline string bitarr(const vector<uint8_t>& v) {
        ostringstream os;
        os << "[";
        for (size_t i = 0; i < v.size(); i++) {
            if (i) os << ",";
            os << (int)v[i];
        }
        os << "]";
        return os.str();
    }

} // namespace jsonutil


// Serializa el Wavelet Tree completo
inline string exportWaveletTree(const WaveletTree& wt) {
    ostringstream os;

    // Alfabeto como string
    const string& alpha = wt.get_alphabet();

    os << "{";
    os << "\"alphabet\":\"" << jsonutil::escape(alpha) << "\",";
    os << "\"sigma\":" << wt.get_sigma() << ",";
    os << "\"root\":" << wt.root() << ",";
    os << "\"nodes\":[";

    int total = wt.num_nodes();
    for (int i = 0; i < total; i++) {
        if (i) os << ",";
        auto [alo, ahi] = wt.node_alpha_range(i);
        os << "{";
        os << "\"id\":" << i << ",";
        os << "\"alpha_lo\":" << alo << ",";
        os << "\"alpha_hi\":" << ahi << ",";
        os << "\"left\":"  << wt.node_left(i)  << ",";
        os << "\"right\":" << wt.node_right(i) << ",";
        os << "\"bits\":"  << jsonutil::bitarr(wt.node_bits(i));
        os << "}";
    }

    os << "]}";
    return os.str();
}

// exportState: serializa todo el estado final del FM-Index a JSON.
//
// Estructura del JSON devuelto:
// {
//   "n": 9,
//   "alphabet": "$ACGT",
//   "sa":  [8,7,5,3,1,6,4,0,2],
//   "bwt": "ACGTGAA$A",
//   "C":   [ {"char":"$","value":0}, {"char":"A","value":1}, ... ],
//   "suffixes": [ {"row":0,"sa":8,"suffix":"$","F":"$","L":"A"}, ... ],
//   "wavelet": { ... }
// }


inline string exportState(const FMIndex& fm) {
    ostringstream os;

    // Recuperamos las piezas
    int n            = fm.text_size();
    const auto& sa   = fm.get_sa();
    const string& al = fm.get_alphabet();
    const auto& C    = fm.get_C();
    string bwt       = fm.get_bwt();

    os << "{";
    os << "\"n\":" << n << ",";
    os << "\"alphabet\":\"" << jsonutil::escape(al) << "\",";

    // Suffix Array
    os << "\"sa\":" << jsonutil::arr(sa) << ",";

    // BWT como string
    os << "\"bwt\":\"" << jsonutil::escape(bwt) << "\",";

    // Tabla C como lista de {char, value}
    os << "\"C\":[";
    for (size_t i = 0; i < al.size(); i++) {
        if (i) os << ",";
        os << "{\"char\":\"" << jsonutil::escape(string(1, al[i]))
           << "\",\"value\":" << C[i] << "}";
    }
    os << "],";

    // Tabla de sufijos ordenados
    os << "\"suffixes\":[";
    {
        // Para cada fila i, determinar que caracter del alfabeto le corresponde en F
        int ci = 0;  // indice del caracter actual en el alfabeto
        for (int i = 0; i < n; i++) {
            // Avanzar ci mientras i alcance el siguiente bloque de C
            while (ci + 1 < (int)al.size() && i >= C[ci + 1]) ci++;
            char F = al[ci];

            if (i) os << ",";
            os << "{";
            os << "\"row\":" << i << ",";
            os << "\"sa\":" << sa[i] << ",";
            os << "\"F\":\"" << jsonutil::escape(string(1, F)) << "\",";
            os << "\"L\":\"" << jsonutil::escape(string(1, bwt[i])) << "\"";
            os << "}";
        }
    }
    os << "],";

    // Wavelet Tree completo
    os << "\"wavelet\":" << exportWaveletTree(fm.wt_ref());

    os << "}";
    return os.str();
}

// searchTracedJson: ejecuta el backward search grabando cada paso, y devuelve
// un JSON con la traza completa para animar en el frontend.
//
// Estructura del JSON:
// {
//   "pattern": "GA",
//   "n": 9,
//   "steps": [
//     { "step":1, "char":"A", "charPos":1,
//       "lo_before":0, "hi_before":9,
//       "rank_lo":0, "rank_hi":4, "C":1,
//       "lo_after":1, "hi_after":5,
//       "matched":"A", "occ":4, "empty":false },
//     ...
//   ],
//   "result": { "count":2, "positions":[0,4], "found":true }
// }

inline string searchTracedJson(const FMIndex& fm, const string& pattern) {
    ostringstream os;
    int n            = fm.text_size();
    const auto& sa   = fm.get_sa();
    const auto& C    = fm.get_C();
    const string& al = fm.get_alphabet();

    os << "{";
    os << "\"pattern\":\"" << jsonutil::escape(pattern) << "\",";
    os << "\"n\":" << n << ",";
    os << "\"steps\":[";

    int lo = 0, hi = n;
    string matched;
    bool empty = false;
    bool firstStep = true;

    // Backward search: leer el patron de derecha a izquierda
    for (int i = (int)pattern.size() - 1; i >= 0; i--) {
        char c = pattern[i];
        int ci = fm.char_index(c);
        matched = string(1, c) + matched;
        int stepNum = (int)pattern.size() - i;

        if (!firstStep) os << ",";
        firstStep = false;

        // Caracter fuera del alfabeto -> no existe
        if (ci < 0) {
            os << "{";
            os << "\"step\":" << stepNum << ",";
            os << "\"char\":\"" << jsonutil::escape(string(1, c)) << "\",";
            os << "\"charPos\":" << i << ",";
            os << "\"lo_before\":" << lo << ",\"hi_before\":" << hi << ",";
            os << "\"outOfAlphabet\":true,";
            os << "\"matched\":\"" << jsonutil::escape(matched) << "\",";
            os << "\"occ\":0,\"empty\":true";
            os << "}";
            empty = true;
            break;
        }

        int rank_lo = fm.wt_rank(lo, c);
        int rank_hi = fm.wt_rank(hi, c);
        int new_lo = C[ci] + rank_lo;
        int new_hi = C[ci] + rank_hi;

        os << "{";
        os << "\"step\":" << stepNum << ",";
        os << "\"char\":\"" << jsonutil::escape(string(1, c)) << "\",";
        os << "\"charPos\":" << i << ",";
        os << "\"lo_before\":" << lo << ",\"hi_before\":" << hi << ",";
        os << "\"rank_lo\":" << rank_lo << ",\"rank_hi\":" << rank_hi << ",";
        os << "\"C\":" << C[ci] << ",";
        os << "\"lo_after\":" << new_lo << ",\"hi_after\":" << new_hi << ",";
        os << "\"matched\":\"" << jsonutil::escape(matched) << "\",";
        os << "\"occ\":" << (new_hi - new_lo) << ",";
        os << "\"empty\":" << ((new_lo >= new_hi) ? "true" : "false");
        os << "}";

        lo = new_lo;
        hi = new_hi;

        if (lo >= hi) { empty = true; break; }
    }

    os << "],";

    // Resultado final
    os << "\"result\":{";
    if (empty || pattern.empty()) {
        os << "\"count\":0,\"positions\":[],\"found\":false";
    } else {
        vector<int> pos;
        for (int i = lo; i < hi; i++) pos.push_back(sa[i]);
        sort(pos.begin(), pos.end());
        os << "\"count\":" << pos.size() << ",";
        os << "\"positions\":" << jsonutil::arr(pos) << ",";
        os << "\"found\":true";
    }
    os << "}";

    os << "}";
    return os.str();
}