// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidHistogramData/Interpolate.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"

#include <memory>
#include <sstream>

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramY;

namespace {

/// Enumeration for supported interpolation types.
enum class InterpolationType { LINEAR, CSPLINE };

constexpr const char *LINEAR_NAME = "Linear";
constexpr const char *CSPLINE_NAME = "CSpline";

/// static logger
Mantid::Kernel::Logger g_log("Interpolate");

/**
 * Compute the number of pre-calculated points given the ysize and step size
 * @param ysize The total number of points in the data set
 * @param stepSize The step size between each calculated point
 * @return The number of calculated nodes
 */
constexpr size_t numberCalculated(const size_t ysize, const size_t stepSize) {
  // First and last points are always assumed to be calculated
  auto nCalc = 1 + (ysize - 1) / stepSize;
  if ((ysize - 1) % stepSize != 0)
    nCalc++;
  return nCalc;
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
void sanityCheck(const Histogram &input, const size_t stepSize, const size_t minCalculated, const char *method) {
  if (input.yMode() == Histogram::YMode::Uninitialized) {
    throw std::runtime_error("interpolate - YMode must be defined for input histogram.");
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
    os << "interpolate - " << method << " requires " << minCalculated << " calculated points but only " << ncalc
       << " were found.";
    throw std::runtime_error(os.str());
  }
  // need at least one non-calculated point
  if (ysize < minCalculated + 1) {
    std::ostringstream os;
    os << "interpolate - " << method << " requires " << minCalculated + 1 << " points but only " << ncalc
       << " were found.";
    throw std::runtime_error(os.str());
  }
}

/**
 * Perform common sanity checks for interpolations
 * @param input A histogram from which to interpolate
 * @param output A histogram where interpolated values are store
 * @throw runtime_error Signals that the sanity check failed.
 */
void sanityCheck(const Histogram &input, const Histogram &output, const size_t minInputSize) {
  const auto inPoints = input.points();
  const auto outPoints = output.points();
  if (inPoints.size() < minInputSize) {
    throw std::runtime_error("interpolate - input histogram has too few points");
  }
  if (outPoints.front() < inPoints.front() || outPoints.back() > inPoints.back()) {
    throw std::runtime_error("interpolate - input does not cover all points in "
                             "output. Extrapolation not suppoted.");
  }
  if (!std::is_sorted(inPoints.cbegin(), inPoints.cend())) {
    throw std::runtime_error("interpolate - input X data must be sorted in ascending order.");
  }
}

/**
 * Perform cubic spline interpolation in place
 * @param input A histogram containing the input x, y, e values
 * @param points A container of points at which to interpolate
 * @param output A histogram containing the original and interpolated values
 */
void interpolateYCSplineInplace(const Mantid::HistogramData::Histogram &input,
                                const Mantid::HistogramData::Points &points, Mantid::HistogramData::Histogram &output,
                                const bool calculateErrors = false, const bool independentErrors = true) {
  auto xs = input.dataX();
  // Interpolation and Error propagation follows method described in Gardner paper
  // "Uncertainties in Interpolated Spectral Data", Journal of Research of the
  // National Institute of Standards and Technology, 2003
  // create tridiagonal "h" matrix
  Mantid::Kernel::Matrix<double> h(xs.size() - 2, xs.size() - 2);
  for (size_t i = 0; i < xs.size() - 2; i++) {
    for (size_t j = 0; j < xs.size() - 2; j++) {
      if (i == j) {
        h[i][j] = (xs[i + 2] - xs[i]) / 3;
      }
      if (i == j + 1) {
        h[i][j] = (xs[i] - xs[j]) / 6;
      }
      if (j == i + 1) {
        h[i][j] = (xs[i + 2] - xs[i + 1]) / 6;
      }
    }
  }
  double xsMaxEpsilon = *(std::max_element(xs.begin(), xs.end())) * std::numeric_limits<double>::epsilon();
  // elements with i=j will have the largest value
  double hMaxEpsilon = xsMaxEpsilon * 2 / 3;

  std::vector<double> d(xs.size() - 2);
  auto ys = input.dataY();
  for (size_t i = 0; i < xs.size() - 2; i++) {
    d[i] = (ys[i + 2] - ys[i + 1]) / (xs[i + 2] - xs[i + 1]) - (ys[i + 1] - ys[i]) / (xs[i + 1] - xs[i]);
  }

  // ypp means y prime prime
  std::vector<double> ypp(xs.size() - 2);
  // would be quicker to solve linear equation rather than invert h but also
  // need h-1 elements later on
  h.invertTridiagonal(2 * hMaxEpsilon);
  ypp = h * d;

  // add in the zero second derivatives at extreme pts to give natural splines
  std::vector<double> ypp_full(xs.size(), 0);
  std::copy(ypp.begin(), ypp.end(), ypp_full.begin() + 1);

  // calculate some covariances to support error propagation
  auto &enew = output.mutableE();
  const auto &eold = input.dataE();
  // u_ypp_ypp - covariance of y'' vs y''
  std::vector<double> u_ypp_ypp(xs.size());
  // u_ypp_y - covariance of y'' vs y
  std::vector<double> u_ypp_y(xs.size());

  for (size_t i = 0; i < xs.size(); i++) {
    for (size_t k = 0; k < xs.size(); k++) {
      // dyppidyk - derivative of y'' at bin i with respect to y at bin k
      double dyppidyk = 0;
      if ((i != 0) && (i != xs.size() - 1)) {
        if (k > 1) {
          dyppidyk += h[i - 1][k - 2] / (xs[k] - xs[k - 1]);
        }
        if ((k > 0) && (k < xs.size() - 1)) {
          dyppidyk += h[i - 1][k - 1] * (1 / (xs[k + 1] - xs[k]) + 1 / (xs[k] - xs[k - 1]));
        }
        if (k < xs.size() - 2) {
          dyppidyk += h[i - 1][k] / (xs[k + 1] - xs[k]);
        }
      }
      u_ypp_ypp[i] += dyppidyk * dyppidyk * pow(eold[k], 2);
      if (k == i) {
        u_ypp_y[i] = dyppidyk * pow(eold[k], 2);
      }
    }
  }

  // Plug the calculated second derivatives into the formula for each cubic polynomial:
  // y = A*y_i + B*y_i+1 + C*ypp_i + D*ypp_i+1
  // Formula is from Gardner paper which references it from Numerical Recipes in C
  // It is derived from Taylor expansion about x_i with term in yp_i expressed in terms of
  // y_i, y_i+1, ypp_i, ypp_i+1
  auto &ynew = output.mutableY();
  for (size_t i = 0; i < points.size(); i++) {
    auto it = std::upper_bound(xs.begin(), xs.end(), points[i]);
    if (it == xs.end()) {
      it = std::prev(xs.end());
    }
    auto index = std::distance(xs.begin(), it);
    auto x2 = xs[index];
    auto x1 = xs[index - 1];
    auto y2 = ys[index];
    auto y1 = ys[index - 1];
    auto e2 = eold[index];
    auto e1 = eold[index - 1];
    auto ypp2 = ypp_full[index];
    auto ypp1 = ypp_full[index - 1];
    auto u_y2pp_y2 = u_ypp_y[index];
    auto u_y1pp_y1 = u_ypp_y[index - 1];
    auto u_y2pp_y2pp = u_ypp_ypp[index];
    auto u_y1pp_y1pp = u_ypp_ypp[index - 1];

    double A = (x2 - points[i]) / (x2 - x1);
    double B = (points[i] - x1) / (x2 - x1);
    double C = (pow(A, 3) - A) * (pow(x2 - x1, 2)) / 6;
    double D = (pow(B, 3) - B) * (pow(x2 - x1, 2)) / 6;

    ynew[i] = A * y1 + B * y2 + C * ypp1 + D * ypp2;

    // propagate the source points errors through to the interpolated point
    // Interpolation error is hard to calculate and is probably v small so
    // assume it's zero
    if (calculateErrors) {
      if (independentErrors) {
        auto var = A * A * e1 * e1 + 2 * A * C * u_y1pp_y1 + B * B * e2 * e2 + 2 * B * D * u_y2pp_y2 +
                   C * C * u_y1pp_y1pp + D * D * u_y2pp_y2pp;
        enew[i] = sqrt(var);
      } else {
        // if the errors are correlated just do linear interpolation on them
        // to get something approximately equal to the two calculated errors
        // Not sure there's much point doing a spline interpolation on the
        // errors
        enew[i] = (points[i] - x1) * e2 + (x2 - points[i]) * e1;
      }
    } else {
      if (points[i] == x1) {
        enew[i] = e1;
      }
    }
  }
}

/**
 * Perform cubic spline interpolation in place
 * @param input A histogram containing the input x, y, e values
 * @param points A container of points at which to interpolate
 * @param output A histogram containing the original and interpolated values
 * @param calculateErrors Boolean to control whether errors are calculated
 * @param independentErrors Boolean to control whether errors on original points
 * are considered to be correlated or independent
 */
void interpolateYLinearInplace(const Mantid::HistogramData::Histogram &input,
                               const Mantid::HistogramData::Points &points, Mantid::HistogramData::Histogram &output,
                               const bool calculateErrors = false, const bool independentErrors = true) {
  const auto xold = input.points();
  const auto &yold = input.y();
  const auto &eold = input.e();
  const auto nypts = points.size();

  auto &ynew = output.mutableY();
  auto &enew = output.mutableE();

  bool calculateInterpolationErrors = true;
  std::vector<double> secondDeriv(input.size() - 1);
  for (size_t i = 0; i < input.size() - 1; i++) {
    if (calculateErrors) {
      if (xold.size() < 3) {
        g_log.warning("Number of x points too small to calculate interpolation errors");
        calculateInterpolationErrors = false;
      } else {

        auto x0_secondDeriv = i < 1 ? 0 : i - 1;
        auto x1_secondDeriv = x0_secondDeriv + 1 >= xold.size() ? xold.size() - 1 : x0_secondDeriv + 1;
        auto x2_secondDeriv = x1_secondDeriv + 1;

        auto firstDeriv01 =
            (yold[x1_secondDeriv] - yold[x0_secondDeriv]) / (xold[x1_secondDeriv] - xold[x0_secondDeriv]);
        auto firstDeriv12 =
            (yold[x2_secondDeriv] - yold[x1_secondDeriv]) / (xold[x2_secondDeriv] - xold[x1_secondDeriv]);
        secondDeriv[i] = (firstDeriv12 - firstDeriv01) / ((xold[x2_secondDeriv] - xold[x0_secondDeriv]) / 2);
      }
    }
  }
  for (size_t i = 0; i < nypts; ++i) {
    auto it = std::upper_bound(xold.begin(), xold.end(), points[i]);
    if (it == xold.end()) {
      it = std::prev(xold.end());
    }
    auto index = std::distance(xold.begin(), it);
    auto x2 = xold[index];
    auto x1 = xold[index - 1];
    auto overgap = 1.0 / (x2 - x1);
    auto y2 = yold[index];
    auto y1 = yold[index - 1];
    auto e2 = eold[index];
    auto e1 = eold[index - 1];

    const double xp = points[i];

    // Linear interpolation
    ynew[i] = (xp - x1) * y2 + (x2 - xp) * y1;
    ynew[i] *= overgap;

    if (calculateErrors) {
      // propagate errors from original points
      double sourcePointsError;
      if (independentErrors) {
        sourcePointsError = sqrt(pow((xp - x1) * e2, 2) + pow(((x2 - xp)) * e1, 2));
        sourcePointsError *= overgap;
      } else {
        // if the errors on the original points are correlated then just
        // do a linear interpolation on them
        sourcePointsError = (xp - x1) * e2 + (x2 - xp) * e1;
      }
      // calculate interpolation error
      auto interpError =
          calculateInterpolationErrors ? 0.5 * (xp - x1) * (x2 - xp) * std::abs(secondDeriv[index - 1]) : 0;
      // combine the two errors
      enew[i] = sqrt(pow(sourcePointsError, 2) + pow(interpError, 2));
    } else {
      if (xp == x1) {
        enew[i] = e1;
      }
    }
  }
}

/**
 * Return a histogram with all the zero points removed from input
 * @param input Histogram containing some points with a zero y value
 * @param stepSize distance between the points that should be kept
 * @return A histogram containing only the required points
 */
Histogram compactInputsAndCallInterpolate(const Histogram &input, const size_t stepSize) {
  const auto xold = input.points();
  const auto &yold = input.y();
  const auto &eold = input.e();
  const auto nypts = yold.size();

  const auto ncalc = numberCalculated(nypts, stepSize);
  std::vector<double> xc(ncalc), yc(ncalc), ec(ncalc);
  for (size_t step = 0, i = 0; step < nypts; step += stepSize, ++i) {
    xc[i] = xold[step];
    yc[i] = yold[step];
    ec[i] = eold[step];
  }
  // Ensure we have the last value
  xc.back() = xold.back();
  yc.back() = yold.back();
  ec.back() = eold.back();

  const Histogram calcValues{Mantid::HistogramData::Points(xc), Mantid::HistogramData::Counts(yc),
                             Mantid::HistogramData::CountStandardDeviations(ec)};

  return calcValues;
}

} // end anonymous namespace

namespace Mantid::HistogramData {

/**
 * Return the minimum size of input points for cpline interpolation.
 * @return the minimum number of points
 */
size_t minSizeForCSplineInterpolation() { return 3; }

/**
 * Return the minimum size of input points for linear interpolation.
 * @return the minimum number of points
 */
size_t minSizeForLinearInterpolation() { return 2; }

/**
 * Linearly interpolate through the y values of a histogram assuming that the
 * calculated "nodes" are stepSize apart.
 * @param input Input histogram defining x values and containing calculated
 * Y values at stepSize intervals. It is assumed that the first/last points
 * are always calculated points.
 * @param stepSize The space, in indices, between the calculated points
 * @return A new Histogram with the y-values from the result of a linear
 * interpolation. The XMode of the output will match the input histogram.
 */
Histogram interpolateLinear(const Histogram &input, const size_t stepSize, const bool calculateErrors,
                            const bool independentErrors) {
  sanityCheck(input, stepSize, minSizeForLinearInterpolation(), LINEAR_NAME);

  // Cheap copy
  Histogram output(input);
  auto calcValues = compactInputsAndCallInterpolate(input, stepSize);
  interpolateLinearInplace(calcValues, output, calculateErrors, independentErrors);

  return output;
}

/**
 * Linearly interpolate across a set of data. (In-place version). See
 * interpolateLinear.
 * @param inOut Input histogram whose points are interpolated in place
 * @param stepSize See interpolateLinear
 */
void interpolateLinearInplace(Histogram &inOut, const size_t stepSize, const bool calculateErrors,
                              const bool independentErrors) {
  sanityCheck(inOut, stepSize, minSizeForLinearInterpolation(), LINEAR_NAME);
  auto calcValues = compactInputsAndCallInterpolate(inOut, stepSize);
  interpolateLinearInplace(calcValues, inOut, calculateErrors, independentErrors);
}

/**
 * Interpolate from input histogram to the output
 * @param input A histogram from which to interpolate
 * @param output A histogram containing the interpolated values
 */
void interpolateLinearInplace(const Histogram &input, Histogram &output, const bool calculateErrors,
                              const bool independentErrors) {
  sanityCheck(input, output, minSizeForLinearInterpolation());
  const auto &interpPoints = output.points();

  interpolateYLinearInplace(input, interpPoints, output, calculateErrors, independentErrors);
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
Histogram interpolateCSpline(const Histogram &input, const size_t stepSize, const bool calculateErrors,
                             const bool independentErrors) {
  sanityCheck(input, stepSize, minSizeForCSplineInterpolation(), CSPLINE_NAME);

  Histogram output(input);
  auto calcValues = compactInputsAndCallInterpolate(input, stepSize);
  interpolateCSplineInplace(calcValues, output, calculateErrors, independentErrors);

  return output;
}

/**
 * Cubic spline interpolate across a set of data. (In-place version). See
 * interpolateCSpline.
 * @param inOut Input histogram whose points are interpolated in place
 * @param stepSize See interpolateCSpline
 */
void interpolateCSplineInplace(Histogram &inOut, const size_t stepSize, const bool calculateErrors,
                               const bool independentErrors) {
  sanityCheck(inOut, stepSize, minSizeForCSplineInterpolation(), CSPLINE_NAME);
  auto calcValues = compactInputsAndCallInterpolate(inOut, stepSize);
  interpolateCSplineInplace(calcValues, inOut, calculateErrors, independentErrors);
}

/**
 * Performs cubic spline interpolation from input to output
 * @param input A histogram from which to interpolate
 * @param output A histogram where to store the interpolated values
 */
void interpolateCSplineInplace(const Histogram &input, Histogram &output, const bool calculateErrors,
                               const bool independentErrors) {
  sanityCheck(input, output, minSizeForCSplineInterpolation());
  const auto &interpPoints = output.points();
  interpolateYCSplineInplace(input, interpPoints, output, calculateErrors, independentErrors);
}

} // namespace Mantid::HistogramData
