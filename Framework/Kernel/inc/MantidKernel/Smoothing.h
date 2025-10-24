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

template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned const numPoints);

template <typename T> std::vector<T> boxcarSumSquareSmooth(std::vector<T> const &input, unsigned const numPoints);

} // namespace Smoothing
} // namespace Mantid::Kernel
