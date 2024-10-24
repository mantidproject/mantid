// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidHistogramData/DllConfig.h"
#include <stdexcept>

namespace Mantid {
namespace HistogramData {
namespace Exception {

/// Thrown whenever invalid bin edges are encountered by HistogramData Rebin
class MANTID_HISTOGRAMDATA_DLL InvalidBinEdgesError : public std::runtime_error {
public:
  InvalidBinEdgesError(const char *what);
};
} // namespace Exception
} // namespace HistogramData
} // namespace Mantid
