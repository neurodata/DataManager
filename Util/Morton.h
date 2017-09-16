#ifndef MORTON_H
#define MORTON_H

#include <array>
#include <cstdint>

#include <glog/logging.h>

class Morton64 {
   public:
    template <class T>
    static uint64_t XYZMorton(const std::array<T, 3>& input) {
        uint64_t morton = 0;

        uint64_t x = static_cast<uint64_t>(input[0]);
        uint64_t y = static_cast<uint64_t>(input[1]);
        uint64_t z = static_cast<uint64_t>(input[2]);

        uint64_t mask = 0x001;

        for (int i = 0; i < 21; i++) {
            morton += (x & mask) << (2 * i);
            morton += (y & mask) << (2 * i + 1);
            morton += (z & mask) << (2 * i + 2);

            mask <<= 1;
        }
        return morton;
    }

    template <class T>
    static void MortonXYZ(const uint64_t _morton, std::array<T, 3>& output) {
        uint64_t morton = _morton;

        uint64_t xmask = 0x001;
        uint64_t ymask = 0x002;
        uint64_t zmask = 0x004;

        std::array<uint64_t, 3> ret({{0, 0, 0}});

        for (int i = 0; i < 21; i++) {
            ret[0] += (xmask & morton) << i;
            ret[1] += ((ymask & morton) << i) >> 1;
            ret[2] += ((zmask & morton) << i) >> 2;
            morton >>= 3;
        }

        for (int i = 0; i < 3; i++) {
            output[i] = static_cast<T>(ret[i]);
        }
    }
};

#endif  // MORTON_H