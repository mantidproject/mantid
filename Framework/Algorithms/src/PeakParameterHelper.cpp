// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PeakParameterHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using Mantid::HistogramData::Histogram;

namespace Mantid {
namespace Algorithms {
namespace PeakParameterHelper {

/** Get an index of a value in a sorted vector.  The index should be the item
 * with value nearest to X
 */
template <typename vector_like> size_t findXIndex(const vector_like &vecx, const double x, const size_t startindex) {
  size_t index;
  if (x <= vecx.front()) {
    index = 0;
  } else if (x >= vecx.back()) {
    index = vecx.size() - 1;
  } else {
    const auto fiter = std::lower_bound(vecx.cbegin() + startindex, vecx.cend(), x);
    if (fiter == vecx.cend())
      throw std::runtime_error("It seems impossible to have this value. ");

    index = static_cast<size_t>(fiter - vecx.cbegin());
    if (index > 0 && (x - vecx[index - 1] < vecx[index] - x))
      --index;
  }

  return index;
}

//----------------------------------------------------------------------------------------------
/** Guess/estimate peak center and thus height by observation
 * @param histogram :: Histogram instance
 * @param bkgd_values :: calculated background value to removed from observed
 * data
 * @param start_index :: X index of the left boundary of observation widow
 * @param stop_index :: X index of the right boundary of the observation window
 * @param peak_center :: estiamted peak center (output)
 * @param peak_center_index :: estimated peak center's index in histogram
 * (output)
 * @param peak_height :: estimated peak height (output)
 * @return :: state whether peak center can be found by obervation
 */
int observePeakCenter(const Histogram &histogram, FunctionValues &bkgd_values, size_t start_index, size_t stop_index,
                      double &peak_center, size_t &peak_center_index, double &peak_height) {
  const auto &vector_x = histogram.points();

  // find the original starting point
  auto peak_center_iter = std::lower_bound(vector_x.begin() + start_index, vector_x.begin() + stop_index, peak_center);
  if (peak_center_iter == vector_x.begin() + stop_index)
    return OUTOFBOUND; // suggested center is not in the window
  peak_center_index = static_cast<size_t>(peak_center_iter - vector_x.begin());

  // initialize the search to that in case something goes wrong below
  const auto &vector_y = histogram.y();
  peak_center = *peak_center_iter; // set the value in case something goes wrong below
  peak_height = vector_y[peak_center_index] - bkgd_values.getCalculated(peak_center_index - start_index);

  // assume that the actual peak is within 40% (in index number)
  // of the window size of the suggested peak. This assumes that
  // the minimum search size is 5 bins (arbitrary).
  const size_t windowSize = stop_index - start_index;
  const size_t searchBox = std::max(static_cast<size_t>(.3 * static_cast<double>(windowSize)), static_cast<size_t>(5));
  size_t left = std::max(peak_center_index - searchBox, start_index);
  if (searchBox > peak_center_index) {
    // prevent overflow since these are unsigned
    left = start_index;
  }
  const size_t rght = std::min(peak_center_index + searchBox, stop_index);

  for (size_t i = left; i < rght; ++i) {
    const double y = vector_y[i] - bkgd_values.getCalculated(i - start_index);
    if (y > peak_height) {
      peak_height = y;
      peak_center = vector_x[i];
      peak_center_index = i;
    }
  }

  return GOOD;
}

//----------------------------------------------------------------------------------------------
/** estimate peak width from 'observation'
 * @param histogram :: Histogram instance
 * @param bkgd_values :: (output) background values calculated from X in given
 * histogram
 * @param ipeak :: array index for the peak center in histogram
 * @param istart :: array index for the left boundary of the peak
 * @param istop :: array index for the right boundary of the peak
 * @param peakWidthEstimateApproach :: whether to guess fwhm from instrument or observation
 * @param peakWidthPercentage :: fwhm scaling factor when guessing from instrument
 * @return peak width as double
 */
double observePeakFwhm(const Histogram &histogram, FunctionValues &bkgd_values, size_t ipeak, size_t istart,
                       size_t istop, const EstimatePeakWidth peakWidthEstimateApproach,
                       const double peakWidthPercentage) {
  double peak_fwhm(-0.);

  if (peakWidthEstimateApproach == EstimatePeakWidth::InstrumentResolution) {
    // width from guessing from delta(D)/D
    const double peak_center = histogram.points()[ipeak];
    peak_fwhm = peak_center * peakWidthPercentage;
  } else if (peakWidthEstimateApproach == EstimatePeakWidth::Observation) {
    // estimate fwhm from area assuming Gaussian (more robust than using
    // moments which overestimates variance by factor ~5 depending on background
    // estimation over window much wider than peak)
    const auto &x_vec = histogram.points();
    const auto &y_vec = histogram.y();
    const size_t numPoints = std::min(istart - istop, bkgd_values.size()) - 1;

    double area = 0.0;
    // integrate using Newton's method
    for (size_t i = 0; i < numPoints; ++i) {
      // skip counting area if negative to give a better fwhm estimate
      if (y_vec[istart + i] < 0.0) {
        continue;
      }
      const auto yavg = 0.5 * (y_vec[istart + i] - bkgd_values.getCalculated(i) + y_vec[istart + i + 1] -
                               bkgd_values.getCalculated(i + 1));
      const auto dx = x_vec[istart + i + 1] - x_vec[istart + i];
      area += yavg * dx;
    }
    peak_fwhm = 2 * std::sqrt(M_LN2 / M_PI) * area / y_vec[ipeak];
  } else {
    // get from last peak or from input!
    throw std::runtime_error("This case for observing peak width is not supported.");
  }

  return peak_fwhm;
}

//----------------------------------------------------------------------------------------------
/** Estimate peak profile's parameters values via observation
 * including
 * (1) peak center (2) peak intensity  (3) peak width depending on peak type
 * In order to make the estimation better, a pre-defined background function is
 * used to remove
 * the background from the obervation data
 * @param histogram :: Histogram instance containing experimental data
 * @param peak_window :: pair of X-value to specify the peak range
 * @param peakfunction :: (in/out) peak function to set observed value to
 * @param bkgdfunction :: background function previously defined
 * @param observe_peak_width :: flag to estimate peak width from input data
 * @param peakWidthEstimateApproach :: whether to guess fwhm from instrument or observation
 * @param peakWidthPercentage :: fwhm scaling factor when guessing from instrument
 * @param minPeakHeight :: function parameters will only be set if estimated peak height is larger
 * @return :: obervation success or not
 */
int estimatePeakParameters(const Histogram &histogram, const std::pair<size_t, size_t> &peak_window,
                           const API::IPeakFunction_sptr &peakfunction,
                           const API::IBackgroundFunction_sptr &bkgdfunction, bool observe_peak_width,
                           const EstimatePeakWidth peakWidthEstimateApproach, const double peakWidthPercentage,
                           const double minPeakHeight) {

  // calculate background
  const auto &vector_x = histogram.points();
  FunctionDomain1DVector domain(vector_x.cbegin() + peak_window.first, vector_x.cbegin() + peak_window.second);
  FunctionValues bkgd_values(domain);
  bkgdfunction->function(domain, bkgd_values);

  // Estimate peak center
  double peak_height;
  double peak_center = peakfunction->centre();
  size_t peak_center_index;

  int result = observePeakCenter(histogram, bkgd_values, peak_window.first, peak_window.second, peak_center,
                                 peak_center_index, peak_height);

  // return if failing to 'observe' peak center
  if (result != GOOD)
    return result;

  if (peak_height < minPeakHeight || std::isnan(peak_height))
    return LOWPEAK;

  // use values from background to locate FWHM
  peakfunction->setHeight(peak_height);
  peakfunction->setCentre(peak_center);

  // Estimate FHWM (peak width)
  if (observe_peak_width && peakWidthEstimateApproach != EstimatePeakWidth::NoEstimation) {
    const double peak_fwhm = observePeakFwhm(histogram, bkgd_values, peak_center_index, peak_window.first,
                                             peak_window.second, peakWidthEstimateApproach, peakWidthPercentage);
    if (peak_fwhm > 0.0) {
      peakfunction->setFwhm(peak_fwhm);
    }
  }

  return GOOD;
}

template MANTID_ALGORITHMS_DLL size_t findXIndex(const HistogramData::Points &, const double, const size_t);
template MANTID_ALGORITHMS_DLL size_t findXIndex(const HistogramData::HistogramX &, const double, const size_t);
template MANTID_ALGORITHMS_DLL size_t findXIndex(const std::vector<double> &, const double, const size_t);

} // namespace PeakParameterHelper
} // namespace Algorithms
} // namespace Mantid
