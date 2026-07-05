#pragma once
#include <vector>
#include <cstdint>

using namespace std;

// Bitvector con rank/access en O(1) usando bloques de 64 bits
class BitVector {
    vector<uint64_t> blocks;
    vector<int> prefix;
    int n = 0;

    static int popcount64(uint64_t x) {
        x -= (x >> 1) & 0x5555555555555555ULL;
        x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
        x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
        return (int)((x * 0x0101010101010101ULL) >> 56);
    }

public:
    BitVector() = default;

    // Construye el bitvector a partir de un arreglo de bits (0 o 1)
    void build(const vector<uint8_t>& bits) {
        n = (int)bits.size();
        int nb = (n + 63) / 64;
        blocks.assign(nb, 0);
        prefix.assign(nb + 1, 0);

        for (int i = 0; i < n; i++)
            if (bits[i]) blocks[i / 64] |= (1ULL << (i % 64));

        for (int i = 0; i < nb; i++)
            prefix[i + 1] = prefix[i] + popcount64(blocks[i]);
    }

    // Cuantos 1s hay en [0, i)
    int rank1(int i) const {
        if (i <= 0) return 0;
        if (i > n) i = n;
        int b = i / 64, r = i % 64;
        int cnt = prefix[b];
        if (r > 0) cnt += popcount64(blocks[b] & ((1ULL << r) - 1));
        return cnt;
    }

    // Cuantos 0s hay en [0, i)
    int rank0(int i) const {
        if (i <= 0) return 0;
        return i - rank1(i);
    }

    // Valor del bit en la posicion i
    bool access(int i) const {
        return (blocks[i / 64] >> (i % 64)) & 1;
    }

    // Devuelve todos los bits como vector de 0s y 1s (para serializar/visualizar)
    vector<uint8_t> get_bits() const {
        vector<uint8_t> bits(n);
        for (int i = 0; i < n; i++)
            bits[i] = (blocks[i / 64] >> (i % 64)) & 1;
        return bits;
    }

    int size() const { return n; }
};
