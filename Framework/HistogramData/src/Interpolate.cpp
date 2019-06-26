// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidHistogramData/Interpolate.h"
#include "MantidHistogramData/Histogram.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#include <memory>
#include <sstream>

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramY;

namespace {

/// Enumeration for supported interpolation types.
enum class InterpolationType { LINEAR, CSPLINE };

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
 * Perform common sanity checks for interpolations
 * @param input A histogram from which to interpolate
 * @param output A histogram where interpolated values are store
 * @throw runtime_error Signals that the sanity check failed.
 */
void sanityCheck(const Histogram &input, const Histogram &output,
                 const size_t minInputSize) {
  const auto inPoints = input.points();
  const auto outPoints = output.points();
  if (inPoints.size() < minInputSize) {
    throw std::runtime_error(
        "interpolate - input histogram has too few points");
  }
  if (outPoints.front() < inPoints.front() ||
      outPoints.back() > inPoints.back()) {
    throw std::runtime_error("interpolate - input does not cover all points in "
                             "output. Extrapolation not suppoted.");
  }
  if (!std::is_sorted(inPoints.cbegin(), inPoints.cend())) {
    throw std::runtime_error(
        "interpolate - input X data must be sorted in ascending order.");
  }
}

/**
 * Perform interpolation in place
 * @param xs A container of x values
 * @param ys A container of y values
 * @param points A container of points at which to interpolate
 * @param outY Output for interpolated values
 * @param type Type of interpolation
 */
template <typename XData, typename YData, typename XInterp, typename YInterp>
void interpolateInplace(const XData &xs, const YData &ys, const XInterp &points,
                        YInterp &outY, const InterpolationType type) {
  const gsl_interp_type *interpType = nullptr;
  switch (type) {
  case InterpolationType::LINEAR:
    interpType = gsl_interp_linear;
    break;
  case InterpolationType::CSPLINE:
    interpType = gsl_interp_cspline;
    break;
  }
  using gsl_interp_uptr = std::unique_ptr<gsl_interp, void (*)(gsl_interp *)>;
  auto interp =
      gsl_interp_uptr(gsl_interp_alloc(interpType, xs.size()), gsl_interp_free);
  gsl_interp_init(interp.get(), xs.data(), ys.data(), xs.size());
  using gsl_interp_accel_uptr =
      std::unique_ptr<gsl_interp_accel, void (*)(gsl_interp_accel *)>;
  auto lookupTable =
      gsl_interp_accel_uptr(gsl_interp_accel_alloc(), gsl_interp_accel_free);
  // Evaluate each point for the full range
  for (size_t i = 0; i < outY.size(); ++i) {
    outY[i] = gsl_interp_eval(interp.get(), xs.data(), ys.data(), points[i],
                              lookupTable.get());
  }
}

/**
 * Perform linear interpolation assuming the input data set to be a Histogram
 * with XMode=Points. It is assumed all sanity checks have been performed
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
  size_t step(stepSize);
  double x1(0.), x2(0.), y1(0.), y2(0.), overgap(0.);
  // Copy over end value skipped by loop
  ynew.back() = yold.back();
  for (size_t i = 0; i < nypts - 1; ++i) // Last point has been calculated
  {
    const double xp = xold[i];
    if (step == stepSize) {
      x1 = xp;
      const auto index2 =
          ((i + stepSize) >= nypts ? nypts - 1 : (i + stepSize));
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

  interpolateInplace(xc, yc, xold, ynew, InterpolationType::CSPLINE);
}

} // end anonymous namespace

namespace Mantid {
namespace HistogramData {

/**
 * Return the minimum size of input points for cpline interpolation.
 * @return the minimum number of points
 */
size_t minSizeForCSplineInterpolation() {
  return gsl_interp_type_min_size(gsl_interp_cspline);
}

/**
 * Return the minimum size of input points for linear interpolation.
 * @return the minimum number of points
 */
size_t minSizeForLinearInterpolation() {
  return gsl_interp_type_min_size(gsl_interp_linear);
}

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
  sanityCheck(input, stepSize, minSizeForLinearInterpolation(), LINEAR_NAME);

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
  sanityCheck(inOut, stepSize, minSizeForLinearInterpolation(), LINEAR_NAME);
  interpolateYLinearInplace(inOut, stepSize, inOut.mutableY());
}

/**
 * Interpolate from input histogram to the output
 * @param input A histogram from which to interpolate
 * @param output A histogram containing the interpolated values
 */
void interpolateLinearInplace(const Histogram &input, Histogram &output) {
  sanityCheck(input, output, minSizeForLinearInterpolation());
  const auto inputPoints = input.points();
  const auto &points = inputPoints.rawData();
  const auto &y = input.y().rawData();
  const auto &interpPoints = output.points();
  auto &newY = output.mutableY();
  interpolateInplace(points, y, interpPoints, newY, InterpolationType::LINEAR);
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
  sanityCheck(input, stepSize, minSizeForCSplineInterpolation(), CSPLINE_NAME);

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
  sanityCheck(inOut, stepSize, minSizeForCSplineInterpolation(), CSPLINE_NAME);
  interpolateYCSplineInplace(inOut, stepSize, inOut.mutableY());
}

/**
 * Performs cubic spline interpolation from input to output
 * @param input A histogram from which to interpolate
 * @param output A histogram where to store the interpolated values
 */
void interpolateCSplineInplace(const Histogram &input, Histogram &output) {
  sanityCheck(input, output, minSizeForCSplineInterpolation());
  const auto &points = input.points().rawData();
  const auto &y = input.y().rawData();
  const auto &interpPoints = output.points();
  auto &newY = output.mutableY();
  interpolateInplace(points, y, interpPoints, newY, InterpolationType::CSPLINE);
}

} // namespace HistogramData
} // namespace Mantid
