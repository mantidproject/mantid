//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AsymmetryHelper.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Progress.h"

#include "MantidAPI/Workspace_fwd.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <numeric>
#include <vector>

namespace {
/// Number of microseconds in one second (10^6)
constexpr double MICROSECONDS_PER_SECOND{1000000.0};
/// Muon lifetime in microseconds
constexpr double MUON_LIFETIME_MICROSECONDS{
    Mantid::PhysicalConstants::MuonLifetime * MICROSECONDS_PER_SECOND};
}

namespace Mantid {

using namespace Kernel;
using API::Progress;
using std::size_t;

/**
 * Corrects the data and errors for one spectrum.
 * The muon lifetime is in microseconds, not seconds, because the data is in
 * microseconds.
 * @param histogram :: [input] Input histogram
 * @param numGoodFrames :: [input] the number of good frames
 * @returns :: Histogram of the normalised counts
 */
HistogramData::Histogram
normaliseCounts(const HistogramData::Histogram &histogram,
                const double numGoodFrames) {
  HistogramData::Histogram result(histogram);
  auto &yData = result.mutableY();
  auto &eData = result.mutableE();
  for (size_t i = 0; i < yData.size(); ++i) {
    const double factor = exp(result.x()[i] / MUON_LIFETIME_MICROSECONDS);
    // Correct the Y data
    if (yData[i] != 0.0) {
      yData[i] *= factor / numGoodFrames;
    } else {
      yData[i] = 0.1 * factor / numGoodFrames;
    }

    // Correct the E data
    if (eData[i] != 0.0) {
      eData[i] *= factor / numGoodFrames;
    } else {
      eData[i] = factor / numGoodFrames;
    }
  }

  return result;
}

/**
* Estimates normalisation constant via
* N_0 = (Delta/f)*(sum_i W_i)/(int_a^b exp(-t/tau)dt )
* where W is the raw data, tau is the muon
* lifetime, t is time, f is the
* number of good frames Delta is the time step,
* a is the start of the range and b is the end of the range.
* @param histogram :: [input] Input histogram
* @param numGoodFrames :: [input] the number of good frames
* @param  StartX :: [input] the start time
* @param  EndX :: [input] the end time
* @returns :: The normalization constant N_0
*/
double estimateNormalisationConst(const HistogramData::Histogram &histogram,
                                  const double numGoodFrames,
                                  const double startX, const double endX) {

  auto xData = histogram.binEdges();
  auto &yData = histogram.y();

  size_t i0 = startIndexFromTime(xData, startX);
  size_t iN = endIndexFromTime(xData, endX);
  double summation = 0.0;
  // remove an extra index as XData is bin boundaries and not point data
  for (size_t i = i0; i < iN; i++) {
    summation += yData[i];
  }
  double Delta = xData[1] - xData[0];
  double denominator = MUON_LIFETIME_MICROSECONDS * numGoodFrames *
                       (exp(-startX / MUON_LIFETIME_MICROSECONDS) -
                        exp(-endX / MUON_LIFETIME_MICROSECONDS));
  return summation * Delta / denominator;
}
/**
* Finds the first index in bin edges that is after
* the start time.
* @param xData :: [input] Input HistogramData as bin edges
* @param startX :: [input] the start time
* @returns :: The index to start calculations from
*/
size_t startIndexFromTime(const HistogramData::BinEdges &xData,
                          const double startX) {
  size_t i;

  if (xData[0] > startX) {
    i = 0;
    return i;
  }
  for (i = 1; i < xData.size() - 1; i++) {
    if (xData[i] >= startX) {
      return i;
    }
  }
  throw std::runtime_error("The start point is equal to or greater than the "
                           "last data point. There is zero range.");
}
/**
* find the first index in bin edges that is after
* the endtime.
* @param xData :: [input] HistogramData as bin edges
* @param endX :: [input] the end time
* @returns :: The last index to  include in calculations
*/
size_t endIndexFromTime(const HistogramData::BinEdges &xData,
                        const double endX) {
  size_t i;
  if (xData[xData.size() - 1] < endX) {
    i = xData.size() - 1;
    return i;
  }
  for (i = xData.size() - 1; i > 1; i--) {
    if (xData[i] <= endX) {
      return i;
    }
  }
  throw std::runtime_error("The end point is less than or equal to the first "
                           "data point. There is zero range.");
}
} // namespace Mantid
