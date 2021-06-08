// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidIndexing/LegacyConversion.h"

namespace Mantid {
namespace Indexing {

/// Converts a vector of specnum_t (int32_t) to vector of SpectrumNumber.
std::vector<SpectrumNumber> makeSpectrumNumberVector(const std::vector<int32_t> &data) {
  return {data.begin(), data.end()};
}

} // namespace Indexing
} // namespace Mantid
