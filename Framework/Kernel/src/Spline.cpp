// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Spline.h"
#include "MantidKernel/DllConfig.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>

#include <algorithm>
#include <vector>

namespace Mantid::Kernel {

template <typename X, typename Y>
Spline<X, Y>::Spline(std::vector<X> const &x, std::vector<Y> const &y, gsl_interp_type const *type)
    : m_spline(gsl_spline_alloc(type, x.size())), m_acc(gsl_interp_accel_alloc()) {
  gsl_spline_init(m_spline, x.data(), y.data(), x.size());
}

template <typename X, typename Y> Spline<X, Y>::~Spline() {
  gsl_spline_free(m_spline);
  gsl_interp_accel_free(m_acc);
}

template <typename X, typename Y> Y Spline<X, Y>::operator()(X const newX) const {
  return gsl_spline_eval(m_spline, newX, m_acc);
}

template <typename X, typename Y> std::vector<Y> Spline<X, Y>::operator()(std::vector<X> const &newX) const {
  std::vector<Y> newY;
  newY.reserve(newX.size());
  std::transform(newX.cbegin(), newX.cend(), std::back_inserter(newY), [this](X const x) { return (*this)(x); });
  return newY;
}

template <typename X, typename Y>
std::vector<Y> CubicSpline<X, Y>::getSplinedYValues(std::vector<X> const &newX, std::vector<X> const &x,
                                                    std::vector<Y> const &y) {
  CubicSpline<X, Y> spline(x, y);
  return spline(newX);
}

template <typename X, typename Y>
std::vector<Y> LinearSpline<X, Y>::getSplinedYValues(std::vector<X> const &newX, std::vector<X> const &x,
                                                     std::vector<Y> const &y) {
  LinearSpline<X, Y> spline(x, y);
  return spline(newX);
}

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL CubicSpline<double, double>;
EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL LinearSpline<double, double>;

} // namespace Mantid::Kernel
