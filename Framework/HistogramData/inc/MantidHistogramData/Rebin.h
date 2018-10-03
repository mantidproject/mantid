// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMREBIN_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMREBIN_H_

#include "MantidHistogramData/DllConfig.h"
namespace Mantid {
namespace HistogramData {
class Histogram;
class BinEdges;

MANTID_HISTOGRAMDATA_DLL Histogram rebin(const Histogram &input,
                                         const BinEdges &binEdges);
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMREBIN_H_ */