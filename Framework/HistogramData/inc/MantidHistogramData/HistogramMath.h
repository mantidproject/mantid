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

/** This header provides basic math operations for Histograms.

  @author Simon Heybrock
  @date 2016
*/
MANTID_HISTOGRAMDATA_DLL Histogram &operator*=(Histogram &histogram, const double factor);
MANTID_HISTOGRAMDATA_DLL Histogram &operator/=(Histogram &histogram, const double factor);
MANTID_HISTOGRAMDATA_DLL Histogram operator*(Histogram histogram, const double factor);
MANTID_HISTOGRAMDATA_DLL Histogram operator*(const double factor, Histogram histogram);
MANTID_HISTOGRAMDATA_DLL Histogram operator/(Histogram histogram, const double factor);

MANTID_HISTOGRAMDATA_DLL Histogram &operator+=(Histogram &histogram, const Histogram &other);
MANTID_HISTOGRAMDATA_DLL Histogram &operator-=(Histogram &histogram, const Histogram &other);
MANTID_HISTOGRAMDATA_DLL Histogram &operator*=(Histogram &histogram, const Histogram &other);
MANTID_HISTOGRAMDATA_DLL Histogram &operator/=(Histogram &histogram, const Histogram &other);
MANTID_HISTOGRAMDATA_DLL Histogram operator+(Histogram histogram, const Histogram &other);
MANTID_HISTOGRAMDATA_DLL Histogram operator-(Histogram histogram, const Histogram &other);
MANTID_HISTOGRAMDATA_DLL Histogram operator*(Histogram histogram, const Histogram &other);
MANTID_HISTOGRAMDATA_DLL Histogram operator/(Histogram histogram, const Histogram &other);

} // namespace HistogramData
} // namespace Mantid
