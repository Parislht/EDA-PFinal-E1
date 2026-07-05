#pragma once
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// DC3 / Skew algorithm (Karkkainen-Sanders, 2003)
// Suffix Array en tiempo O(n).
// recurrencia T(n) = T(2n/3) + O(n) => O(n) total.
namespace dc3detail {

    // Compara pares (a1,a2) <= (b1,b2) lexicograficamente
    inline bool leq2(int a1, int a2, int b1, int b2) {
        return a1 < b1 || (a1 == b1 && a2 <= b2);
    }
    // Compara tripletas
    inline bool leq3(int a1, int a2, int a3, int b1, int b2, int b3) {
        return a1 < b1 || (a1 == b1 && leq2(a2, a3, b2, b3));
    }

    // Radix sort estable
    static void radixPass(const int* a, int* b, const int* r, int n, int K) {
        vector<int> c(K + 1, 0);
        for (int i = 0; i < n; i++) c[r[a[i]]]++;
        for (int i = 0, sum = 0; i <= K; i++) { int t = c[i]; c[i] = sum; sum += t; }
        for (int i = 0; i < n; i++) b[c[r[a[i]]]++] = a[i];
    }

    // Construye el SA de s[0..n-1] (con valores en 1..K) y lo deja en SA[0..n-1]
    static void suffixArray(const int* s, int* SA, int n, int K) {
        int n0 = (n + 2) / 3, n1 = (n + 1) / 3, n2 = n / 3;
        int n02 = n0 + n2;

        vector<int> s12(n02 + 3, 0);
        vector<int> SA12(n02 + 3, 0);
        vector<int> s0(n0, 0);
        vector<int> SA0(n0, 0);

        // Posiciones i % 3 != 0
        for (int i = 0, j = 0; i < n + (n0 - n1); i++)
            if (i % 3 != 0) s12[j++] = i;

        // Ordenar las tripletas por sus 3 caracteres (3 pasadas de radix)
        radixPass(s12.data(), SA12.data(), s + 2, n02, K);
        radixPass(SA12.data(), s12.data(), s + 1, n02, K);
        radixPass(s12.data(), SA12.data(), s,     n02, K);

        // Asignar nombres a cada tripleta distinta
        int name = 0, c0 = -1, c1 = -1, c2 = -1;
        for (int i = 0; i < n02; i++) {
            if (s[SA12[i]] != c0 || s[SA12[i]+1] != c1 || s[SA12[i]+2] != c2) {
                name++;
                c0 = s[SA12[i]]; c1 = s[SA12[i]+1]; c2 = s[SA12[i]+2];
            }
            if (SA12[i] % 3 == 1) s12[SA12[i]/3]      = name;   // grupo mod 1
            else                  s12[SA12[i]/3 + n0] = name;   // grupo mod 2
        }

        // Si hay nombres repetidos, recursar; si todos son unicos, SA directo
        if (name < n02) {
            suffixArray(s12.data(), SA12.data(), n02, name);
            for (int i = 0; i < n02; i++) s12[SA12[i]] = i + 1;
        } else {
            for (int i = 0; i < n02; i++) SA12[s12[i] - 1] = i;
        }

        // Ordenar los sufijos mod 0 por
        for (int i = 0, j = 0; i < n02; i++)
            if (SA12[i] < n0) s0[j++] = 3 * SA12[i];
        radixPass(s0.data(), SA0.data(), s, n0, K);

        // Merge de SA0 (mod 0) con SA12 (mod 1 y mod 2)
        for (int p = 0, t = n0 - n1, k = 0; k < n; k++) {
            auto GetI = [&]() {
                return SA12[t] < n0 ? SA12[t]*3 + 1 : (SA12[t]-n0)*3 + 2;
            };
            int i = GetI();     // sufijo mod 1/2 actual
            int j = SA0[p];     // sufijo mod 0 actual

            bool menor;
            if (SA12[t] < n0)
                menor = leq2(s[i], s12[SA12[t]+n0], s[j], s12[j/3]);
            else
                menor = leq3(s[i], s[i+1], s12[SA12[t]-n0+1],
                             s[j], s[j+1], s12[j/3+n0]);

            if (menor) {
                SA[k] = i;
                if (++t == n02)
                    for (k++; p < n0; p++, k++) SA[k] = SA0[p];
            } else {
                SA[k] = j;
                if (++p == n0)
                    for (k++; t < n02; t++, k++) SA[k] = GetI();
            }
        }
    }

} // namespace dc3detail

// Interfaz publica
inline vector<int> buildSA_DC3(const string& text) {
    int n = (int)text.size();
    if (n == 0) return {};
    if (n == 1) return {0};

    int K = 0;
    int mp[256];
    for (int i = 0; i < 256; i++) mp[i] = 0;
    for (unsigned char c : text) mp[c] = 1;
    for (int i = 0; i < 256; i++) if (mp[i]) mp[i] = ++K;

    vector<int> s(n + 3, 0);
    for (int i = 0; i < n; i++) s[i] = mp[(unsigned char)text[i]];

    vector<int> SA(n + 3, 0);
    dc3detail::suffixArray(s.data(), SA.data(), n, K);
    SA.resize(n);
    return SA;
}