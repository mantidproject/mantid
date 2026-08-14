// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

#include <span>
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
template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned int const numPoints);

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
template <typename T> std::vector<T> boxcarErrorSmooth(std::vector<T> const &input, unsigned int const numPoints);

/** Overloads taking a contiguous range of doubles, so that the size-checked histogram data
 * types, which do not hand out a std::vector<double>, can be smoothed directly. A
 * std::vector<double> argument still selects the templates above; deduction cannot produce a
 * std::span, hence these are not templates.
 *
 * @param input The contiguous range to be smoothed
 * @param numPoints The width of the boxcar window (must be >= 3)
 * @return A new vector containing the smoothed data
 * @throw std::invalid_argument if numPoints is too small
 */
MANTID_KERNEL_DLL std::vector<double> boxcarSmooth(std::span<double const> input, unsigned int const numPoints);
MANTID_KERNEL_DLL std::vector<double> boxcarErrorSmooth(std::span<double const> input, unsigned int const numPoints);

/** Performs boxcar (moving average) smoothing on the input data,
 * using a RMSE average, as is appropriate for error averaging.
 *
 * @param input The input vector to be smoothed
 * @param numPoints The width of the boxcar window (must be >= 3)
 * @return A new vector containing the smoothed data
 * @throw std::invalid_argument if numPoints is too small
 * @tparam T Numeric type (e.g., int, float, double)
 */
template <typename T> std::vector<T> boxcarRMSESmooth(std::vector<T> const &input, unsigned int const numPoints);

/** Performs FFT smoothing on the input data, with high frequencies set to zero
 *  NOTE: the input data MUST be defined on a uniform grid
 *
 * @param input The input vector to be smoothed
 * @param cutoff The cutoff frequency; all components from this number forward will be set to zero
 * @return A new vector containing the smoothed data
 * @tparam Y numeric type for y-values
 */
template <typename Y> std::vector<Y> fftSmooth(std::vector<Y> const &input, unsigned const cutoff);

/** Performs FFT smoothing on the input data, using a Butterworth filter
 *  NOTE: the input data MUST be defined on a uniform grid
 *
 * @param input The input vector to be smoothed
 * @param cutoff Represents the cutoff frequency, where step function behavior begins descent
 * @param order Represents the steepness of the frequency cutoff; as this approaches infinity, approaches step cutoff
 * @return A new vector containing the smoothed data
 * @tparam Y numeric type for y-values
 */
template <typename Y>
std::vector<Y> fftButterworthSmooth(std::vector<Y> const &input, unsigned const cutoff, unsigned const order);

} // namespace Smoothing
} // namespace Mantid::Kernel
