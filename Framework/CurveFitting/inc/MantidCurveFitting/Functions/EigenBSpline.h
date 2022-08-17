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
#include "MantidCurveFitting/EigenMatrix.h"
#include "MantidCurveFitting/EigenVector.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

#include <memory>
#include <unsupported/Eigen/Splines>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

typedef Eigen::Spline<double, 2> Spline;

/**
A wrapper around Eigen functions implementing a B-spline.
This function can also calculate derivatives up to order 2 as a by product of
the spline.
*/
class MANTID_CURVEFITTING_DLL EigenBSpline : public BackgroundFunction {

public:
  /// Constructor
  EigenBSpline();
  /// overwrite IFunction base class methods
  std::string name() const override { return "BSpline"; }
  const std::string category() const override { return "Background"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;

private:
  /// Initialize the class objects.
  void resetObjects();
  /// Reset fitting parameters
  void resetParameters();
  /// Reset b-spline knots
  void resetKnots();

  /// --- Eigen Implementation Functions
  /// Get number of B-Spline coefficients
  int getNBSplineCoefficients();
  /// Get number of Break Points
  int getNBreakPoints();
  /// Populate a vector with a uniform set of break points
  std::vector<double> calcUniformBreakPoints(const double startX, const double endX);
  /// Initialise the spline given a vector of knots and breakpoints
  void initialiseSpline(std::vector<double> &knotVector, std::vector<double> &breakPoints);
  /// Generate a uniform knot vector
  std::vector<double> generateUniformKnotVector(bool clamped = true);
  /// Get number of Knots
  int getNKnots();
  /// Get order of the spline
  int getOrder();
  /// Get number of spans (segments between knots)
  int getNSpans();
  /// Generate a knot vector based upon break points
  std::vector<double> generateKnotVector(std::vector<double> breakPoints);

  // Test FN
  EigenMatrix test_make_break_points_2d(std::vector<double> breakPoints);

  /// --- Eigen Implementation Member Variables
  Spline m_spline;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
