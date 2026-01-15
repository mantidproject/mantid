// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <atomic>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {
struct SpectraProcessingData {
  // There must be the same number of counts and binedges entries
  std::vector<std::vector<std::atomic_uint32_t>> counts; // [spectrum][bin]
  std::vector<const std::vector<double> *> binedges;
};

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
