// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <gsl/gsl_spline.h>
#include <vector>

namespace Mantid::Kernel {

template <typename X, typename Y> class Spline {
  gsl_spline *m_spline;
  gsl_interp_accel *m_acc;

public:
  Spline(std::vector<X> const &x, std::vector<Y> const &y, gsl_interp_type const *);
  virtual ~Spline();
  Y operator()(X const newX) const;
  std::vector<Y> operator()(std::vector<X> const &newX) const;
};

template <typename X, typename Y> class CubicSpline : public Spline<X, Y> {
public:
  CubicSpline(std::vector<X> const &x, std::vector<Y> const &y) : Spline<X, Y>(x, y, gsl_interp_cspline) {};
  static std::vector<Y> getSplinedYValues(std::vector<X> const &newX, std::vector<X> const &x, std::vector<Y> const &y);
};

template <typename X, typename Y> class LinearSpline : public Spline<X, Y> {
public:
  LinearSpline(std::vector<X> const &x, std::vector<Y> const &y) : Spline<X, Y>(x, y, gsl_interp_linear) {};
  static std::vector<Y> getSplinedYValues(std::vector<X> const &newX, std::vector<X> const &x, std::vector<Y> const &y);
};

} // namespace Mantid::Kernel
