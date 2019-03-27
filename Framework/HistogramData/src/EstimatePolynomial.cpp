// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidHistogramData/EstimatePolynomial.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/Points.h"
#include <limits>
#include <sstream>
#include <stdexcept>

using Mantid::HistogramData::HistogramY;
using Mantid::HistogramData::Points;

namespace {                    // anonymous
const double BAD_CHISQ(1.e10); // big invalid value
const double INVALID_CHISQ(std::numeric_limits<double>::quiet_NaN());

inline void calcFlatParameters(const double sum, const double sumY,
                               double &bg0) {
  if (sum != 0.)
    bg0 = sumY / sum;
  // otherwise outputs are already 0.
}

// use Cramer's rule for 2 x 2 matrix
inline void calcLinearParameters(const double sum, const double sumX,
                                 const double sumY, const double sumXY,
                                 const double sumX2, double &bg0, double &bg1) {
  const double determinant = sum * sumX2 - sumX * sumX;
  if (determinant != 0) {
    bg0 = (sumY * sumX2 - sumX * sumXY) / determinant;
    bg1 = (sum * sumXY - sumY * sumX) / determinant;
  } // otherwise outputs are already 0.
}

// use Cramer's rule for 3 x 3 matrix
// | a b c |
// | d e f |
// | g h i |
// 3 x 3 determinate:  aei+bfg+cdh-ceg-bdi-afh
inline void calcQuadraticParameters(const double sum, const double sumX,
                                    const double sumY, const double sumXY,
                                    const double sumX2, const double sumX2Y,
                                    const double sumX3, const double sumX4,
                                    double &bg0, double &bg1, double &bg2) {
  double determinant = sum * sumX2 * sumX4 + sumX * sumX3 * sumX2 +
                       sumX2 * sumX * sumX3 - sumX2 * sumX2 * sumX2 -
                       sumX * sumX * sumX4 - sum * sumX3 * sumX3;
  if (determinant != 0) {
    bg0 =
        (sumY * sumX2 * sumX4 + sumX * sumX3 * sumX2Y + sumX2 * sumXY * sumX3 -
         sumX2 * sumX2 * sumX2Y - sumX * sumXY * sumX4 - sumY * sumX3 * sumX3) /
        determinant;
    bg1 = (sum * sumXY * sumX4 + sumY * sumX3 * sumX2 + sumX2 * sumX * sumX2Y -
           sumX2 * sumXY * sumX2 - sumY * sumX * sumX4 - sum * sumX3 * sumX2Y) /
          determinant;
    bg2 = (sum * sumX2 * sumX2Y + sumX * sumXY * sumX2 + sumY * sumX * sumX3 -
           sumY * sumX2 * sumX2 - sumX * sumX * sumX2Y - sum * sumXY * sumX3) /
          determinant;
  } // otherwise outputs are already 0.
}

// y = bg0
struct constant {
  explicit constant(const double bg0) : bg0(bg0) {}

  double operator()(const double x, const double y) const {
    (void)x;
    const double temp = bg0 - y;
    return temp * temp;
  }

  double bg0;
};

// y = bg0 + bg1*x
struct linear {
  explicit linear(const double bg0, const double bg1) : bg0(bg0), bg1(bg1) {}

  double operator()(const double x, const double y) const {
    const double temp = bg0 + bg1 * x - y;
    return temp * temp;
  }

  double bg0;
  double bg1;
};

// y = bg0 + bg1*x + bg2*x**2
struct quadratic {
  explicit quadratic(const double bg0, const double bg1, const double bg2)
      : bg0(bg0), bg1(bg1), bg2(bg2) {}

  double operator()(const double x, const double y) const {
    const double temp = bg0 + bg1 * x + bg2 * x * x - y;
    return temp * temp;
  }

  double bg0;
  double bg1;
  double bg2;
};
} // anonymous namespace

