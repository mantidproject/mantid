// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_DATAOBJECTS_MORTONINDEX_COORDINATE_CONVERSION_H_
#define MANTID_DATAOBJECTS_MORTONINDEX_COORDINATE_CONVERSION_H_

#include "Types.h"
#include <algorithm>
#include <cmath>
#include <iomanip>

namespace morton_index {
/**
 * Checks if a point defined by a Morton number is within the box bounds (as
 * defined by an upper and lower Morton number).
 *
 * A box contains a Morton number if it is within the upper and lower bounds,
 * inclusive of the box bounds themselves.
 *
 * @param lower Lower bound
 * @param upper Upper bound
 * @param value Morton value to test
 */
template <typename MortonT>
bool morton_contains(const MortonT lower, const MortonT upper,
                     const MortonT value) {
  return lower <= value && value <= upper;
}

template <size_t ND, typename IntT, typename MortonT>
MortonT calculateDefaultBound(IntT intBound) {
  IntArray<ND, IntT> minCoord;
  minCoord.fill(intBound);
  return Interleaver<ND, IntT, MortonT>::interleave(minCoord);
}

/**
 * Calculates the required width of the interleaved integer to accurately
 * represent coordinates in a given space at a given resolution.
 */
template <int ND>
size_t CalculateRequiredCoordinateIntegerWidth(const MDSpaceBounds<ND> &bounds,
                                               const MDSpaceSteps<ND> &steps) {
  size_t maxBits(0);
  for (size_t i = 0; i < ND; ++i) {
    /* Calculate the required integer width to accurately represent coordinates
     * on axis i. */
    size_t bits =
        std::ceil(std::log2((bounds(i, 1) - bounds(i, 0)) / steps(i)));

    /* Record highest bit width requirement */
    maxBits = std::max(maxBits, bits);
  }
  return maxBits;
}

/**
 * Expands a coordinate space enough that floating point error cannot cause an
 * overflow when mapping a value at the upper limit of the range to an integer
 * range.
 *
 * Sets bounds to the next lowest (for lower axis bound) or highest (for upper
 * axis bound) representable floating point value.
 */
template <int ND> void ExpandBounds(MDSpaceBounds<ND> &bounds) {
  for (size_t i = 0; i < ND; ++i) {
    bounds(i, 0) =
        std::nexttoward(bounds(i, 0), std::numeric_limits<float>::lowest());
    bounds(i, 1) =
        std::nexttoward(bounds(i, 1), std::numeric_limits<float>::max());
  }
}

/**
 * Checks that a coordinate is within the extents of an MD space.
 */
template <int ND>
bool CheckCoordinatesInMDSpace(const MDSpaceBounds<ND> &bounds,
                               const MDCoordinate<ND> &coord) {
  for (size_t i = 0; i < coord.rows(); i++) {
    const float coordValue = coord(i, 0);
    const Eigen::Matrix<float, 1, 2> coordBounds = bounds.row(i);

    if (coordValue < coordBounds[0] || coordValue > coordBounds[1]) {
      return false;
    }
  }

  return true;
}

/**
 * Converts a point to integer range given a range of floating point
 * coordinates.
 */
template <int ND, typename IntT>
Eigen::Array<IntT, ND, 1>
ConvertCoordinatesToIntegerRange(const MDSpaceBounds<ND> &bounds,
                                 const float *crd) {
  MDCoordinate<ND> coord(crd);
  const MDCoordinate<ND> range = bounds.col(1) - bounds.col(0);
  const MDCoordinate<ND> coordFactorOfRange = (coord - bounds.col(0)) / range;
  const MDCoordinate<ND> n =
      coordFactorOfRange * std::numeric_limits<IntT>::max();
  Eigen::Array<IntT, ND, 1> res = n.template cast<IntT>();
  for (unsigned i = 0; i < ND; ++i)
    if (coord[i] == bounds(i, 1))
      res[i] = std::numeric_limits<IntT>::max();
    else if (coord[i] == bounds(i, 0))
      res[i] = 0;
  return res;
}

/**
 * Converts a point expressed as integer coordinates back to floating point,
 * given bounds of the original coordinate space.
 */
template <int ND, typename IntT>
MDCoordinate<ND>
ConvertCoordinatesFromIntegerRange(const MDSpaceBounds<ND> &bounds,
                                   const Eigen::Array<IntT, ND, 1> &intCoord) {
  const auto range = bounds.col(1) - bounds.col(0);
  const auto coordFactorOfRange =
      intCoord.template cast<float>() / (float)std::numeric_limits<IntT>::max();
  return bounds.col(0) + (coordFactorOfRange * range);
}

/**
 * Converts a single floating point coordinate to an integer range.
 */
template <typename IntT>
IntT ConvertCoordinateToIntegerRange(float value, float lower, float upper) {
  const float coordFactorOfRange = (value - lower) / (upper - lower);
  const float intValue = coordFactorOfRange * std::numeric_limits<IntT>::max();
  return (IntT)intValue;
}

/**
 * Converts a single integer coordinate to a floating point.
 */
template <typename IntT>
float ConvertCoordinateFromIntegerRange(IntT value, float lower, float upper) {
  const float coordFactorOfRange =
      (float)value / (float)std::numeric_limits<IntT>::max();
  return lower + (coordFactorOfRange * (upper - lower));
}

template <size_t nd, typename IntT, typename MortonT>
MDCoordinate<nd> indexToCoordinates(const MortonT &idx,
                                    const MDSpaceBounds<nd> &space) {
  return morton_index::ConvertCoordinatesFromIntegerRange<nd, IntT>(
      space, Interleaver<nd, IntT, MortonT>::deinterleave(idx));
}

template <size_t nd, typename IntT, typename MortonT, typename coord_t = float>
MortonT coordinatesToIndex(coord_t *coord, const MDSpaceBounds<nd> &space) {
  return Interleaver<nd, IntT, MortonT>::interleave(
      morton_index::ConvertCoordinatesToIntegerRange<nd, IntT>(space, coord));
}

} // namespace morton_index

#endif // MANTID_DATAOBJECTS_MORTONINDEX_COORDINATE_CONVERSION_H_