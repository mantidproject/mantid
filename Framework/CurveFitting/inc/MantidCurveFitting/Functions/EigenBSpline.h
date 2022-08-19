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

typedef Eigen::Spline<double, 1, 2> Spline2Degree;
typedef Eigen::Spline<double, 1, 3> Spline3Degree;
typedef Eigen::Spline<double, 1, 4> Spline4Degree;

/**
A wrapper around Eigen functions implementing a B-spline.
This function can also calculate basis function derivatives.
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

  /// Initialize the class objects.
  void resetObjects();
  /// Reset fitting parameters
  void resetParameters();
  /// Reset b-spline knots
  void resetKnots();
  /// Populate a vector with a uniform set of break points
  std::vector<double> calcUniformBreakPoints(const double startX, const double endX);
  /// Generate a knot vector based upon break points
  std::vector<double> generateKnotVector(const std::vector<double> &breakPoints);
  /// evaluate non-zero basis functions, return which index to use as the base of the results vector.
  int evaluateBasisFunctions(EigenVector &B, const double x, int currentBBase) const;
  /// initialise the m_spline variable with a given knot vector and breakpoints
  void initialiseSpline(const std::vector<double> &knots, const std::vector<double> &breakPoints);
  /// get the index of the span/interval which x is in
  int EigenBSpline::getSpanIndex(const double x, const int currentBBase, const bool clamped = true) const;
  /// Evaluate derivatives up to a specified order for each non-zero basis function
  EigenMatrix evaluateBasisFnDerivatives(const double x, const int derivOrder) const;

  /// Get number of B-Spline coefficients
  inline int getNBSplineCoefficients() { return getNBreakPoints() + getOrder() - 2; }
  /// Get number of Break Points
  inline int getNBreakPoints() const { return getAttribute("NBreak").asInt(); }
  /// Get number of Knots
  inline int getNKnots() const { return getNBreakPoints() + getClampedKnots() * 2 - 2; }
  /// Get order of the spline, k
  inline int getOrder() const { return getAttribute("Order").asInt(); }
  /// Get number of knots required to clamp the spline closed
  inline int getClampedKnots() const { return getDegree() + 1; }
  /// Get the degree of constituent polynomial functions
  inline int getDegree() const { return getOrder() - 1; }

  /// Member viarbles for spline of each compatable degree
  Spline2Degree m_spline2;
  Spline3Degree m_spline3;
  Spline4Degree m_spline4;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
