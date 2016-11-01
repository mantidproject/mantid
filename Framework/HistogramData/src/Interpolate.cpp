#include "MantidHistogramData/Interpolate.h"

#include "MantidHistogramData/Histogram.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;

namespace {

/**
 * Perform common sanity checks for interpolations
 * @param input See interpolateLinear
 * @param stepSize See interpolateLinear
 */
void sanityCheck(const Histogram &input, const size_t stepSize) {
  if (input.yMode() == Histogram::YMode::Uninitialized) {
    throw std::runtime_error(
        "interpolateLinear() - YMode must be defined for input histogram.");
  }
  if (input.y().size() < 3) {
    throw std::runtime_error("interpolateLinear() - At least 3 values are "
                             "required for interpolation");
  }
  if (stepSize >= input.y().size()) {
    throw std::runtime_error("interpolateLinear() - Step size must be smaller "
                             "than the number of points");
  }
}

/**
 * Perform linear interpolation assuming the input data set to be a Histogram
 * with XMode=Points. It is assumed all sanity cheks have been performed
 * by the interpolateLinear entry point.
 * @param input See interpolateLinear
 * @param stepSize See interpolateLinear
 * @param ynew A reference to the output Y values
 * @return See interpolateLinear
 */
void interpolateYLinearInplace(const Histogram &input, const size_t stepSize,
                               HistogramY &ynew) {
  auto binCentre = input.xMode() == Histogram::XMode::BinEdges
                       ? [](const HistogramX &x,
                            size_t i) { return 0.5 * (x[i] + x[i + 1]); }
                       : [](const HistogramX &x, size_t i) { return x[i]; };

  auto &xold = input.x();
  auto &yold = input.y();
  auto nypts = yold.size();
  size_t step(stepSize), index2(0);
  double x1(0.), x2(0.), y1(0.), y2(0.), overgap(0.);
  // Copy over end value skipped by loop
  ynew.back() = yold.back();
  for (size_t i = 0; i < nypts - 1; ++i) // Last point has been calculated
  {
    double xp = binCentre(xold, i);
    if (step == stepSize) {
      x1 = xp;
      index2 = ((i + stepSize) >= nypts ? nypts - 1 : (i + stepSize));
      x2 = binCentre(xold, index2);
      overgap = 1.0 / (x2 - x1);
      y1 = yold[i];
      y2 = yold[index2];
      step = 1;
      ynew[i] = yold[i];
      continue;
    }
    // Linear interpolation
    ynew[i] = (xp - x1) * y2 + (x2 - xp) * y1;
    ynew[i] *= overgap;
    step++;
  }
}

} // end anonymous namespace

namespace Mantid {
namespace HistogramData {

/**
 * Linearly interpolate through the y values of a histogram assuming that the
 * calculated "nodes" are stepSize apart. Currently errors are not treated.
 * @param input Input histogram defining x values and containing calculated
 * Y values at stepSize intervals. It is assumed that the first/last points
 * are always calculated points.
 * @param stepSize The space, in indices, between the calculated points
 * @return A new Histogram with the y-values from the result of a linear
 * interpolation. If the XMode of the output will match that of the input
 */
Histogram interpolateLinear(const Histogram &input, const size_t stepSize) {
  sanityCheck(input, stepSize);

  HistogramY ynew(input.y().size());
  interpolateYLinearInplace(input, stepSize, ynew);
  // Cheap copy
  Histogram output(input);
  if (output.yMode() == Histogram::YMode::Counts) {
    output.setCounts(ynew);
  } else {
    output.setFrequencies(ynew);
  }
  return output;
}

/**
 * Linearly interpolate across a set of data. (In-place version). See
 * interpolateLinear.
 * @param inOut Input histogram whose points are interpolated in place
 * @param stepSize See interpolateLinear
 */
void interpolateLinearInplace(Histogram &inOut, const size_t stepSize) {
  sanityCheck(inOut, stepSize);
  interpolateYLinearInplace(inOut, stepSize, inOut.mutableY());
}

} // namespace HistogramData
} // namespace Mantid
