// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// boost::multiprecision is not used here because the
// with of the number is one WORD bigger, than actual
// number size, see here
// https://www.boost.org/doc/libs/1_61_0/libs/multiprecision/doc/html/boost_multiprecision/tut/ints/cpp_int.html
#include "WideInt.h"
#include <Eigen/Dense>
#include <ostream>

namespace morton_index {
using std::operator"" _cppui128;
using std::operator"" _cppui256;
using uint96_t = std::wide_uint<96>;
using uint128_t = std::uint128_t;
using uint256_t = std::uint256_t;
} // namespace morton_index

namespace morton_index {

template <size_t ND, typename IntT> using IntArray = Eigen::Array<IntT, static_cast<int>(ND), 1>;

template <size_t ND> using MDCoordinate = Eigen::Array<float, static_cast<int>(ND), 1>;

template <size_t ND> using MDSpaceBounds = Eigen::Array<float, static_cast<int>(ND), 2>;
template <size_t ND> using MDSpaceDimensions = Eigen::Array<float, static_cast<int>(ND), 1>;
template <size_t ND> using MDSpaceSteps = Eigen::Array<float, static_cast<int>(ND), 1>;

template <typename CoordT, size_t ND> using AffineND = Eigen::Transform<CoordT, static_cast<int>(ND), Eigen::Affine>;

template <size_t ND> using BinIndices = Eigen::Matrix<size_t, 1, static_cast<int>(ND)>;

/**
 * This class implements the structure of size 96bit, that can be used
 * as Morton index.
 */
using Morton96 = uint96_t;

/**
 * This structure binds the size of accesible memory to store
 * the Morton index to the Morton index type. Typically this
 * size is number_of_dimensions * sizeof(coordinate_type)
 * @tparam SZ :: the size of accesible memory.
 */
template <size_t SZ> struct MortonIndex { using type = uint256_t; };

template <> struct MortonIndex<1> { using type = uint8_t; };

template <> struct MortonIndex<2> { using type = uint16_t; };

template <> struct MortonIndex<4> { using type = uint32_t; };

template <> struct MortonIndex<8> { using type = uint64_t; };

template <> struct MortonIndex<12> { using type = Morton96; };

template <> struct MortonIndex<16> { using type = uint128_t; };

/**
 * This structure binds floating point types to
 * the unsigned integer types of the same width
 * @tparam FP :: floating point type.
 */
template <typename FP> struct UnderlyingInt {};

template <> struct UnderlyingInt<float> { using type = uint32_t; };

template <> struct UnderlyingInt<double> { using type = uint64_t; };

/**
 * This structure determines Morton index type and
 * underlying unsigned integer type for the floating
 * point coordinate type and number of dimensions
 * @tparam ND :: number of dimensions
 * @tparam FP :: floating point type
 */
template <size_t ND, typename FP> struct IndexTypes {
  using MortonType = typename MortonIndex<ND * sizeof(FP)>::type;
  using IntType = typename UnderlyingInt<FP>::type;
};

} // namespace morton_index
