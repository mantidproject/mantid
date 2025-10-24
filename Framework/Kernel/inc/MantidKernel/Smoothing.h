// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <vector>

namespace Mantid::Kernel {

namespace Smoothing {

/** Performs boxcar (moving average) smoothing on the input data.
 *
 * @param input The input vector to be smoothed
 * @param numPoints The width of the boxcar window (must be >= 3)
 * @return A new vector containing the smoothed data
 * @throw std::invalid_argument if numPoints is too small
 * @tparam T Numeric type (e.g., int, float, double)
 */
template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned const numPoints);

/** Performs boxcar (moving average) smoothing on the input data,
 * using error propagation formula.  This is the correct method to use
 * for smoothing histogram errors after their y-values have been smoothed.
 *
 * @param input The input vector to be smoothed
 * @param numPoints The width of the boxcar window (must be >= 3)
 * @return A new vector containing the smoothed data
 * @throw std::invalid_argument if numPoints is too small
 * @tparam T Numeric type (e.g., int, float, double)
 */
template <typename T> std::vector<T> boxcarErrorSmooth(std::vector<T> const &input, unsigned const numPoints);

/** Performs boxcar (moving average) smoothing on the input data,
 * using a RMSE average, as is appropriate for error averaging.
 *
 * @param input The input vector to be smoothed
 * @param numPoints The width of the boxcar window (must be >= 3)
 * @return A new vector containing the smoothed data
 * @throw std::invalid_argument if numPoints is too small
 * @tparam T Numeric type (e.g., int, float, double)
 */
template <typename T> std::vector<T> boxcarRMSESmooth(std::vector<T> const &input, unsigned const numPoints);

} // namespace Smoothing
} // namespace Mantid::Kernel
