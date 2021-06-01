// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidHistogramData/Histogram.h"

#include <algorithm>
#include <utility>

namespace Mantid {
namespace CurveFitting {

namespace EstimatePeakParameters {
enum EstimatePeakWidth { NoEstimation, Observation, InstrumentResolution };
enum PeakFitResult { NOSIGNAL, LOWPEAK, OUTOFBOUND, GOOD };
} // namespace EstimatePeakParameters

/// observe peak center
int observePeakCenter(const HistogramData::Histogram &histogram, API::FunctionValues &bkgd_values, size_t start_index,
                      size_t stop_index, double &peak_center, size_t &peak_center_index, double &peak_height);

/// Observe peak width
double observePeakFwhm(const HistogramData::Histogram &histogram, API::FunctionValues &bkgd_values, size_t ipeak,
                       size_t istart, size_t istop,
                       const EstimatePeakParameters::EstimatePeakWidth peakWidthEstimateApproach,
                       const double peakWidthPercentage);

/// Estimate peak parameters by 'observation'
int estimatePeakParameters(const HistogramData::Histogram &histogram, const std::pair<size_t, size_t> &peak_window,
                           const API::IPeakFunction_sptr &peakfunction,
                           const API::IBackgroundFunction_sptr &bkgdfunction, bool observe_peak_width,
                           const EstimatePeakParameters::EstimatePeakWidth peakWidthEstimateApproach,
                           const double peakWidthPercentage, const double minPeakHeight);

} // namespace CurveFitting
} // namespace Mantid
