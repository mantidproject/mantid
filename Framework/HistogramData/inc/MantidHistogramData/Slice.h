// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_SLICE_H_
#define MANTID_HISTOGRAMDATA_SLICE_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace HistogramData {

/** Slicing function for Histogram.

  @author Simon Heybrock
  @date 2017
*/
MANTID_HISTOGRAMDATA_DLL Histogram slice(const Histogram &histogram,
                                         const size_t begin, const size_t end);

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_SLICE_H_ */
