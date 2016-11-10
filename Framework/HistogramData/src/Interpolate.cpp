#include "MantidHistogramData/Interpolate.h"
#include "MantidHistogramData/Histogram.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#include <memory>
#include <sstream>

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::HistogramY;

namespace {

constexpr const char *LINEAR_NAME = "Linear";
constexpr const char *CSPLINE_NAME = "CSpline";

/**
 * Compute the number of pre-calculated points given the ysize and step size
 * @param ysize The total number of points in the data set
 * @param stepSize The step size between each calculated point
 * @return The number of calculated nodes
 */
constexpr size_t numberCalculated(const size_t ysize, const size_t stepSize) {
  // First and last points are always assumed to be calculated
  return 1 + ((ysize - 1) / stepSize);
}

/**
 * Perform common sanity checks for interpolations
 * @param input See interpolateLinear
 * @param stepSize See interpolateLinear
 * @param minCalculated The minimum number of calculated values required
 * by the routine
 * @param method A string providing the name of the interpolation method. Used
 * in error messages
 * @throws std::runtime_error if input.yMode() == Uninitialized or
 * stepSize is invalid or the number of calculated points is less than the
 * the required value
 */
void sanityCheck(const Histogram &input, const size_t stepSize,
                 const size_t minCalculated, const char *method) {
  if (input.yMode() == Histogram::YMode::Uninitialized) {
    throw std::runtime_error(
        "interpolate - YMode must be defined for input histogram.");
  }
  const auto ysize = input.y().size();
  if (stepSize >= ysize) {
    throw std::runtime_error("interpolate - Step size must be smaller "
                             "than the number of points");
  }
  // First and last points are always assumed to be calculated
  const size_t ncalc = numberCalculated(ysize, stepSize);
  if (ncalc < minCalculated) {
    std::ostringstream os;
    os << "interpolate - " << method << " requires " << minCalculated
       << " points but only " << ncalc << " were found.";
    throw std::runtime_error(os.str());
  }
}

/**
 * Perform linear interpolation assuming the input data set to be a Histogram
 * with XMode=Points. It is assumed all sanity cheks have been performed
 * by the interpolateLinear entry point.
 * @param input See interpolateLinear
 * @param stepSize See interpolateLinear
 * @param ynew A reference to the output Y values
 */
void interpolateYLinearInplace(const Histogram &input, const size_t stepSize,
                               HistogramY &ynew) {
  const auto xold = input.points();
  const auto &yold = input.y();
  const auto nypts = yold.size();
  size_t step(stepSize), index2(0);
  double x1(0.), x2(0.), y1(0.), y2(0.), overgap(0.);
  // Copy over end value skipped by loop
  ynew.back() = yold.back();
  for (size_t i = 0; i < nypts - 1; ++i) // Last point has been calculated
  {
    const double xp = xold[i];
    if (step == stepSize) {
      x1 = xp;
      index2 = ((i + stepSize) >= nypts ? nypts - 1 : (i + stepSize));
      x2 = xold[index2];
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

/**
 * Perform cubic spline interpolation. It is assumed all sanity checks have been
 * performed by the interpolateCSpline entry point.
 * @param input See interpolateCSpline
 * @param stepSize See interpolateCSpline
 * @param ynew A reference to the output Y values
 */
void interpolateYCSplineInplace(const Histogram &input, const size_t stepSize,
                                HistogramY &ynew) {
  const auto xold = input.points();
  const auto &yold = input.y();
  const auto nypts = yold.size();

  const auto ncalc = numberCalculated(nypts, stepSize);
  std::vector<double> xc(ncalc), yc(ncalc);
  for (size_t step = 0, i = 0; step < nypts; step += stepSize, ++i) {
    xc[i] = xold[step];
    yc[i] = yold[step];
  }
  // Ensure we have the last value
  xc.back() = xold.back();
  yc.back() = yold.back();

  // Manage gsl memory with unique_ptrs
  using gsl_spline_uptr = std::unique_ptr<gsl_spline, void (*)(gsl_spline *)>;
  auto spline = gsl_spline_uptr(gsl_spline_alloc(gsl_interp_cspline, ncalc),
                                gsl_spline_free);
  // Compute spline
  gsl_spline_init(spline.get(), xc.data(), yc.data(), ncalc);
  // Evaluate each point for the full range
  using gsl_interp_accel_uptr =
      std::unique_ptr<gsl_interp_accel, void (*)(gsl_interp_accel *)>;
  auto lookupTable =
      gsl_interp_accel_uptr(gsl_interp_accel_alloc(), gsl_interp_accel_free);
  for (size_t i = 0; i < nypts; ++i) {
    ynew[i] = gsl_spline_eval(spline.get(), xold[i], lookupTable.get());
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
 * interpolation. The XMode of the output will match the input histogram.
 */
Histogram interpolateLinear(const Histogram &input, const size_t stepSize) {
  sanityCheck(input, stepSize, 2, LINEAR_NAME);

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
  sanityCheck(inOut, stepSize, 2, LINEAR_NAME);
  interpolateYLinearInplace(inOut, stepSize, inOut.mutableY());
}

/**
 * Interpolate through the y values of a histogram using a cubic spline,
 * assuming that the calculated "nodes" are stepSize apart.
 * Currently errors are ignored.
 * @param input Input histogram defining x values and containing calculated
 * Y values at stepSize intervals. It is assumed that the first/last points
 * are always calculated points.
 * @param stepSize The space, in indices, between the calculated points
 * @return A new Histogram with the y-values from the result of a linear
 * interpolation. The XMode of the output will match the input histogram.
 */
Histogram interpolateCSpline(const Histogram &input, const size_t stepSize) {
  sanityCheck(input, stepSize, 4, CSPLINE_NAME);

  HistogramY ynew(input.y().size());
  interpolateYCSplineInplace(input, stepSize, ynew);
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
 * Cubic spline interpolate across a set of data. (In-place version). See
 * interpolateCSpline.
 * @param inOut Input histogram whose points are interpolated in place
 * @param stepSize See interpolateCSpline
 */
void interpolateCSplineInplace(Histogram &inOut, const size_t stepSize) {
  sanityCheck(inOut, stepSize, 4, CSPLINE_NAME);
  interpolateYCSplineInplace(inOut, stepSize, inOut.mutableY());
}

} // namespace HistogramData
} // namespace Mantid
