// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BSpline.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(BSpline)

/**
 * Constructor
 */
BSpline::BSpline() {
  const size_t nbreak = 10;

  auto orderValidator = BoundedValidator<int>();
  orderValidator.setLower(1);
  declareAttribute("Order", Attribute(3), orderValidator);

  auto NBreakValidator = BoundedValidator<int>();
  NBreakValidator.setLower(2);
  declareAttribute("NBreak", Attribute(static_cast<int>(nbreak)), NBreakValidator);

  auto startXValidator = BoundedValidator<double>();
  startXValidator.setUpper(1.0);
  startXValidator.setUpperExclusive(true);
  declareAttribute("StartX", Attribute(0.0), startXValidator);

  auto endXValidator = BoundedValidator<double>();
  endXValidator.setLower(0.0);
  endXValidator.setLowerExclusive(true);
  declareAttribute("EndX", Attribute(1.0), endXValidator);

  auto breakPointsValidator = ArrayBoundedValidator<double>(0.0, 1.0);
  breakPointsValidator.setError(1e-8);
  declareAttribute("BreakPoints", Attribute(std::vector<double>(nbreak)), breakPointsValidator);

  declareAttribute("Uniform", Attribute(true));

  m_spline = Spline1D();
  resetParameters();
  resetKnots();
}

void BSpline::resetValidators() {
  auto attStartX = getAttribute("StartX");
  auto attEndX = getAttribute("EndX");

  auto startXValidator = std::dynamic_pointer_cast<BoundedValidator<double>>(attStartX.getValidator());
  startXValidator->setUpper(attEndX.asDouble());

  auto endXValidator = std::dynamic_pointer_cast<BoundedValidator<double>>(attEndX.getValidator());
  endXValidator->setLower(attStartX.asDouble());

  auto breakPointsValidator =
      std::dynamic_pointer_cast<ArrayBoundedValidator<double>>(getAttribute("BreakPoints").getValidator());
  breakPointsValidator->setLower(attStartX.asDouble());
  breakPointsValidator->setUpper(attEndX.asDouble());
}

/** Execute the function
 *
 * @param out :: The array to store the calculated y values
 * @param xValues :: The array of x values to interpolate
 * @param nData :: The size of the arrays
 */
void BSpline::function1D(double *out, const double *xValues, const size_t nData) const {
  size_t np = nParams();
  EigenVector B(np);
  size_t currentBBase = 0;
  double startX = getAttribute("StartX").asDouble();
  double endX = getAttribute("EndX").asDouble();

  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    if (x < startX || x > endX) {
      out[i] = 0.0;
    } else {
      currentBBase = evaluateBasisFunctions(B, x, currentBBase);

      double val = 0.0;
      for (size_t j = 0; j < np; ++j) {
        val += getParameter(j) * B.get(j);
      }
      out[i] = val;
    }
  }
}

/** Initialise the spline member variable
 *
 * @param breakPoints :: Vector of breakpoints to be passed as control points
 */
void BSpline::initialiseSpline(const std::vector<double> &breakPoints) {
  m_spline = Spline1D(EigenVector(m_knots).mutator(), EigenVector(breakPoints).mutator());
}

/** Evaluate the basis functions that make up the spline at point x
 *
 * @param B :: The EigenVector to store the results in
 * @param x :: Position at which to evaluate the basis functions
 * @param currentBBase :: The last return value from this function
 * @returns :: The index to use as the base in the results vector. This corresponds to the
 * index of the first basis vector being evaluated at point x, along the entire spline.
 */
size_t BSpline::evaluateBasisFunctions(EigenVector &B, const double x, size_t currentBBase) const {
  int degree = getDegree();
  auto res = m_spline.BasisFunctions(x, degree, m_spline.knots());

  currentBBase = getSpanIndex(x, currentBBase);
  B.zero();
  for (int i = 0; i < res.size(); i++) { // Populate B
    B[currentBBase + i] = res.data()[i];
  }
  return currentBBase;
}

/** Return the span to which point x corresponds
 *
 * @param x :: a position along the spline
 * @param currentBBase :: The last return value from this function (for looping efficiency, given x will
 * always be in accending order)
 * @param clamped :: if the spline is to be clamped or not (BSpline only currently supports clamped).
 * @returns :: The index of the span to which point x corresponds, base 0, not including spans between the
 * clamped knots.
 */
size_t BSpline::getSpanIndex(const double x, const size_t currentBBase, const bool clamped) const {
  const size_t clampedKnots = clamped ? static_cast<size_t>(getClampedKnots()) : 1u;
  size_t nKnots = m_knots.size();
  for (size_t i = currentBBase + clampedKnots; i < nKnots; i++) {
    if (x < m_knots[i]) {
      return i - clampedKnots;
    }
  }
  return nKnots - clampedKnots * 2;
}

/** Calculate the derivatives for a set of points on the spline
 *
 * @param out :: The array to store the derivatives in
 * @param xValues :: The array of x values we wish to know the derivatives of
 * @param nData :: The size of the arrays
 * @param order :: The order of the derivatives to calculate
 */
