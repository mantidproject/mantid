// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cinttypes>
#include <cstddef>

#include "Types.h"

namespace morton_index {

/**
 * Pad an integer with a given number of padding bits.
 *
 * @tparam N Number of padding bits to add
 * @tparam IntT Integer type
 * @tparam MortonT Padded integer type
 * @return Padded integer
 */
template <size_t N, typename IntT, typename MortonT> MortonT pad(IntT) {
  throw std::runtime_error("No pad() specialisation.");
}

/**
 * Compacts (removes padding from) an integer with a given number of padding
 * bits.
 *
 * @tparam N Number of padding bits to remove
 * @tparam IntT Integer type
 * @tparam MortonT Padded integer type
 * @return Original integer
 */
template <size_t N, typename IntT, typename MortonT> IntT compact(MortonT) {
  throw std::runtime_error("No compact() specialisation.");
}

/* Bit masks used for pad and compact operations are derived using
 * docs/bit_padding_generator.py. */
/* For more details see docs/bit_cppierleaving.md. */

template <> inline uint32_t pad<1, uint16_t, uint32_t>(uint16_t v) {
  uint32_t x(v);
  x &= 0xffff;
  x = (x | x << 8) & 0xff00ff;
  x = (x | x << 4) & 0xf0f0f0f;
  x = (x | x << 2) & 0x33333333;
  x = (x | x << 1) & 0x55555555;
  return x;
}

template <> inline uint16_t compact<1, uint16_t, uint32_t>(uint32_t x) {
  x &= 0x55555555;
  x = (x | x >> 1) & 0x33333333;
  x = (x | x >> 2) & 0xf0f0f0f;
  x = (x | x >> 4) & 0xff00ff;
  x = (x | x >> 8) & 0xffff;
  return (uint16_t)x;
}

template <> inline uint64_t pad<1, uint16_t, uint64_t>(uint16_t v) {
  uint64_t x(v);
  x &= 0xffff;
  x = (x | x << 8) & 0xff00ff;
  x = (x | x << 4) & 0xf0f0f0f;
  x = (x | x << 2) & 0x33333333;
  x = (x | x << 1) & 0x55555555;
  return x;
}

template <> inline uint16_t compact<1, uint16_t, uint64_t>(uint64_t x) {
  x &= 0x55555555;
  x = (x | x >> 1) & 0x33333333;
  x = (x | x >> 2) & 0xf0f0f0f;
  x = (x | x >> 4) & 0xff00ff;
  x = (x | x >> 8) & 0xffff;
  return (uint16_t)x;
}

template <> inline uint32_t pad<2, uint8_t, uint32_t>(uint8_t v) {
  uint32_t x(v);
  x &= 0xff;
  x = (x | x << 8) & 0xf00f;
  x = (x | x << 4) & 0xc30c3;
  x = (x | x << 2) & 0x249249;
  return x;
}

template <> inline uint8_t compact<2, uint8_t, uint32_t>(uint32_t x) {
  x &= 0x249249;
  x = (x | x >> 2) & 0xc30c3;
  x = (x | x >> 4) & 0xf00f;
  x = (x | x >> 8) & 0xff;
  return (uint8_t)x;
}

template <> inline uint64_t pad<2, uint16_t, uint64_t>(uint16_t v) {
  uint64_t x(v);
  x &= 0xffff;
  x = (x | x << 16) & 0xff0000ff;
  x = (x | x << 8) & 0xf00f00f00f;
  x = (x | x << 4) & 0xc30c30c30c3;
  x = (x | x << 2) & 0x249249249249;
  return x;
}

template <> inline uint16_t compact<2, uint16_t, uint64_t>(uint64_t x) {
  x &= 0x249249249249;
  x = (x | x >> 2) & 0xc30c30c30c3;
  x = (x | x >> 4) & 0xf00f00f00f;
  x = (x | x >> 8) & 0xff0000ff;
  x = (x | x >> 16) & 0xffff;
  return (uint16_t)x;
}

template <> inline uint64_t pad<3, uint16_t, uint64_t>(uint16_t v) {
  uint64_t x(v);
  x &= 0xffff;
  x = (x | x << 32) & 0xf800000007ff;
  x = (x | x << 16) & 0xf80007c0003f;
  x = (x | x << 8) & 0xc0380700c03807;
  x = (x | x << 4) & 0x843084308430843;
  x = (x | x << 2) & 0x909090909090909;
  x = (x | x << 1) & 0x1111111111111111;
  return x;
}

template <> inline uint16_t compact<3, uint16_t, uint64_t>(uint64_t x) {
  x &= 0x1111111111111111;
  x = (x | x >> 1) & 0x909090909090909;
  x = (x | x >> 2) & 0x843084308430843;
  x = (x | x >> 4) & 0xc0380700c03807;
  x = (x | x >> 8) & 0xf80007c0003f;
  x = (x | x >> 16) & 0xf800000007ff;
  x = (x | x >> 32) & 0xffff;
  return (uint16_t)x;
}

template <> inline uint128_t pad<1, uint32_t, uint128_t>(uint32_t v) {
  uint128_t x(v);
  x &= 0xffffffff_cppui128;
  x = (x | x << 16) & 0xffff0000ffff_cppui128;
  x = (x | x << 8) & 0xff00ff00ff00ff_cppui128;
  x = (x | x << 4) & 0xf0f0f0f0f0f0f0f_cppui128;
  x = (x | x << 2) & 0x3333333333333333_cppui128;
  x = (x | x << 1) & 0x5555555555555555_cppui128;
  return x;
}

template <> inline uint32_t compact<1, uint32_t, uint128_t>(uint128_t x) {

  x &= 0x5555555555555555_cppui128;
  x = (x | x >> 1) & 0x3333333333333333_cppui128;
  x = (x | x >> 2) & 0xf0f0f0f0f0f0f0f_cppui128;
  x = (x | x >> 4) & 0xff00ff00ff00ff_cppui128;
  x = (x | x >> 8) & 0xffff0000ffff_cppui128;
  x = (x | x >> 16) & 0xffffffff_cppui128;
  return (uint32_t)x;
}

template <> inline uint128_t pad<2, uint32_t, uint128_t>(uint32_t v) {

  uint128_t x(v);
  x &= 0xffffffff_cppui128;
  x = (x | x << 32) & 0xffff00000000ffff_cppui128;
  x = (x | x << 16) & 0xff0000ff0000ff0000ff_cppui128;
  x = (x | x << 8) & 0xf00f00f00f00f00f00f00f_cppui128;
  x = (x | x << 4) & 0xc30c30c30c30c30c30c30c3_cppui128;
  x = (x | x << 2) & 0x249249249249249249249249_cppui128;
  return x;
}

template <> inline uint32_t compact<2, uint32_t, uint128_t>(uint128_t x) {

  x &= 0x249249249249249249249249_cppui128;
  x = (x | x >> 2) & 0xc30c30c30c30c30c30c30c3_cppui128;
  x = (x | x >> 4) & 0xf00f00f00f00f00f00f00f_cppui128;
  x = (x | x >> 8) & 0xff0000ff0000ff0000ff_cppui128;
  x = (x | x >> 16) & 0xffff00000000ffff_cppui128;
  x = (x | x >> 32) & 0xffffffff_cppui128;
  return (uint32_t)x;
}

template <> inline uint128_t pad<3, uint32_t, uint128_t>(uint32_t v) {

  uint128_t x(v);
  x &= 0xffffffff_cppui128;
  x = (x | x << 64) & 0xffc0000000000000003fffff_cppui128;
  x = (x | x << 32) & 0xffc00000003ff800000007ff_cppui128;
  x = (x | x << 16) & 0xf80007c0003f0000f80007c0003f_cppui128;
  x = (x | x << 8) & 0xc0380700c0380700c0380700c03807_cppui128;
  x = (x | x << 4) & 0x8430843084308430843084308430843_cppui128;
  x = (x | x << 2) & 0x9090909090909090909090909090909_cppui128;
  x = (x | x << 1) & 0x11111111111111111111111111111111_cppui128;
  return x;
}

template <> inline uint32_t compact<3, uint32_t, uint128_t>(uint128_t x) {

  x &= 0x11111111111111111111111111111111_cppui128;
  x = (x | x >> 1) & 0x9090909090909090909090909090909_cppui128;
  x = (x | x >> 2) & 0x8430843084308430843084308430843_cppui128;
  x = (x | x >> 4) & 0xc0380700c0380700c0380700c03807_cppui128;
  x = (x | x >> 8) & 0xf80007c0003f0000f80007c0003f_cppui128;
  x = (x | x >> 16) & 0xffc00000003ff800000007ff_cppui128;
  x = (x | x >> 32) & 0xffc0000000000000003fffff_cppui128;
  x = (x | x >> 64) & 0xffffffff_cppui128;
  return (uint32_t)x;
}

template <> inline uint256_t pad<2, uint64_t, uint256_t>(uint64_t v) {

  uint256_t x(v);
  x &= 0xffffffffffffffff_cppui256;
  x = (x | x << 64) & 0xffffffff0000000000000000ffffffff_cppui256;
  x = (x | x << 32) & 0xffff00000000ffff00000000ffff00000000ffff_cppui256;
  x = (x | x << 16) & 0xff0000ff0000ff0000ff0000ff0000ff0000ff0000ff_cppui256;
  x = (x | x << 8) & 0xf00f00f00f00f00f00f00f00f00f00f00f00f00f00f00f_cppui256;
  x = (x | x << 4) & 0xc30c30c30c30c30c30c30c30c30c30c30c30c30c30c30c3_cppui256;
  x = (x | x << 2) & 0x249249249249249249249249249249249249249249249249_cppui256;
  return x;
}

template <> inline uint64_t compact<2, uint64_t, uint256_t>(uint256_t x) {

  x &= 0x249249249249249249249249249249249249249249249249_cppui256;
  x = (x | x >> 2) & 0xc30c30c30c30c30c30c30c30c30c30c30c30c30c30c30c3_cppui256;
  x = (x | x >> 4) & 0xf00f00f00f00f00f00f00f00f00f00f00f00f00f00f00f_cppui256;
  x = (x | x >> 8) & 0xff0000ff0000ff0000ff0000ff0000ff0000ff0000ff_cppui256;
  x = (x | x >> 16) & 0xffff00000000ffff00000000ffff00000000ffff_cppui256;
  x = (x | x >> 32) & 0xffffffff0000000000000000ffffffff_cppui256;
  x = (x | x >> 64) & 0xffffffffffffffff_cppui256;
  return (uint64_t)x;
}

template <> inline uint256_t pad<3, uint64_t, uint256_t>(uint64_t v) {

  uint256_t x(v);
  x &= 0xffffffffffffffff_cppui256;
  x = (x | x << 128) & 0xfffff800000000000000000000000000000007ffffffffff_cppui256;
  x = (x | x << 64) & 0xfffff80000000000000007ffffc0000000000000003fffff_cppui256;
  x = (x | x << 32) & 0xffc00000003ff800000007ff00000000ffc00000003ff800000007ff_cppui256;
  x = (x | x << 16) & 0xf80007c0003f0000f80007c0003f0000f80007c0003f0000f80007c0003f_cppui256;
  x = (x | x << 8) & 0xc0380700c0380700c0380700c0380700c0380700c0380700c0380700c03807_cppui256;
  x = (x | x << 4) & 0x843084308430843084308430843084308430843084308430843084308430843_cppui256;
  x = (x | x << 2) & 0x909090909090909090909090909090909090909090909090909090909090909_cppui256;
  x = (x | x << 1) & 0x1111111111111111111111111111111111111111111111111111111111111111_cppui256;
  return x;
}

template <> inline uint64_t compact<3, uint64_t, uint256_t>(uint256_t x) {

  x &= 0x1111111111111111111111111111111111111111111111111111111111111111_cppui256;
  x = (x | x >> 1) & 0x909090909090909090909090909090909090909090909090909090909090909_cppui256;
  x = (x | x >> 2) & 0x843084308430843084308430843084308430843084308430843084308430843_cppui256;
  x = (x | x >> 4) & 0xc0380700c0380700c0380700c0380700c0380700c0380700c0380700c03807_cppui256;
  x = (x | x >> 8) & 0xf80007c0003f0000f80007c0003f0000f80007c0003f0000f80007c0003f_cppui256;
  x = (x | x >> 16) & 0xffc00000003ff800000007ff00000000ffc00000003ff800000007ff_cppui256;
  x = (x | x >> 32) & 0xfffff80000000000000007ffffc0000000000000003fffff_cppui256;
  x = (x | x >> 64) & 0xfffff800000000000000000000000000000007ffffffffff_cppui256;
  x = (x | x >> 128) & 0xffffffffffffffff_cppui256;
  return (uint64_t)x;
}

/**
 * Interleaves an integer coordinate.
 *
 * @tparam ND Number of dimensions
 * @tparam IntT Intermediate integer type
 * @tparam MortonT Morton number type
 * @param coord Coordinate in intermediate integer space
 * @return Interleaved integer (Morton number)
 */
template <size_t ND, typename IntT, typename MortonT> MortonT interleave(const IntArray<ND, IntT> &coord) {
  MortonT retVal(0);
  for (size_t i = 0; i < ND; i++) {
    retVal |= pad<ND - 1, IntT, MortonT>(coord[i]) << static_cast<int>(i);
  }
  return retVal;
}

template <size_t ND, typename IntT> Morton96 interleave(const IntArray<3, uint32_t> &coord) {
  return Morton96(interleave<ND, IntT, uint128_t>(coord));
}

/**
 * Deinterleaves a Morton number into an integer coordinate.
 *
 * @tparam ND Number of dimensions
 * @tparam IntT Intermediate integer type
 * @tparam MortonT Morton number type
 * @param z Morton number
 * @return Integer coordinate
 */
template <size_t ND, typename IntT, typename MortonT> IntArray<ND, IntT> deinterleave(const MortonT z) {
  IntArray<ND, IntT> retVal;
  for (size_t i = 0; i < ND; i++) {
    retVal[i] = static_cast<IntT>(compact<ND - 1, IntT, MortonT>(z >> static_cast<int>(i)));
  }
  return retVal;
}

template <size_t ND, typename IntT> IntArray<ND, IntT> deinterleave(const Morton96 &z) {
  return deinterleave<ND, IntT, uint128_t>(uint128_t(z));
}

template <size_t ND, typename IntT, typename MortonT> struct Interleaver {
  static MortonT interleave(const IntArray<ND, IntT> &coord) {
    return morton_index::interleave<ND, IntT, MortonT>(coord);
  }

  static IntArray<ND, IntT> deinterleave(const MortonT z) { return morton_index::deinterleave<ND, IntT, MortonT>(z); }
};

template <size_t ND, typename IntT> struct Interleaver<ND, IntT, Morton96> {
  static Morton96 interleave(const IntArray<ND, IntT> &coord) { return morton_index::interleave<ND, IntT>(coord); }

  static IntArray<ND, IntT> deinterleave(const Morton96 z) { return morton_index::deinterleave<ND, IntT>(z); }
};

} // namespace morton_index
