// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

#include <boost/scoped_array.hpp>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_spline.h>
#include <valarray>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**

A wrapper around GSL functions implementing cubic spline interpolation.
This function can also calculate derivatives up to order 2 as a by product of
the spline.

@author Samuel Jackson, STFC
@date 05/07/2013
*/
class MANTID_CURVEFITTING_DLL CubicSpline : public BackgroundFunction {

public:
  /// Constructor
  CubicSpline();

  /// overwrite IFunction base class methods
  std::string name() const override { return "CubicSpline"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const override;
  void setParameter(size_t i, const double &value, bool explicitlySet = true) override;
  using ParamFunction::setParameter;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

  /// Set the value of a data point location to x
  void setXAttribute(const size_t index, double x);

private:
  /// Minimum number of data points in spline
  const int m_min_points;

  /// Functor to free a GSL objects in a shared pointer
  struct GSLFree {
    void operator()(gsl_spline *spline) { gsl_spline_free(spline); }
    void operator()(gsl_interp_accel *acc) { gsl_interp_accel_free(acc); }
  } m_gslFree;

  /// GSL interpolation accelerator object
  std::shared_ptr<gsl_interp_accel> m_acc;

  /// GSL data structure used to calculate spline
  std::shared_ptr<gsl_spline> m_spline;

  /// Flag for checking if the spline needs recalculating
  mutable bool m_recalculateSpline;

  /// Reallocate the spline object to use n data points
  void reallocGSLObjects(const int n);

  /// Method to setup the gsl function
  void setupInput(boost::scoped_array<double> &x, boost::scoped_array<double> &y, int n) const;

  /// Calculate the spline
  void calculateSpline(double *out, const double *xValues, const size_t nData) const;

  /// Calculate the derivative
  void calculateDerivative(double *out, const double *xValues, const size_t nData, const size_t order) const;

  /// Initialise GSL objects if required
  void initGSLObjects(boost::scoped_array<double> &x, boost::scoped_array<double> &y, int n) const;

  /// Check if an error occurred and throw appropriate message
  void checkGSLError(const int status, const int errorType) const;

  /// Check if an x value falls within the range of the spline
  bool checkXInRange(double x) const;

  /// Evaluate a point on the spline, with basic error handling
  double splineEval(const double x) const;
};

using CubicSpline_sptr = std::shared_ptr<CubicSpline>;
using CubicSpline_const_sptr = const std::shared_ptr<CubicSpline>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
