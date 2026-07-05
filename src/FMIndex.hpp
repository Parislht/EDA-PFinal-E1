#pragma once
#include "WaveletTree.hpp"
#include "dc3.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <set>
#include <iostream>
#include <chrono>

using namespace std;

// FM-Index: indice comprimido para busqueda de patrones en O(m * log sigma)
// Combina Suffix Array + BWT + Tabla C + Wavelet Tree
class FMIndex {

    WaveletTree wt;
    vector<int> sa;
    vector<int> C;
    string alphabet;
    int n = 0;
    int char_idx[256];   // texto original reconstruido



    // Suffix Array con prefix doubling, O(n log^2 n)
    // Ordena sufijos comparando pares (rank[i], rank[i+k]) con k = 1, 2, 4,
    static vector<int> build_sa(const string& text) {
        int n = (int)text.size();
        vector<int> sa(n), ra(n), nr(n);
        iota(sa.begin(), sa.end(), 0);
        for (int i = 0; i < n; i++) ra[i] = text[i];

        for (int k = 1; k < n; k *= 2) {
            auto cmp = [&](int a, int b) {
                if (ra[a] != ra[b]) return ra[a] < ra[b];
                int x = (a + k < n) ? ra[a + k] : -1;
                int y = (b + k < n) ? ra[b + k] : -1;
                return x < y;
            };
            sort(sa.begin(), sa.end(), cmp);

            nr[sa[0]] = 0;
            for (int i = 1; i < n; i++)
                nr[sa[i]] = nr[sa[i - 1]] + (cmp(sa[i - 1], sa[i]) ? 1 : 0);
            ra = nr;
            if (ra[sa[n - 1]] == n - 1) break;
        }
        return sa;
    }

public:

    enum class SAMethod { PrefixDoubling, DC3 };

    FMIndex() { memset(char_idx, -1, sizeof(char_idx)); }

    // Construye el indice completo a partir del texto original (sin $)
    void build(const string& original_text, bool verbose = false,
               SAMethod method = SAMethod::DC3) {
        string text = original_text + "$";
        n = (int)text.size();

        // Detectar alfabeto
        set<char> chars(text.begin(), text.end());
        alphabet.assign(chars.begin(), chars.end());
        int sigma = (int)alphabet.size();
        memset(char_idx, -1, sizeof(char_idx));
        for (int i = 0; i < sigma; i++)
            char_idx[(unsigned char)alphabet[i]] = i;

        if (verbose) {
            cout << "Texto: " << n << " caracteres (con $)" << endl;
            cout << "Alfabeto: " << alphabet << " (sigma = " << sigma << ")" << endl;
        }

        // Suffix Array (elegir metodo)
        auto t0 = chrono::high_resolution_clock::now();
        if (verbose) cout << "Construyendo Suffix Array ("
                          << (method == SAMethod::DC3 ? "DC3" : "PrefixDoubling")
                          << ") ->" << flush;
        if (method == SAMethod::DC3)
            sa = buildSA_DC3(text);
        else
            sa = build_sa(text);
        auto t1 = chrono::high_resolution_clock::now();
        if (verbose)
            cout << " " << chrono::duration<double>(t1 - t0).count() << " s" << endl;

        // BWT: BWT[i] = T[SA[i] - 1], con wrap-around para SA[i] = 0
        t0 = chrono::high_resolution_clock::now();
        if (verbose) cout << "Generando BWT ->" << flush;
        string bwt(n, ' ');
        for (int i = 0; i < n; i++)
            bwt[i] = (sa[i] == 0) ? text[n - 1] : text[sa[i] - 1];
        t1 = chrono::high_resolution_clock::now();
        if (verbose)
            cout << " " << chrono::duration<double>(t1 - t0).count() << " s" << endl;

        // Tabla C: C[c] = cuantos caracteres en T son menores que c
        vector<int> freq(sigma, 0);
        for (char c : text) freq[char_idx[(unsigned char)c]]++;
        C.resize(sigma, 0);
        for (int i = 1; i < sigma; i++) C[i] = C[i - 1] + freq[i - 1];

        if (verbose) {
            cout << "Frecuencias: ";
            for (int i = 0; i < sigma; i++)
                cout << alphabet[i] << ":" << freq[i] << " ";
            cout << endl;
            cout << "Tabla C: ";
            for (int i = 0; i < sigma; i++)
                cout << alphabet[i] << "->" << C[i] << "  ";
            cout << endl;
        }

        // Wavelet Tree sobre la BWT
        t0 = chrono::high_resolution_clock::now();
        if (verbose) cout << "Construyendo Wavelet Tree ->" << flush;
        wt.build(bwt, alphabet);
        t1 = chrono::high_resolution_clock::now();
        if (verbose) {
            cout << " " << chrono::duration<double>(t1 - t0).count() << " s" << endl;
            cout << "Wavelet Tree: " << wt.num_nodes() << " nodos internos" << endl;
        }
    }

