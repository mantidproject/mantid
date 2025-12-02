// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/GSL_Helpers.h"
#include <algorithm>
#include <span>
#include <vector>

namespace Mantid::Kernel {

/**
 * @brief Generic spline interpolation base class
 * @tparam X Type for x-coordinates (must be numeric)
 * @tparam Y Type for y-coordinates (must be numeric)
 */
template <typename X, typename Y> class Spline {
  static_assert(std::is_floating_point<X>(), "Splines must have floating point X values");
  static_assert(std::is_floating_point<Y>(), "Splines must have floating point Y values");

  spline::spline_uptr m_spline;
  spline::accel_uptr m_acc;
  Spline(Spline const &) = delete;
  Spline(Spline &&) = delete;
  Spline &operator=(Spline const &) = delete;
  Spline &operator=(Spline &&) = delete;

  X xForRange(X const x) const {
    if (x < m_spline->interp->xmin) {
      return m_spline->interp->xmin;
    } else if (x > m_spline->interp->xmax) {
      return m_spline->interp->xmax;
    } else {
      return x;
    }
  }

public:
  Spline(std::span<X const> x, std::span<Y const> y, gsl_interp_type const *type)
      : m_spline(spline::make_spline<X, Y>(x, y, type)), m_acc(spline::make_interp_accel()) {}
  virtual ~Spline() = default;
  Y operator()(X const newX) const {
    Y y;
    X properX = xForRange(newX);
    int err = gsl_spline_eval_e(m_spline.get(), properX, m_acc.get(), &y);
    if (err != GSL_SUCCESS) {
      throw std::runtime_error("Failure in GSL spline interpolation at " + std::to_string(newX));
    }
    return y;
  }
  std::vector<Y> operator()(std::span<X const> newX) const {
    std::vector<Y> newY;
    newY.reserve(newX.size());
    std::transform(newX.begin(), newX.end(), std::back_inserter(newY), [this](X const x) { return (*this)(x); });
    return newY;
  }
  Y deriv(X const newX, unsigned int order = 1) const {
    Y deriv;
    int err = GSL_SUCCESS;
    bool outOfRange = (newX != xForRange(newX)); // xForRange = newX if within bounds
    if (outOfRange) {
      // if we're outside the range, just return 0
      deriv = 0;
    } else {
      // otherwise evaltue up to the second derivative
      if (order == 1) {
        err = gsl_spline_eval_deriv_e(m_spline.get(), newX, m_acc.get(), &deriv);
      } else if (order == 2) {
        err = gsl_spline_eval_deriv2_e(m_spline.get(), newX, m_acc.get(), &deriv);
      } else {
        // for derivatives higher than 2, just return 0
        deriv = 0;
      }
    }
    if (err != GSL_SUCCESS) {
      throw std::runtime_error("Failure in GSL spline derivative at " + std::to_string(newX));
    }
    return deriv;
  }
  std::vector<Y> deriv(std::span<X const> newX, unsigned int order = 1) const {
    std::vector<Y> derivY;
    derivY.reserve(newX.size());
    std::transform(newX.begin(), newX.end(), std::back_inserter(derivY),
                   [this, order](X const x) { return this->deriv(x, order); });
    return derivY;
  }
};

/**
 * @brief Cubic spline interpolation using GSL.
 * @tparam X Type for x-coordinates (must be numeric)
 * @tparam Y Type for y-coordinates (must be numeric)
 */
template <typename X, typename Y> class CubicSpline : public Spline<X, Y> {
public:
  CubicSpline(std::span<X const> x, std::span<Y const> y) : Spline<X, Y>(x, y, gsl_interp_cspline) {}
  static std::vector<Y> getSplinedYValues(std::span<X const> newX, std::span<X const> x, std::span<Y const> y) {
    CubicSpline<X, Y> spline(x, y);
    return spline(newX);
  }
};

/**
 * @brief Linear interpolation using GSL.
 * @tparam X Type for x-coordinates (must be numeric)
 * @tparam Y Type for y-coordinates (must be numeric)
 */
template <typename X, typename Y> class LinearSpline : public Spline<X, Y> {
public:
  LinearSpline(std::span<X const> x, std::span<Y const> y) : Spline<X, Y>(x, y, gsl_interp_linear) {}
  static std::vector<Y> getSplinedYValues(std::span<X const> newX, std::span<X const> x, std::span<Y const> y) {
    LinearSpline<X, Y> spline(x, y);
    return spline(newX);
  }
};

} // namespace Mantid::Kernel
