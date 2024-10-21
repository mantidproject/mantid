// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/MortonIndex/BitInterleaving.h"
#include <cxxtest/TestSuite.h>
#include <utility>

using namespace morton_index;

/**
 * Interleaves four 32 bit integers into a single 128 bit integer (represented
 * by 64 bit LSB and MSB).
 */
inline void Interleave_4_32_128(uint64_t &msb, uint64_t &lsb, const uint32_t a, const uint32_t b, const uint32_t c,
                                const uint32_t d) {
  const size_t halfBitLen(sizeof(uint32_t) * CHAR_BIT / 2);

  lsb = interleave<4, uint16_t, uint64_t>({(uint16_t)a, (uint16_t)b, (uint16_t)c, (uint16_t)d});

  msb = interleave<4, uint16_t, uint64_t>({(uint16_t)(a >> halfBitLen), (uint16_t)(b >> halfBitLen),
                                           (uint16_t)(c >> halfBitLen), (uint16_t)(d >> halfBitLen)});
}

/**
 * Deinterleaves a 128 bit integer (represented by 64 bit MSB and LSB) into four
 * 32 bit integers.
 */
inline void Deinterleave_4_32_128(const uint64_t msb, const uint64_t lsb, uint32_t &a, uint32_t &b, uint32_t &c,
                                  uint32_t &d) {
  const size_t halfBitLen(sizeof(uint32_t) * CHAR_BIT / 2);

  const auto coordLsb = deinterleave<4, uint16_t, uint64_t>(lsb);
  const auto coordMsb = deinterleave<4, uint16_t, uint64_t>(msb);

  a = coordLsb[0] | (coordMsb[0] << halfBitLen);
  b = coordLsb[1] | (coordMsb[1] << halfBitLen);
  c = coordLsb[2] | (coordMsb[2] << halfBitLen);
  d = coordLsb[3] | (coordMsb[3] << halfBitLen);
}

/* Reverse wrapper taken from: https://stackoverflow.com/a/28139075 */

template <typename T> struct reversion_wrapper {
  T &iterable;
};

template <typename T> auto begin(reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

template <typename T> auto end(reversion_wrapper<T> w) { return std::rend(w.iterable); }

template <typename T> reversion_wrapper<T> reverse(T &&iterable) { return {iterable}; }

/**
 * Converts from a bit pattern represented as a string to an integer of a given
 * fixed width.
 *
 * Assumes sizeof(IntT) == bitStr.size() / 8 and that the least significant bit
 * is at the end of the string.
 */
template <typename IntT> IntT bit_string_to_int(const std::string &bitStr) {
  IntT res = 0;
  size_t i = 0;
  for (const auto &c : reverse(bitStr)) {
    res |= (IntT)(c == '1' ? 1 : 0) << i++;
  }
  return res;
}

class BitInterleavingTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BitInterleavingTest *createSuite() { return new BitInterleavingTest(); }
  static void destroySuite(BitInterleavingTest *suite) { delete suite; }

  const uint32_t integerA = bit_string_to_int<uint32_t>("10101010101010101010101010101010");
  const uint32_t integerB = bit_string_to_int<uint32_t>("00000000000000001111111111111111");
  const uint32_t integerC = bit_string_to_int<uint32_t>("11111111111111110000000000000000");
  const uint32_t integerD = bit_string_to_int<uint32_t>("00000000111111111111111100000000");

  const uint64_t interleavedMsb =
      bit_string_to_int<uint64_t>("0101010001010100010101000101010011011100110111001101110011011100");
  const uint64_t interleavedLsb =
      bit_string_to_int<uint64_t>("1011101010111010101110101011101000110010001100100011001000110010");

  void test_BitInterleaving128BitTest_Interleave_4_32_128() {
    uint64_t msb(0);
    uint64_t lsb(0);

    Interleave_4_32_128(msb, lsb, integerA, integerB, integerC, integerD);

    TS_ASSERT_EQUALS(interleavedMsb, msb);
    TS_ASSERT_EQUALS(interleavedLsb, lsb);
  }

  void test_BitInterleaving128BitTest_Deinterleave_4_32_128() {
    uint32_t a(0), b(0), c(0), d(0);
    Deinterleave_4_32_128(interleavedMsb, interleavedLsb, a, b, c, d);

    TS_ASSERT_EQUALS(integerA, a);
    TS_ASSERT_EQUALS(integerB, b);
    TS_ASSERT_EQUALS(integerC, c);
    TS_ASSERT_EQUALS(integerD, d);
  }

  void test_BitInterleaving128BitTest_Interleave_4_32_128_std() {
    uint128_t res = interleave<4, uint32_t, uint128_t>({integerA, integerB, integerC, integerD});

    uint128_t interleaved(interleavedLsb);
    interleaved |= ((uint128_t)interleavedMsb) << 64;

    TS_ASSERT_EQUALS(interleaved, res);
  }

  void test_BitInterleaving128BitTest_Deinterleave_4_32_128_std() {
    uint128_t z(interleavedLsb);
    z |= ((uint128_t)interleavedMsb) << 64;

    const auto result = deinterleave<4, uint32_t, uint128_t>(z);

    TS_ASSERT_EQUALS(integerA, result[0]);
    TS_ASSERT_EQUALS(integerB, result[1]);
    TS_ASSERT_EQUALS(integerC, result[2]);
    TS_ASSERT_EQUALS(integerD, result[3]);
  }
};
