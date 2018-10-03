// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_BSPLINE_H_
#define MANTID_CURVEFITTING_BSPLINE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

#include <boost/shared_ptr.hpp>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_version.h>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**

A wrapper around GSL functions implementing a B-spline.
This function can also calculate derivatives up to order 2 as a by product of
the spline.
*/
class DLLExport BSpline : public BackgroundFunction {

public:
  /// Constructor
  BSpline();
  /// overwrite IFunction base class methods
  std::string name() const override { return "BSpline"; }
  const std::string category() const override { return "Background"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void derivative1D(double *out, const double *xValues, size_t nData,
                    const size_t order) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;

private:
  /// Initialize the GSL objects.
  void resetGSLObjects();
  /// Reset fitting parameters
  void resetParameters();
  /// Reset b-spline knots
  void resetKnots();
  /// Copy break points from GSL internal objects
  void getGSLBreakPoints(std::vector<double> &bp) const;

  /// structure used by GSL internally
  boost::shared_ptr<gsl_bspline_workspace> m_bsplineWorkspace;
#if GSL_MAJOR_VERSION < 2
  mutable boost::shared_ptr<gsl_bspline_deriv_workspace>
      m_bsplineDerivWorkspace;
#endif
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_BSPLINE_H_*/