void BSpline::derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const {
  double startX = getAttribute("StartX").asDouble();
  double endX = getAttribute("EndX").asDouble();
  size_t jstart = 0;
  size_t splineOrder = static_cast<size_t>(getAttribute("Order").asInt());

  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    if (x < startX || x > endX) {
      out[i] = 0.0;
    } else {
      jstart = getSpanIndex(x, jstart);
      size_t jend = jstart + splineOrder;

      auto res = evaluateBasisFnDerivatives(x, order);
      double val = 0.0;
      for (size_t j = jstart; j < jend; ++j) {
        val += getParameter(j) * res(order, j - jstart);
      }
      out[i] = val;
    }
  }
}

/** Calculate the derivatives of the basis functions for a specific point on the spline
 *
 * @param x :: A point on the spline
 * @param derivOrder :: The order of the derivatives to calculate
 * @returns :: An eigen matrix comprising of the derivatives for each basis function.
 */
EigenMatrix BSpline::evaluateBasisFnDerivatives(const double x, const size_t derivOrder) const {
  int degree = getDegree();
  EigenMatrix res(derivOrder + 1u, degree + 1);
  res.mutator() = m_spline.BasisFunctionDerivatives(x, derivOrder, degree, m_spline.knots());
  return res;
}

/** Set an attribute for the function
 *
 * @param attName :: The name of the attribute to set
 * @param att :: The attribute to set
 */
void BSpline::setAttribute(const std::string &attName, const API::IFunction::Attribute &att) {
  bool isUniform = attName == "Uniform" && att.asBool();
  storeAttributeValue(attName, att);

  if (attName == "BreakPoints") {
    storeAttributeValue("NBreak", Attribute(static_cast<int>(att.asVector().size())));
    resetParameters();
    resetKnots();
  } else if (isUniform || attName == "StartX" || attName == "EndX") {
    resetValidators();
    resetKnots();
  } else if (attName == "NBreak" || attName == "Order") {
    resetParameters();
    resetKnots();
  }
}

/**
 * @return Names of all declared attributes in correct order.
 */
std::vector<std::string> BSpline::getAttributeNames() const {
  return {"Uniform", "Order", "NBreak", "EndX", "StartX", "BreakPoints"};
}

/**
 * Reset fitting parameters after changes to some attributes.
 */
void BSpline::resetParameters() {
  if (nParams() > 0) {
    clearAllParameters();
  }
  size_t np = getNBSplineCoefficients();
  for (size_t i = 0; i < np; ++i) {
    std::string pname = "A" + std::to_string(i);
    declareParameter(pname);
  }
}

/**
 * Recalculate the B-spline knots and initialise spline variable
 */
void BSpline::resetKnots() {
  bool isUniform = getAttribute("Uniform").asBool();
  std::vector<double> breakPoints;
  if (isUniform) {
    // create uniform knots in the interval [StartX, EndX]
    double startX = getAttribute("StartX").asDouble();
    double endX = getAttribute("EndX").asDouble();
    // calc uniform break points
    breakPoints = calcUniformBreakPoints(startX, endX);
    storeAttributeValue("BreakPoints", Attribute(breakPoints));
    // calc uniform knots
    resetKnotVector(breakPoints);
  } else {
    // set the break points from BreakPoints vector attribute, update other attributes
    breakPoints = getAttribute("BreakPoints").asVector();
    // check that points are in ascending order
    double prev = breakPoints[0];
    for (size_t i = 1; i < breakPoints.size(); ++i) {
      double next = breakPoints[i];
      if (next < prev) {
        throw std::invalid_argument("BreakPoints must be in ascending order.");
      }
      prev = next;
    }
    int nbreaks = getNBreakPoints();
    // if number of break points change do necessary updates
    if (static_cast<size_t>(nbreaks) != breakPoints.size()) {
      storeAttributeValue("NBreak", Attribute(static_cast<int>(breakPoints.size())));
      resetParameters();
    }
    resetKnotVector(breakPoints);
  }
  initialiseSpline(breakPoints);
}

/**
 * Populate a provided vector with a set of uniform break points
 * @param startX :: A double representing the first x value of the range
 * @param endX :: A double representing the first last x value of the range
 * @returns :: a vector with a set of uniform break points
 */
std::vector<double> BSpline::calcUniformBreakPoints(const double startX, const double endX) {
  const int nBreak = getNBreakPoints();
  std::vector<double> breakPoints(nBreak);
  const double interval = (endX - startX) / (nBreak - 1.0);
  std::generate(breakPoints.begin(), breakPoints.end(),
                [n = 0, &interval, &startX]() mutable { return n++ * interval + startX; });
  return breakPoints;
}

/**
 * Reset knot vector given a vector of break points
 * @param breakPoints :: A vector of breakpoints
 */
void BSpline::resetKnotVector(const std::vector<double> &breakPoints) {
  const int nKnots = getNKnots();
  const int clampedKnots = getClampedKnots();
  m_knots.clear();
  m_knots.resize(nKnots);

  for (int i = 0; i < nKnots; i++) {
    if (i < clampedKnots) {
      m_knots[i] = breakPoints[0];
    } else if (i >= nKnots - clampedKnots) {
      m_knots[i] = breakPoints[breakPoints.size() - 1];
    } else {
      m_knots[i] = breakPoints[i - clampedKnots + 1];
    }
  }
}

} // namespace Mantid::CurveFitting::Functions
