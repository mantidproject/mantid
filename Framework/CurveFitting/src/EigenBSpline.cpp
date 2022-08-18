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
#include <limits>
#include <time.h>

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
  B[0] = std::numeric_limits<double>::max();

  if (startX >= endX) {
    throw std::invalid_argument("BSpline: EndX must be greater than StartX.");
  }

  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    if (x < startX || x > endX) {
      out[i] = 0.0;
    } else {
      currentBBase = evaluateBasisFunctions(m_spline, B, x, currentBBase);

      double val = 0.0;
      for (size_t j = 0; j < np; ++j) {
        val += getParameter(j) * B.get(j);
      }
      out[i] = val;
    }
  }
}

void EigenBSpline::initialiseSpline(const std::vector<double> &knots, const std::vector<double> &breakPoints) {
  m_spline = Spline1D(EigenVector(knots).mutator(), EigenVector(breakPoints).mutator());
}

int EigenBSpline::evaluateBasisFunctions(const Spline1D &spline, EigenVector &B, const double x,
                                         int currentBBase) const {
  auto res = spline.basisFunctions(x); // Calculate Non-Zero Basis Functions

  if (res.data()[0] >= B[currentBBase]) { // attempts to shift results along the vector when a particular basis function
                                          // goes out of scope.
    currentBBase++;
  }
  B.zero();

  for (int i = 0; i < res.size(); i++) { // Populate B
    B[currentBBase + i] = res.data()[i];
  }

  return currentBBase;
}

/** Calculate the derivatives for a set of points on the spline
 *
 * @param out :: The array to store the derivatives in
 * @param xValues :: The array of x values we wish to know the derivatives of
 * @param nData :: The size of the arrays
 * @param order :: The order of the derivatives o calculate
 */
void EigenBSpline::derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const {
  // int splineOrder = getAttribute("Order").asInt();

  // EigenMatrix B(splineOrder, order + 1);
  // double startX = getAttribute("StartX").asDouble();
  // double endX = getAttribute("EndX").asDouble();

  // if (startX >= endX) {
  //    throw std::invalid_argument("BSpline: EndX must be greater than StartX.");
  //}

  // for (size_t i = 0; i < nData; ++i) {
  //    double x = xValues[i];
  //    if (x < startX || x > endX) {
  //        out[i] = 0.0;
  //    }
  //    else {
  //        size_t jstart(0);
  //        size_t jend(0);

  //        EigenMatrix B_tr(B.tr());
  //        gsl_matrix_view B_gsl_tr = getGSLMatrixView(B_tr.mutator());
  //        gsl_bspline_deriv_eval_nonzero(x, order, &B_gsl_tr.matrix, &jstart, &jend,
  //        m_bsplineWorkspace.get()); B = B_tr.tr();

  //        double val = 0.0;
  //        for (size_t j = jstart; j <= jend; ++j) {
  //            val += getParameter(j) * B.get(j - jstart, order);
  //        }
  //        out[i] = val;
  //    }
  //}
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
  return {"Uniform", "Order", "NBreak", "StartX", "EndX", "BreakPoints"};
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
 * Recalculate the B-spline knots
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
    knots = generateUniformKnotVector();
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
 * Get number of B-Spline Coefficients
 * @returns :: number of B-Spline Coefficients as int
 */
int EigenBSpline::getNBSplineCoefficients() { return getNBreakPoints() + getOrder() - 2; }

/**
 * Return the number of break points as per the NBreak Atrribute;
 * @returns the number break points
 */
int EigenBSpline::getNBreakPoints() { return getAttribute("NBreak").asInt(); }

/**
 * Populate a provided vector with a set of uniform break points
 * @param startX :: A double representing the first x value of the range
 * @param endX :: A double representing the first last x value of the range
 * @param breakPoints :: a vector to populate
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

std::vector<double> EigenBSpline::generateUniformKnotVector(const bool clamped) {
  const int nKnots = getNKnots();
  const int clampedKnots = clamped ? getDegree() + 1 : 1;
  const double startX = getAttribute("StartX").asDouble();
  const double endX = getAttribute("EndX").asDouble();
  const double interval = (endX - startX) / (nKnots - (2 * clampedKnots - 2) - 1);
  std::vector<double> knots(nKnots);
  std::generate(knots.begin(), knots.end(), [n = 1, &interval, &clampedKnots, &nKnots, &startX, &endX]() mutable {
    return n <= clampedKnots ? (0 * n++) + startX : std::min(((n++) - clampedKnots) * interval + startX, endX);
  });
  return knots;
};

int EigenBSpline::getNKnots() { return getNSpans() + 1; };

int EigenBSpline::getOrder() { return getAttribute("Order").asInt(); }

int EigenBSpline::getNSpans() { return getNBreakPoints() + getOrder(); }

int EigenBSpline::getDegree() { return getOrder() - 1; }

std::vector<double> EigenBSpline::generateKnotVector(const std::vector<double> &breakPoints) {
  const int nKnots = getNKnots();
  const int clampedKnots = getDegree() + 1;
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