    // Cuenta cuantas veces aparece el patron en el texto
    int count(const string& pattern) const {
        if (pattern.empty()) return 0;
        int lo = 0, hi = n;

        for (int i = (int)pattern.size() - 1; i >= 0; i--) {
            char c = pattern[i];
            int ci = char_idx[(unsigned char)c];
            if (ci < 0) return 0;
            lo = C[ci] + wt.rank(lo, c);
            hi = C[ci] + wt.rank(hi, c);
            if (lo >= hi) return 0;
        }
        return hi - lo;
    }

    // Localiza las posiciones exactas donde aparece el patron
    vector<int> locate(const string& pattern) const {
        if (pattern.empty()) return {};
        int lo = 0, hi = n;

        for (int i = (int)pattern.size() - 1; i >= 0; i--) {
            char c = pattern[i];
            int ci = char_idx[(unsigned char)c];
            if (ci < 0) return {};
            lo = C[ci] + wt.rank(lo, c);
            hi = C[ci] + wt.rank(hi, c);
            if (lo >= hi) return {};
        }

        vector<int> pos;
        for (int i = lo; i < hi; i++) pos.push_back(sa[i]);
        sort(pos.begin(), pos.end());
        return pos;
    }

    // Busqueda mostrando cada paso del backward search
    void search_verbose(const string& pattern, int max_steps = 5) const {
        cout << "\nQuery: \"" << pattern << "\"" << endl;
        cout << "Patron leido de derecha a izquierda" << endl;

        int lo = 0, hi = n;
        string matched;

        for (int i = (int)pattern.size() - 1; i >= 0; i--) {
            char c = pattern[i];
            matched = string(1, c) + matched;
            int ci = char_idx[(unsigned char)c];
            int step = (int)pattern.size() - i;

            if (ci < 0) {
                cout << "  Caracter '" << c << "' no esta en el alfabeto" << endl;
                cout << "  Resultado: 0 ocurrencias" << endl;
                return;
            }

            int rk_lo = wt.rank(lo, c);
            int rk_hi = wt.rank(hi, c);
            int new_lo = C[ci] + rk_lo;
            int new_hi = C[ci] + rk_hi;

            if (step <= max_steps) {
                cout << endl<< "  Paso " << step << " - '" << c << "' (pos " << i << "):" << endl;
                cout << "    lo = C[" << c << "] + rank(" << lo << ", " << c << ") = "
                     << C[ci] << " + " << rk_lo << " = " << new_lo << endl;
                cout << "    hi = C[" << c << "] + rank(" << hi << ", " << c << ") = "
                     << C[ci] << " + " << rk_hi << " = " << new_hi << endl;
                cout << "    Rango [" << new_lo << ", " << new_hi << "] -> "
                     << (new_hi - new_lo) << " sufijos empiezan con \""
                     << matched << "\"" << endl;
            } else if (step == max_steps + 1) {
                cout << "  -> (pasos restantes omitidos)" << endl;
            }

            lo = new_lo;
            hi = new_hi;

            if (lo >= hi) {
                cout << "  Rango vacio en paso " << step << ": \""
                     << pattern << "\" no aparece en T" << endl;
                cout << endl<<"  Resultado: 0 ocurrencias" << endl;
                return;
            }
        }

        int occ = hi - lo;
        cout << "  Resultado: " << occ << " ocurrencias" << endl;

        if (occ <= 20) {
            vector<int> pos;
            for (int i = lo; i < hi; i++) pos.push_back(sa[i]);
            sort(pos.begin(), pos.end());
            cout << "  Posiciones: ";
            for (int i = 0; i < (int)pos.size(); i++) {
                if (i > 0) cout << ", ";
                cout << pos[i];
            }
            cout << endl;
        }
    }

    // Muestra informacion resumida del indice
    void print_info() const {
        cout << "SA[0..9]: ";
        for (int i = 0; i < min(10, n); i++) cout << sa[i] << " ";
        cout << endl;

        cout << "BWT[0..19]: ";
        for (int i = 0; i < min(20, n); i++) cout << wt.access(i);
        cout << endl;
    }

    int text_size() const { return n; }

    //Accessors para serializacion
    const vector<int>& get_sa() const { return sa; }
    const vector<int>& get_C() const { return C; }
    const string& get_alphabet() const { return alphabet; }
    const WaveletTree& wt_ref() const { return wt; }

    // Reconstruye la BWT como string leyendo el Wavelet Tree posicion por posicion.
    string get_bwt() const {
        string bwt(n, ' ');
        for (int i = 0; i < n; i++) bwt[i] = wt.access(i);
        return bwt;
    }

    // Indice de un caracter en el alfabeto
    int char_index(char c) const { return char_idx[(unsigned char)c]; }

    // rank del wavelet tree
    int wt_rank(int i, char c) const { return wt.rank(i, c); }

};
