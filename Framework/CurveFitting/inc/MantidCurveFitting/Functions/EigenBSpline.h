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

typedef Eigen::Spline<double, 1, 2> Spline1D;

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
  /// Generate a uniform knot vector
  std::vector<double> generateUniformKnotVector(const bool clamped = true);
  /// Get number of Knots
  int getNKnots();
  /// Get order of the spline
  int getOrder();
  /// Get number of spans (segments between knots)
  int getNSpans();
  /// Get the degree of constituent polynomial functions
  int getDegree();
  /// Generate a knot vector based upon break points
  std::vector<double> generateKnotVector(const std::vector<double> &breakPoints);
  /// evaluate non-zero basis functions, return which index to use as the base of the results vector.
  int evaluateBasisFunctions(const Spline1D &spline, EigenVector &B, const double x, int currentBBase) const;
  /// initialise the m_spline variable with a given knot vector and breakpoints
  void initialiseSpline(const std::vector<double> &knots, const std::vector<double> &breakPoints);

  Spline1D m_spline;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
