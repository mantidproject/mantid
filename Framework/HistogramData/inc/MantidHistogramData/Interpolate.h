// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_INTERPOLATE_H_
#define MANTID_HISTOGRAMDATA_INTERPOLATE_H_

#include "MantidHistogramData/DllConfig.h"

#include <cstddef>

namespace Mantid {
namespace HistogramData {
class Histogram;

/**
  Defines public functions to perform interpolation of histogram data.
*/

MANTID_HISTOGRAMDATA_DLL Histogram interpolateLinear(const Histogram &input,
                                                     const size_t stepSize);

MANTID_HISTOGRAMDATA_DLL void interpolateLinearInplace(Histogram &inOut,
                                                       const size_t stepSize);

MANTID_HISTOGRAMDATA_DLL void interpolateLinearInplace(const Histogram &input,
                                                       Histogram &output);

MANTID_HISTOGRAMDATA_DLL Histogram interpolateCSpline(const Histogram &input,
                                                      const size_t stepSize);

MANTID_HISTOGRAMDATA_DLL void interpolateCSplineInplace(Histogram &inOut,
                                                        const size_t stepSize);

MANTID_HISTOGRAMDATA_DLL void interpolateCSplineInplace(const Histogram &input,
                                                        Histogram &output);
MANTID_HISTOGRAMDATA_DLL
size_t minSizeForCSplineInterpolation();

MANTID_HISTOGRAMDATA_DLL
size_t minSizeForLinearInterpolation();

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_INTERPOLATE_H_ */
