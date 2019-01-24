// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_MORTONINDEX_TYPES_H_
#define MANTID_DATAOBJECTS_MORTONINDEX_TYPES_H_

#include <Eigen/Dense>
#include <boost/multiprecision/cpp_int.hpp>
#include <ostream>

namespace morton_index {

using uint128_t = boost::multiprecision::uint128_t;

template<size_t ND, typename IntT>
using IntArray = Eigen::Array<IntT, static_cast<int>(ND), 1>;

template<size_t ND>
using MDCoordinate = Eigen::Array<float, static_cast<int>(ND), 1>;

template<size_t ND>
using MDSpaceBounds = Eigen::Array<float, static_cast<int>(ND), 2>;
template<size_t ND>
using MDSpaceDimensions = Eigen::Array<float, static_cast<int>(ND), 1>;
template<size_t ND>
using MDSpaceSteps = Eigen::Array<float, static_cast<int>(ND), 1>;

template<typename CoordT, size_t ND>
using AffineND = Eigen::Transform<CoordT, static_cast<int>(ND), Eigen::Affine>;

template<size_t ND>
using BinIndices = Eigen::Matrix<size_t, 1, static_cast<int>(ND)>;

/**
 * This class implements the structure of size 96bit, that can be used
 * as Morton index.
 */
#pragma pack(push, 1)
struct Morton96 {
  uint64_t lower;
  uint32_t upper;

  explicit Morton96(const uint128_t &cmpl);
  template<typename IntT> Morton96(const IntT &smth_int);
  uint128_t to_uint128_t() const;

  bool operator<(const Morton96 &b) const;
  bool operator>(const Morton96 &b) const;
  bool operator>=(const Morton96 &b) const;
  bool operator<=(const Morton96 &b) const;
  bool operator==(const Morton96 &b) const;
  bool operator!=(const Morton96 &b) const;
  friend uint128_t operator-(const Morton96 &a, const Morton96 &b);
  template<typename T> uint128_t operator+(const T &b) const;
  Morton96 operator+(const Morton96 &b) const;
  template<typename T> uint128_t operator/(const T &b) const;
  template<typename T> uint128_t operator*(const T &b) const;
  friend std::ostream &operator<<(std::ostream &os, const Morton96 &morton96);
};
#pragma pack(pop)

inline Morton96::Morton96(const uint128_t &smpl) {
  union {
    uint128_t smpl;   // simple
    uint64_t cmpl[2]; // complex
  } uni = {smpl};
  upper = static_cast<uint32_t>(uni.cmpl[1]);
  lower = uni.cmpl[0];
}

template<typename IntT>
inline Morton96::Morton96(const IntT &smth_int)
    : Morton96(uint128_t(smth_int)) {}

inline uint128_t Morton96::to_uint128_t() const {
  union {
    uint64_t cmpl[2]; // complex
    uint128_t smpl;   // simple
  } uni = {{lower, upper}};
  return uni.smpl;
}

inline bool Morton96::operator<(const Morton96 &b) const {
  if (upper != b.upper)
    return upper < b.upper;
  else
    return lower < b.lower;
}

inline bool Morton96::operator>(const Morton96 &b) const {
  if (upper != b.upper)
    return upper > b.upper;
  else
    return lower > b.lower;
}

inline bool Morton96::operator==(const Morton96 &b) const {

  return lower == b.lower && upper == b.upper;
}

inline bool Morton96::operator>=(const Morton96 &b) const {
  return *this > b || *this == b;
}

inline bool Morton96::operator<=(const Morton96 &b) const {

  return *this < b || *this == b;
}

inline bool Morton96::operator!=(const Morton96 &b) const {

  return lower != b.lower || upper != b.upper;
}

inline uint128_t operator-(const Morton96 &a, const Morton96 &b) {
  return a.to_uint128_t() - b.to_uint128_t();
}

template<typename T> inline uint128_t Morton96::operator+(const T &b) const {
  return to_uint128_t() + b;
}

inline Morton96 Morton96::operator+(const Morton96 &b) const {
  return Morton96(to_uint128_t() + b.to_uint128_t());
}

template<typename T> inline uint128_t Morton96::operator/(const T &b) const {
  return to_uint128_t() / b;
}

template<typename T> inline uint128_t Morton96::operator*(const T &b) const {
  return to_uint128_t() * b;
}

inline std::ostream &operator<<(std::ostream &os, const Morton96 &morton96) {
  os << morton96.to_uint128_t();
  return os;
}

/**
 * This structure binds the size of accesible memory to store
 * the Morton index to the Morton index type. Typically this
 * size is number_of_dimensions * sizeof(coordinate_type)
 * @tparam SZ :: the size of accesible memory.
 */
template<size_t SZ> struct MortonIndex {
  using type = boost::multiprecision::uint256_t;
};

template<> struct MortonIndex<1> { using type = uint8_t; };

template<> struct MortonIndex<2> { using type = uint16_t; };

template<> struct MortonIndex<4> { using type = uint32_t; };

template<> struct MortonIndex<8> { using type = uint64_t; };

template<> struct MortonIndex<12> { using type = Morton96; };

template<> struct MortonIndex<16> { using type = uint128_t; };

/**
 * This structure binds floating point types to
 * the unsigned integer types of the same width
 * @tparam FP :: floating point type.
 */
template<typename FP> struct UnderlyingInt {};

template<> struct UnderlyingInt<float> { using type = uint32_t; };

template<> struct UnderlyingInt<double> { using type = uint64_t; };

/**
 * This structure determines Morton index type and
 * underlying unsigned integer type for the floating
 * point coordinate type and number of dimensions
 * @tparam ND :: number of dimensions
 * @tparam FP :: floating point type
 */
template<size_t ND, typename FP> struct IndexTypes {
  using MortonType = typename MortonIndex<ND * sizeof(FP)>::type;
  using IntType = typename UnderlyingInt<FP>::type;
};

} // morton_index
#endif // MANTID_DATAOBJECTS_MORTONINDEX_TYPES_H_