namespace Mantid {
namespace HistogramData {

void estimate(const size_t order, const Points &X, const HistogramY &Y,
              const size_t i_min, const size_t i_max, const size_t p_min,
              const size_t p_max, bool haveGap, double &out_bg0,
              double &out_bg1, double &out_bg2, double &out_chisq_red) {
  // Validate input
  if (order > 2)
    throw std::runtime_error("can only estimate up to order=2");
  if (i_min >= i_max) {
    std::stringstream err;
    err << "i_min (" << i_min << ")cannot be larger or equal to i_max ("
        << i_max << ")";
    throw std::runtime_error(err.str());
  }
  if (i_max > X.size()) {
    std::stringstream err;
    err << "i_max  (" << i_max << ") cannot be larger or equal to size of X "
        << X.size() << ")";
    throw std::runtime_error(err.str());
  }
  if (haveGap && p_min >= p_max)
    throw std::runtime_error("p_min cannot larger or equal to p_max");
  // ignore when p-range is outside of i-range

  // set all output parameters to zero
  out_bg0 = 0.;
  out_bg1 = 0.;
  out_bg2 = 0.;
  out_chisq_red = INVALID_CHISQ;

  // accumulate sum
  double sum = 0.0;
  double sumX = 0.0;
  double sumY = 0.0;
  double sumX2 = 0.0;
  double sumXY = 0.0;
  double sumX2Y = 0.0;
  double sumX3 = 0.0;
  double sumX4 = 0.0;
  for (size_t i = i_min; i < i_max; ++i) {
    if (haveGap && i >= p_min && i < p_max)
      continue;
    sum += 1.0;
    sumX += X[i];
    sumX2 += X[i] * X[i];
    sumY += Y[i];
    sumXY += X[i] * Y[i];
    sumX2Y += X[i] * X[i] * Y[i];
    sumX3 += X[i] * X[i] * X[i];
    sumX4 += X[i] * X[i] * X[i] * X[i];
  }

  if (sum == 0.) {
    return;
  }

  // Estimate flat
  double bg0_flat = 0.;
  calcFlatParameters(sum, sumY, bg0_flat);

  // Estimate linear
  double bg0_linear = 0.;
  double bg1_linear = 0.;
  calcLinearParameters(sum, sumX, sumY, sumXY, sumX2, bg0_linear, bg1_linear);

  // Estimate quadratic
  double bg0_quadratic = 0.;
  double bg1_quadratic = 0.;
  double bg2_quadratic = 0.;
  calcQuadraticParameters(sum, sumX, sumY, sumXY, sumX2, sumX2Y, sumX3, sumX4,
                          bg0_quadratic, bg1_quadratic, bg2_quadratic);

  // Setup to calculate the residuals
  double chisq_flat = 0.;
  double chisq_linear = 0.;
  double chisq_quadratic = 0.;
  auto residual_flat = constant(bg0_flat);
  auto residual_linear = linear(bg0_linear, bg1_linear);
  auto residual_quadratic =
      quadratic(bg0_quadratic, bg1_quadratic, bg2_quadratic);
  double num_points = 0.;

  // calculate the chisq - not normalized by the number of points
  for (size_t i = i_min; i < i_max; ++i) {
    if (haveGap && i >= p_min && i < p_max)
      continue;

    num_points += 1.;
    chisq_flat += residual_flat(X[i], Y[i]);
    chisq_linear += residual_linear(X[i], Y[i]);
    chisq_quadratic += residual_quadratic(X[i], Y[i]);
  }

  // convert to <reduced chisq> = chisq / (<number points> - <number
  // parameters>)
  chisq_flat = chisq_flat / (num_points - 1.);
  chisq_linear = chisq_linear / (num_points - 2.);
  chisq_quadratic = chisq_quadratic / (num_points - 3.);

  if (order < 2) {
    chisq_quadratic = BAD_CHISQ;
    if (order < 1) {
      chisq_linear = BAD_CHISQ;
    }
  }

  // choose the right background function to return
  // this is written that lower order polynomial wins in the case of a tie
  if ((chisq_flat <= chisq_linear) && (chisq_flat <= chisq_quadratic)) {
    out_bg0 = bg0_flat;
    out_chisq_red = chisq_flat;
  } else if ((chisq_linear <= chisq_flat) &&
             (chisq_linear <= chisq_quadratic)) {
    out_bg0 = bg0_linear;
    out_bg1 = bg1_linear;
    out_chisq_red = chisq_linear;
  } else {
    out_bg0 = bg0_quadratic;
    out_bg1 = bg1_quadratic;
    out_bg2 = bg2_quadratic;
    out_chisq_red = chisq_quadratic;
  }
}

void estimateBackground(const size_t order, const Histogram &histo,
                        const size_t i_min, const size_t i_max,
                        const size_t p_min, const size_t p_max, double &out_bg0,
                        double &out_bg1, double &out_bg2,
                        double &out_chisq_red) {
  const auto &X = histo.points();
  const auto &Y = histo.y();

  // fit with a hole in the middle
  estimate(order, X, Y, i_min, i_max, p_min, p_max, true, out_bg0, out_bg1,
           out_bg2, out_chisq_red);
}

void estimatePolynomial(const size_t order, const Histogram &histo,
                        const size_t i_min, const size_t i_max, double &out_bg0,
                        double &out_bg1, double &out_bg2,
                        double &out_chisq_red) {
  const auto &X = histo.points();
  const auto &Y = histo.y();
  estimate(order, X, Y, i_min, i_max, 0, 0, false, out_bg0, out_bg1, out_bg2,
           out_chisq_red);
}

} // namespace HistogramData
} // namespace Mantid
