// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidHistogramData/Exception.h"

namespace Mantid {
namespace HistogramData {
namespace Exception {
InvalidBinEdgesError::InvalidBinEdgesError(const char *what) : runtime_error(what) {}
} // namespace Exception
} // namespace HistogramData
} // namespace Mantid