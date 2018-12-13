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

using uint128_t = boost::multiprecision::uint128_t;

template <size_t ND, typename IntT> using IntArray = Eigen::Array<IntT, static_cast<int>(ND), 1>;

template <size_t ND> using MDCoordinate = Eigen::Array<float, static_cast<int>(ND), 1>;

template <size_t ND> using MDSpaceBounds = Eigen::Array<float, static_cast<int>(ND), 2>;
template <size_t ND> using MDSpaceDimensions = Eigen::Array<float, static_cast<int>(ND), 1>;
template <size_t ND> using MDSpaceSteps = Eigen::Array<float, static_cast<int>(ND), 1>;

template <typename CoordT, size_t ND>
using AffineND = Eigen::Transform<CoordT, static_cast<int>(ND), Eigen::Affine>;

template <size_t ND> using BinIndices = Eigen::Matrix<size_t, 1, static_cast<int>(ND)>;

#pragma pack(push ,1)
struct Morton96 {
  uint64_t lower;
  uint32_t upper;

  Morton96(const uint128_t& cmpl);
  template <typename IntT>
  Morton96(const IntT& smth_int);
  uint128_t to_uint128_t() const;

  friend bool operator<(const Morton96& a, const Morton96& b);
  friend bool operator>(const Morton96& a, const Morton96& b);
  friend bool operator>=(const Morton96& a, const Morton96& b);
  friend bool operator<=(const Morton96& a, const Morton96& b);
  friend bool operator==(const Morton96& a, const Morton96& b);
  friend bool operator!=(const Morton96& a, const Morton96& b);
  friend uint128_t operator-(const Morton96& a, const Morton96& b);
  template <typename T>
  uint128_t operator+(const T& b) const;
  Morton96 operator+(const Morton96& b) const;
  template <typename T>
  uint128_t operator/(const T& b)  const;
  template <typename T>
  uint128_t operator*(const T& b)  const;
};
#pragma pack(pop)

inline Morton96::Morton96(const uint128_t &smpl) {
  union {
    uint128_t smpl; //simple
    uint64_t cmpl[2]; // complex
  } uni = {smpl};
  upper = static_cast<uint32_t>(uni.cmpl[1]);
  lower = uni.cmpl[0];
}

template <typename IntT>
inline Morton96::Morton96(const IntT& smth_int) : Morton96(uint128_t(smth_int)) {}

inline uint128_t Morton96::to_uint128_t() const{
  union {
    uint64_t cmpl[2]; // complex
    uint128_t smpl; //simple
  } uni = {{lower, upper}};
  return uni.smpl;
}

inline bool operator<(const Morton96& a, const Morton96& b) {
  if(a.upper != b.upper)
    return a.upper < b.upper;
  else
    return a.lower < b.lower;
}

inline bool operator>(const Morton96& a, const Morton96& b) {
  if(a.upper != b.upper)
    return a.upper > b.upper;
  else
    return a.lower > b.lower;
}

inline bool operator==(const Morton96& a, const Morton96& b) {

  return a.lower == b.lower && a.upper == b.upper;
}

inline bool operator>=(const Morton96& a, const Morton96& b) {
  return a > b || a == b;
}

inline bool operator<=(const Morton96& a, const Morton96& b) {

  return a < b || a == b;
}

inline bool operator!=(const Morton96& a, const Morton96& b) {

  return a.lower != b.lower || a.upper != b.upper;
}

inline uint128_t operator-(const Morton96& a, const Morton96& b) {
  return a.to_uint128_t() - b.to_uint128_t();
}

template <typename T>
inline uint128_t Morton96::operator+(const T& b)  const {
  return to_uint128_t() + b;
}

inline Morton96 Morton96::operator+(const Morton96& b) const {
  return Morton96(to_uint128_t() + b.to_uint128_t());
}

template <typename T>
inline uint128_t Morton96::operator/(const T& b)  const {
  return to_uint128_t() / b;
}

template <typename T>
inline uint128_t Morton96::operator*(const T& b)  const {
  return to_uint128_t() * b;
}

#endif // MANTID_DATAOBJECTS_MORTONINDEX_TYPES_H_
