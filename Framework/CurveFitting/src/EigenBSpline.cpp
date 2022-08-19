// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/EigenBSpline.h"
#include "MantidAPI/FunctionFactory.h"

#include <algorithm>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(EigenBSpline)

/**
 * Constructor
 */
EigenBSpline::EigenBSpline() {
  const size_t nbreak = 10;
  declareAttribute("Uniform", Attribute(true));
  declareAttribute("Order", Attribute(3));
  declareAttribute("NBreak", Attribute(static_cast<int>(nbreak)));

  declareAttribute("StartX", Attribute(0.0));
  declareAttribute("EndX", Attribute(1.0));
  declareAttribute("BreakPoints", Attribute(std::vector<double>(nbreak)));
  declareAttribute("Knots", Attribute(std::vector<double>(getNKnots())));

  resetObjects();
  resetParameters();
  resetKnots();
}

/** Execute the function
 *
 * @param out :: The array to store the calculated y values
 * @param xValues :: The array of x values to interpolate
 * @param nData :: The size of the arrays
 */
void EigenBSpline::function1D(double *out, const double *xValues, const size_t nData) const {
  size_t np = nParams();
  EigenVector B(np);
  int currentBBase = 0;
  double startX = getAttribute("StartX").asDouble();
  double endX = getAttribute("EndX").asDouble();
  const std::vector<double> breakPoints = getAttribute("BreakPoints").asVector();
  const std::vector<double> knots = getAttribute("Knots").asVector();

  if (startX >= endX) {
    throw std::invalid_argument("BSpline: EndX must be greater than StartX.");
  }

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
 * @param knots :: Vector of knots
 * @param breakPoints :: Vector of breakpoints to be passed as control points
 */
void EigenBSpline::initialiseSpline(const std::vector<double> &knots, const std::vector<double> &breakPoints) {
  int degree = getDegree();
  switch (degree) {
  case 2:
    m_spline2 = Spline2Degree(EigenVector(knots).mutator(), EigenVector(breakPoints).mutator());
    break;
  case 3:
    m_spline3 = Spline3Degree(EigenVector(knots).mutator(), EigenVector(breakPoints).mutator());
    break;
  case 4:
    m_spline4 = Spline4Degree(EigenVector(knots).mutator(), EigenVector(breakPoints).mutator());
    break;
  default:
    throw std::invalid_argument("Spline order must be between 3 and 5 inclusive");
  }
}

/** Evaluate the basis functions that make up the spline at point x
 *
 * @param B :: The EigenVector to store the results in
 * @param x :: Position at which to evaluate the basis functions
 * @param currentBBase :: The last return value from this function
 * @returns :: The index to use as the base in the results vector. This corresponds to the
 * index of the first basis vector being evaluated at point x, along the entire spline.
 */
int EigenBSpline::evaluateBasisFunctions(EigenVector &B, const double x, int currentBBase) const {
  int degree = getDegree();
  Eigen::Array<double, 1, Eigen::Dynamic> res;
  switch (degree) {
  case 2:
    res = m_spline2.basisFunctions(x);
    break;
  case 3:
    res = m_spline3.basisFunctions(x);
    break;
  case 4:
    res = m_spline4.basisFunctions(x);
    break;
  default:
    throw std::invalid_argument("Spline order must be between 3 and 5 inclusive");
  }
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
 * @clamped :: if the spline is to be clamped or not (BSpline only currently supports clamped).
 * @returns :: The index of the span to which point x corresponds, base 0, not including spans between the
 * clamped knots.
 */
int EigenBSpline::getSpanIndex(const double x, const int currentBBase, const bool clamped) const {
  auto knots = getAttribute("Knots").asVector();
  const int clampedKnots = clamped ? getClampedKnots() : 1;
  for (int i = currentBBase + clampedKnots; i < knots.size(); i++) {
    if (x < knots[i]) {
      return i - clampedKnots;
    }
  }
  return knots.size() - clampedKnots * 2;
}

/** Calculate the derivatives for a set of points on the spline
 *
 * @param out :: The array to store the derivatives in
 * @param xValues :: The array of x values we wish to know the derivatives of
 * @param nData :: The size of the arrays
 * @param order :: The order of the derivatives to calculate
 */
void EigenBSpline::derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const {
  int splineOrder = getAttribute("Order").asInt();
  double startX = getAttribute("StartX").asDouble();
  double endX = getAttribute("EndX").asDouble();
  int jstart = 0;

  if (startX >= endX) {
    throw std::invalid_argument("BSpline: EndX must be greater than StartX.");
  }

  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    if (x < startX || x > endX) {
      out[i] = 0.0;
    } else {
      jstart = getSpanIndex(x, jstart);
      int jend = jstart + splineOrder;

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
EigenMatrix EigenBSpline::evaluateBasisFnDerivatives(const double x, const int derivOrder) const {
  int degree = getDegree();
  EigenMatrix res(derivOrder + 1, degree + 1);
  switch (degree) {
  case 2:
    res.mutator() = m_spline2.basisFunctionDerivatives(x, derivOrder);
    break;
  case 3:
    res.mutator() = m_spline3.basisFunctionDerivatives(x, derivOrder);
    break;
  case 4:
    res.mutator() = m_spline4.basisFunctionDerivatives(x, derivOrder);
    break;
  default:
    throw std::invalid_argument("Spline order must be between 3 and 5 inclusive");
  }
  return res;
}

/** Set an attribute for the function
 *
 * @param attName :: The name of the attribute to set
 * @param att :: The attribute to set
 */
void EigenBSpline::setAttribute(const std::string &attName, const API::IFunction::Attribute &att) {
  bool isUniform = attName == "Uniform" && att.asBool();

  storeAttributeValue(attName, att);

  if (attName == "BreakPoints" || isUniform || attName == "StartX" || attName == "EndX") {
    resetKnots();
  } else if (attName == "NBreak" || attName == "Order") {
    resetObjects();
    resetParameters();
    resetKnots();
  }
}

/**
 * @return Names of all declared attributes in correct order.
 */
std::vector<std::string> EigenBSpline::getAttributeNames() const {
  return {"Uniform", "Order", "NBreak", "StartX", "EndX", "BreakPoints", "Knots"};
}

/**
 * Initialize the class objects.
 */
void EigenBSpline::resetObjects() {
  int order = getAttribute("Order").asInt();
  int nbreak = getAttribute("NBreak").asInt();
  if (order <= 0) {
    throw std::invalid_argument("BSpline: Order must be greater than zero.");
  }
  if (nbreak < 2) {
    throw std::invalid_argument("BSpline: NBreak must be at least 2.");
  }
  // TO DO - RESET ANY EIGEN CLASS VARIABLES
}

/**
 * Reset fitting parameters after changes to some attributes.
 */
void EigenBSpline::resetParameters() {
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
void EigenBSpline::resetKnots() {
  bool isUniform = getAttribute("Uniform").asBool();
  std::vector<double> breakPoints;
  std::vector<double> knots;
  if (isUniform) {
    // create uniform knots in the interval [StartX, EndX]
    double startX = getAttribute("StartX").asDouble();
    double endX = getAttribute("EndX").asDouble();
    // calc uniform break points
    breakPoints = calcUniformBreakPoints(startX, endX);
    storeAttributeValue("BreakPoints", Attribute(breakPoints));
    // calc uniform knots
    knots = generateKnotVector(breakPoints);
    setAttribute("Knots", Attribute(knots));
  } else {
    // set the break points from BreakPoints vector attribute, update other attributes
    breakPoints = getAttribute("BreakPoints").asVector();
    // check that points are in ascending order
    double prev = breakPoints[0];
    for (size_t i = 1; i < breakPoints.size(); ++i) {
      double next = breakPoints[i];
      if (next <= prev) {
        throw std::invalid_argument("BreakPoints must be in ascending order.");
      }
      prev = next;
    }
    int nbreaks = getNBreakPoints();
    // if number of break points change do necessary updates
    if (static_cast<size_t>(nbreaks) != breakPoints.size()) {
      storeAttributeValue("NBreak", Attribute(static_cast<int>(breakPoints.size())));
      resetObjects();
      resetParameters();
    }
    knots = generateKnotVector(breakPoints);
    setAttribute("Knots", Attribute(knots));
    storeAttributeValue("StartX", Attribute(breakPoints.front()));
    storeAttributeValue("EndX", Attribute(breakPoints.back()));
  }
  initialiseSpline(knots, breakPoints);
}

/**
 * Populate a provided vector with a set of uniform break points
 * @param startX :: A double representing the first x value of the range
 * @param endX :: A double representing the first last x value of the range
 * @returns :: a vector with a set of uniform break points
 */
std::vector<double> EigenBSpline::calcUniformBreakPoints(const double startX, const double endX) {
  const int nBreak = getNBreakPoints();
  std::vector<double> breakPoints(nBreak);
  const double interval = (endX - startX) / (nBreak - 1.0);
  std::generate(breakPoints.begin(), breakPoints.end(),
                [n = 0, &interval, &startX]() mutable { return n++ * interval + startX; });
  return breakPoints;
}

/**
 * Generate a knot vector given a vector of break points
 * @param breakPoints :: A vector of breakpoints
 * @returns :: Populated knot vector
 */
std::vector<double> EigenBSpline::generateKnotVector(const std::vector<double> &breakPoints) {
  const int nKnots = getNKnots();
  const int clampedKnots = getClampedKnots();
  std::vector<double> knots(nKnots);

  for (int i = 0; i < nKnots; i++) {
    if (i < clampedKnots) {
      knots[i] = breakPoints[0];
    } else if (i >= nKnots - clampedKnots) {
      knots[i] = breakPoints[breakPoints.size() - 1];
    } else {
      knots[i] = breakPoints[i - clampedKnots + 1];
    }
  }
  return knots;
};

} // namespace Mantid::CurveFitting::Functions
