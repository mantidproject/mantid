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
#include "MantidKernel/IValidator.h"

#include <memory>
#include <unsupported/Eigen/Splines>

namespace Mantid::CurveFitting::Functions {

typedef Eigen::Spline<double, 1, Eigen::Dynamic> Spline1D;

/**
A wrapper around Eigen functions implementing a B-spline.
This function can also calculate basis function derivatives.
*/
class MANTID_CURVEFITTING_DLL BSpline : public BackgroundFunction {
public:
  /// Constructor
  BSpline();
  /// overwrite IFunction base class methods
  std::string name() const override { return "BSpline"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;

  /// Reset fitting parameters
  void resetParameters();
  /// Reset b-spline knots
  void resetKnots();
  /// Reset Function Attribute Validators
  void resetValidators();
  /// Populate a vector with a uniform set of break points
  [[nodiscard]] std::vector<double> calcUniformBreakPoints(const double startX, const double endX);
  /// Reset knot vector based upon break points
  void resetKnotVector(const std::vector<double> &breakPoints);
  /// evaluate non-zero basis functions, return which index to use as the base of the results vector.
  size_t evaluateBasisFunctions(EigenVector &B, const double x, size_t currentBBase) const;
  /// initialise the m_spline variable based on the knot vector and given a vector of breakpoints
  void initialiseSpline(const std::vector<double> &breakPoints);
  /// get the index of the span/interval which x is in
  size_t getSpanIndex(const double x, const size_t currentBBase, const bool clamped = true) const;
  /// Evaluate derivatives up to a specified order for each non-zero basis function
  [[nodiscard]] EigenMatrix evaluateBasisFnDerivatives(const double x, const size_t derivOrder) const;

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

  /// Member variable for spline
  Spline1D m_spline;
  std::vector<double> m_knots;
};

} // namespace Mantid::CurveFitting::Functions
