// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
namespace Mantid {
namespace HistogramData {
class Histogram;
class BinEdges;

MANTID_HISTOGRAMDATA_DLL Histogram rebin(const Histogram &input, const BinEdges &binEdges);
} // namespace HistogramData
} // namespace Mantid
