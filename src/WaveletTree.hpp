#pragma once
#include "BitVector.hpp"
#include <string>
#include <vector>
#include <cstring>

using namespace std;

// Wavelet Tree: soporta rank(i, c) en O(log sigma) y access(i) en O(log sigma)
// Construido sobre un string con alfabeto arbitrario
class WaveletTree {
    struct Node {
        BitVector bv;
        int left_child = -1, right_child = -1;
        int alpha_lo = 0, alpha_hi = 0;
    };

    vector<Node> nodes;
    string alphabet;
    int code_of[256];
    int sigma = 0;

    // Construccion recursiva: divide el alfabeto en dos mitades por nivel
    int build_rec(const vector<int>& seq, int alo, int ahi) {
        if (ahi - alo <= 1) return -1;

        int idx = (int)nodes.size();
        nodes.push_back({});
        nodes[idx].alpha_lo = alo;
        nodes[idx].alpha_hi = ahi;

        int mid = (alo + ahi) / 2;
        vector<uint8_t> bits(seq.size());
        vector<int> left_seq, right_seq;

        for (int i = 0; i < (int)seq.size(); i++) {
            if (seq[i] < mid) {
                bits[i] = 0;
                left_seq.push_back(seq[i]);
            } else {
                bits[i] = 1;
                right_seq.push_back(seq[i]);
            }
        }

        nodes[idx].bv.build(bits);
        int lc = build_rec(left_seq, alo, mid);
        int rc = build_rec(right_seq, mid, ahi);
        nodes[idx].left_child = lc;
        nodes[idx].right_child = rc;
        return idx;
    }

public:
    WaveletTree() { memset(code_of, -1, sizeof(code_of)); }

    // Construye el wavelet tree sobre el texto dado con el alfabeto especificado
    void build(const string& text, const string& alpha) {
        alphabet = alpha;
        sigma = (int)alphabet.size();
        memset(code_of, -1, sizeof(code_of));
        for (int i = 0; i < sigma; i++)
            code_of[(unsigned char)alphabet[i]] = i;

        nodes.clear();
        nodes.reserve(2 * sigma);

        vector<int> seq(text.size());
        for (int i = 0; i < (int)text.size(); i++)
            seq[i] = code_of[(unsigned char)text[i]];

        build_rec(seq, 0, sigma);
    }

    // rank(i, c): cuantas veces aparece c en las primeras i posiciones [0..i-1]
    int rank(int i, char c) const {
        int cd = code_of[(unsigned char)c];
        if (cd < 0 || nodes.empty()) return 0;
        int pos = i, nid = 0;

        while (nid != -1) {
            const Node& nd = nodes[nid];
            int mid = (nd.alpha_lo + nd.alpha_hi) / 2;
            if (cd < mid) {
                pos = nd.bv.rank0(pos);
                nid = nd.left_child;
            } else {
                pos = nd.bv.rank1(pos);
                nid = nd.right_child;
            }
        }
        return pos;
    }

    // access(i): que caracter hay en la posicion i del string original
    char access(int i) const {
        int nid = 0, pos = i;
        int alo = 0, ahi = sigma;

        while (ahi - alo > 1) {
            const Node& nd = nodes[nid];
            int mid = (alo + ahi) / 2;
            if (!nd.bv.access(pos)) {
                pos = nd.bv.rank0(pos);
                ahi = mid;
                nid = nd.left_child;
            } else {
                pos = nd.bv.rank1(pos);
                alo = mid;
                nid = nd.right_child;
            }
        }
        return alphabet[alo];
    }

    int get_sigma() const { return sigma; }
    const string& get_alphabet() const { return alphabet; }
    int num_nodes() const { return (int)nodes.size(); }
};
