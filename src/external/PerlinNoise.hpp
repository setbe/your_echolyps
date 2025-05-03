//  THIS LIBRARY HAS BEEN EDITED BY `Your Echolyps` project.
//  No-std, using hi::math
//  Permits only <stdint.h>
//  Depends on custom hi::math implementation instead of math.h
//
//  See original library here: https://github.com/Reputeless/PerlinNoise
//
//----------------------------------------------------------------------------------------
//
//	siv::PerlinNoise
//	Perlin noise library for modern C++
//
//	Copyright (C) 2013-2021 Ryo Suzuki <reputeless@gmail.com>
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files(the
// "Software"), to deal 	in the Software without restriction, including
// without limitation the rights 	to use, copy, modify, merge, publish,
// distribute, sublicense, and / or sell 	copies of the Software, and to
// permit persons to whom the Software is 	furnished to do so, subject to
// the
// following conditions :
//
//	The above copyright notice and this permission notice shall be included
// in
// 	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
// THE 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN 	THE SOFTWARE.
//
//----------------------------------------------------------------------------------------

#pragma once
#include "linmath.hpp"
#include <stdint.h>

namespace siv {

template <typename Float> class BasicPerlinNoise {
  public:
    using value_type = Float;
    using seed_type = uint32_t;

    // Constructors
    BasicPerlinNoise() noexcept { initDefault(); }
    explicit BasicPerlinNoise(seed_type s) { reseed(s); }

    // Reseed
    void reseed(seed_type s) {
        // fill 0..255
        for (uint32_t i = 0; i < 256; ++i)
            m_perm[i] = (uint8_t)i;
        // xorshift RNG
        uint32_t x = s ? s : 2463534242u;
        for (int i = 0; i < 10; ++i)
            x ^= x << 13, x ^= x >> 17, x ^= x << 5;
        // shuffle
        for (int i = 255; i > 0; --i) {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            uint32_t j = x % (uint32_t)(i + 1);
            uint8_t tmp = m_perm[i];
            m_perm[i] = m_perm[j];
            m_perm[j] = tmp;
        }
    }

    // Serialize/deserialize
    const uint8_t *serialize() const noexcept { return m_perm; }
    void deserialize(const uint8_t state[256]) noexcept {
        for (int i = 0; i < 256; ++i)
            m_perm[i] = state[i];
    }

    // Noise in [-1,1]
    value_type noise1D(value_type x) const noexcept {
        return noise3D(x, Y_OFFSET, Z_OFFSET);
    }
    value_type noise2D(value_type x, value_type y) const noexcept {
        return noise3D(x, y, Z_OFFSET);
    }
    value_type noise3D(value_type x, value_type y,
                       value_type z) const noexcept {
        // floor
        Float fx = hi::math::floorf(x);
        Float fy = hi::math::floorf(y);
        Float fz = hi::math::floorf(z);
        int ix = ((int)fx) & 255;
        int iy = ((int)fy) & 255;
        int iz = ((int)fz) & 255;
        Float dx = x - fx;
        Float dy = y - fy;
        Float dz = z - fz;
        Float u = fade(dx);
        Float v = fade(dy);
        Float w = fade(dz);
        int A = (m_perm[ix] + iy) & 255;
        int B = (m_perm[(ix + 1) & 255] + iy) & 255;
        int AA = (m_perm[A] + iz) & 255;
        int AB = (m_perm[(A + 1) & 255] + iz) & 255;
        int BA = (m_perm[B] + iz) & 255;
        int BB = (m_perm[(B + 1) & 255] + iz) & 255;
        Float p0 = grad(m_perm[AA], dx, dy, dz);
        Float p1 = grad(m_perm[BA], dx - 1, dy, dz);
        Float p2 = grad(m_perm[AB], dx, dy - 1, dz);
        Float p3 = grad(m_perm[BB], dx - 1, dy - 1, dz);
        Float p4 = grad(m_perm[(AA + 1) & 255], dx, dy, dz - 1);
        Float p5 = grad(m_perm[(BA + 1) & 255], dx - 1, dy, dz - 1);
        Float p6 = grad(m_perm[(AB + 1) & 255], dx, dy - 1, dz - 1);
        Float p7 = grad(m_perm[(BB + 1) & 255], dx - 1, dy - 1, dz - 1);
        Float q0 = lerp(p0, p1, u);
        Float q1 = lerp(p2, p3, u);
        Float q2 = lerp(p4, p5, u);
        Float q3 = lerp(p6, p7, u);
        Float r0 = lerp(q0, q1, v);
        Float r1 = lerp(q2, q3, v);
        return lerp(r0, r1, w);
    }

    // remap to [0,1]
    value_type noise1D_01(value_type x) const noexcept {
        return remap01(noise1D(x));
    }
    value_type noise2D_01(value_type x, value_type y) const noexcept {
        return remap01(noise2D(x, y));
    }
    value_type noise3D_01(value_type x, value_type y,
                          value_type z) const noexcept {
        return remap01(noise3D(x, y, z));
    }

    // Octaves
    value_type octave1D(value_type x, int oct,
                        value_type pers = 0.5f) const noexcept {
        Float sum = 0, amp = 1;
        for (int i = 0; i < oct; ++i) {
            sum += noise1D(x) * amp;
            x *= 2;
            amp *= pers;
        }
        return sum;
    }
    value_type octave2D(value_type x, value_type y, int oct,
                        value_type pers = 0.5f) const noexcept {
        Float sum = 0, amp = 1;
        for (int i = 0; i < oct; ++i) {
            sum += noise2D(x, y) * amp;
            x *= 2;
            y *= 2;
            amp *= pers;
        }
        return sum;
    }
    value_type octave3D(value_type x, value_type y, value_type z, int oct,
                        value_type pers = 0.5f) const noexcept {
        Float sum = 0, amp = 1;
        for (int i = 0; i < oct; ++i) {
            sum += noise3D(x, y, z) * amp;
            x *= 2;
            y *= 2;
            z *= 2;
            amp *= pers;
        }
        return sum;
    }

    // clamps & remaps
    value_type octave1D_11(value_type x, int o,
                           value_type p = 0.5f) const noexcept {
        return clamp(octave1D(x, o, p));
    }
    value_type octave2D_11(value_type x, value_type y, int o,
                           value_type p = 0.5f) const noexcept {
        return clamp(octave2D(x, y, o, p));
    }
    value_type octave3D_11(value_type x, value_type y, value_type z, int o,
                           value_type p = 0.5f) const noexcept {
        return clamp(octave3D(x, y, z, o, p));
    }
    value_type octave1D_01(value_type x, int o,
                           value_type p = 0.5f) const noexcept {
        return remapClamp01(octave1D(x, o, p));
    }
    value_type octave2D_01(value_type x, value_type y, int o,
                           value_type p = 0.5f) const noexcept {
        return remapClamp01(octave2D(x, y, o, p));
    }
    value_type octave3D_01(value_type x, value_type y, value_type z, int o,
                           value_type p = 0.5f) const noexcept {
        return remapClamp01(octave3D(x, y, z, o, p));
    }

    value_type normalizedOctave1D(value_type x, int o,
                                  value_type p = 0.5f) const noexcept {
        return octave1D(x, o, p) / maxAmp(o, p);
    }
    value_type normalizedOctave2D(value_type x, value_type y, int o,
                                  value_type p = 0.5f) const noexcept {
        return octave2D(x, y, o, p) / maxAmp(o, p);
    }
    value_type normalizedOctave3D(value_type x, value_type y, value_type z,
                                  int o, value_type p = 0.5f) const noexcept {
        return octave3D(x, y, z, o, p) / maxAmp(o, p);
    }
    value_type normalizedOctave1D_01(value_type x, int o,
                                     value_type p = 0.5f) const noexcept {
        return remap01(normalizedOctave1D(x, o, p));
    }
    value_type normalizedOctave2D_01(value_type x, value_type y, int o,
                                     value_type p = 0.5f) const noexcept {
        return remap01(normalizedOctave2D(x, y, o, p));
    }
    value_type normalizedOctave3D_01(value_type x, value_type y, value_type z,
                                     int o,
                                     value_type p = 0.5f) const noexcept {
        return remap01(normalizedOctave3D(x, y, z, o, p));
    }

  private:
    uint8_t m_perm[256];
    static const Float Y_OFFSET, Z_OFFSET;

    void initDefault() noexcept {
        for (int i = 0; i < 256; ++i)
            m_perm[i] = default_perm[i];
    }
    static Float fade(Float t) noexcept {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    static Float lerp(Float a, Float b, Float t) noexcept {
        return a + (b - a) * t;
    }
    static Float grad(uint8_t h, Float x, Float y, Float z) noexcept {
        return ((h & 1) ? -x : x) + ((h & 2) ? -y : y);
    }
    static Float remap01(Float x) noexcept { return x * 0.5f + 0.5f; }
    static Float clamp(Float x) noexcept {
        return x < -1 ? -1 : (x > 1 ? 1 : x);
    }
    static Float remapClamp01(Float x) noexcept {
        return clamp(x) * 0.5f + 0.5f;
    }
    static Float maxAmp(int oct, Float p) noexcept {
        Float s = 0, a = 1;
        for (int i = 0; i < oct; ++i) {
            s += a;
            a *= p;
        }
        return s;
    }

    static const uint8_t default_perm[256];
};

// static members
template <typename F> const F BasicPerlinNoise<F>::Y_OFFSET = F(0.12345);
template <typename F> const F BasicPerlinNoise<F>::Z_OFFSET = F(0.34567);

// default_perm definition (copy values from original)
template <typename F>
const uint8_t BasicPerlinNoise<F>::default_perm[256] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,
    225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190,
    6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117,
    35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
    171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158,
    231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,
    245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
    76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
    164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
    202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,
    58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,
    154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
    19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
    228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
    145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184,
    84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
    222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156,
    180};

using PerlinNoise = BasicPerlinNoise<float>;

} // namespace siv
