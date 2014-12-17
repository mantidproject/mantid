#include "MantidKernel/BinFinder.h"
#include "MantidKernel/Exception.h"

using std::size_t;

namespace Mantid {
namespace Kernel {

/** Constructor. Sets up the calculation for later.
 *
 * @param binParams: the binning parameters, as a vector of doubles. E.g.
 *    0, 1.0, 100, -0.5, 1e7:
 *    bins in step to 1.0 from 0 to 100; then log steps up to 1e7
 */
BinFinder::BinFinder(const std::vector<double> &binParams) {
  boundaries.clear();
  stepSizes.clear();

  size_t n = binParams.size();
  if (n < 3)
    throw std::invalid_argument("BinFinder: not enough bin parameters.");
  if (n % 2 == 0)
    throw std::invalid_argument(
        "BinFinder: the number of bin parameters should be odd.");

  for (size_t i = 0; i < n / 2; i++) {
    // The boundaries
    double min = binParams[i * 2];
    double max = binParams[i * 2 + 2];
    // Only the first bin needs the min boundary
    if (i == 0)
      boundaries.push_back(min);
    boundaries.push_back(max);
    // The step
    double step = binParams[i * 2 + 1];
    stepSizes.push_back(step);
    if (step == 0)
      throw std::invalid_argument("BinFinder: step size of 0.");
    if ((step < 0) && (min <= 0))
      throw std::invalid_argument(
          "BinFinder: logarithmic binning with 0.0 starting bin.");
    if (max <= min)
      throw std::invalid_argument(
          "BinFinder: final bin must be > starting bin boundary.");

    int numBins = 0;

    // Pre-do some calculations for log binning.
    if (step < 0) {
      double log_step = log(1.0 + fabs(step));
      logSteps.push_back(log_step);
      if (i == 0)
        logBoundaries.push_back(log(min));
      logBoundaries.push_back(log(max));
      // How many bins is that?
      numBins = static_cast<int>(ceil((log(max) - log(min)) / log_step));

      // Check that the last bin is at least .25 x the previous step
      // This is because the VectorHelper removes that final bin. Annoying!
      double nextToLastValue = min * pow(1.0 + fabs(step), numBins - 1);
      double nextToNextToLastValue = min * pow(1.0 + fabs(step), numBins - 2);
      double lastBinSize = max - nextToLastValue;
      double nextToLastBinSize = nextToLastValue - nextToNextToLastValue;
      if (lastBinSize < nextToLastBinSize * 0.25)
        numBins--;
      if (numBins < 1)
        numBins = 1;

    } else {
      // Empty log values; these won't be used
      logSteps.push_back(0);
      if (i == 0)
        logBoundaries.push_back(0);
      logBoundaries.push_back(0);
      //# of linear bins
      numBins = static_cast<int>(ceil((max - min) / step));

      // Check that the last bin is at least .25 x the previous step
      // This is because the VectorHelper removes that final bin. Annoying!
      double lastBinSize = max - ((numBins - 1) * step + min);
      if (lastBinSize < step * 0.25)
        numBins--;
      if (numBins < 1)
        numBins = 1;
    }

    // Find the end bin index
    int startBinIndex = 0;
    if (i > 0)
      startBinIndex = this->endBinIndex[i - 1];
    endBinIndex.push_back(numBins + startBinIndex);
  }
  // How many binning regions?
  numRegions = static_cast<int>(stepSizes.size());
}

/// Destructor
BinFinder::~BinFinder() {}

/** Returns the last bin boundary index,
 * which should be == to the size of the X axis.
 */
int BinFinder::lastBinIndex() {
  if (endBinIndex.size() > 0)
    return endBinIndex[endBinIndex.size() - 1];
  else
    return -1;
}

/** Find the bin index for a value.
 * @param x: x-value to histogram
 * @return an int corresponding to the bin index to use, or -1 if out of bounds.
 */
int BinFinder::bin(double x) {
  int index;
  double min;

  // Too small?
  if (x < boundaries[0])
    return -1;

  // Find which binning region to use
  int i = -1;
  for (i = 0; i < numRegions; i++) {
    min = boundaries[i];
    double max = boundaries[i + 1];
    if ((x >= min) && (x < max))
      break;
  }
  // Didn't find it?
  if (i >= numRegions)
    return -1;

  // Step size in this region
  double step = stepSizes[i];
  if (step > 0) {
    // Linear binning. Truncate when you divide by the step size
    index = static_cast<int>((x - min) / step);
    // Add bin index offset if not in the first region
    if (i > 0)
      index += endBinIndex[i - 1];
    // In the event that a final bin was skipped, cap to the max
    if (index >= endBinIndex[i])
      index = endBinIndex[i] - 1;
    return index;
  } else {
    /** Log binning formula for bin index n:
     *  x_n = min * ( 1 + |step| ) ^ n
     *  log(x_n) = log(min) + n * log(1+|step|)
     *  therefore
     *  n = (log(x_n) - log(min))/log(1+|step|)
     */

    double log_x = log(x); // Just one log to call per event!
    double log_step = logSteps[i];
    double log_min = logBoundaries[i];
    index = static_cast<int>((log_x - log_min) / log_step);
    // Add bin index offset if not in the first region
    if (i > 0)
      index += endBinIndex[i - 1];
    // In the event that a final bin was skipped, cap to the max
    if (index >= endBinIndex[i])
      index = endBinIndex[i] - 1;
    return index;
  }
}
}
} // Namespace
