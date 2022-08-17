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
  declareAttribute("Order", Attribute(2));
  declareAttribute("NBreak", Attribute(static_cast<int>(nbreak)));

  declareAttribute("StartX", Attribute(0.0));
  declareAttribute("EndX", Attribute(1.0));
  declareAttribute("BreakPoints", Attribute(std::vector<double>(nbreak)));

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
  double startX = getAttribute("StartX").asDouble();
  double endX = getAttribute("EndX").asDouble();

  if (startX >= endX) {
    throw std::invalid_argument("BSpline: EndX must be greater than StartX.");
  }

  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    if (x < startX || x > endX) {
      out[i] = 0.0;
    } else {
      B.zero(); // reset vector
      Eigen::Vector3d pt = m_spline.basisFunctions(x);

      // populate B with results;
      double val = 0.0;
      for (size_t j = 0; j < np; ++j) {
        val += getParameter(j) * B.get(j);
      }
      out[i] = val;
    }
  }
}

/** Calculate the derivatives for a set of points on the spline
 *
 * @param out :: The array to store the derivatives in
 * @param xValues :: The array of x values we wish to know the derivatives of
 * @param nData :: The size of the arrays
 * @param order :: The order of the derivatives o calculate
 */
void EigenBSpline::derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const {

  //        int splineOrder = getAttribute("Order").asInt();
  //        auto k = static_cast<size_t>(splineOrder);
  //#if GSL_MAJOR_VERSION < 2
  //        if (!m_bsplineDerivWorkspace) {
  //            gsl_bspline_deriv_workspace* ws = gsl_bspline_deriv_alloc(k);
  //            m_bsplineDerivWorkspace = std::shared_ptr<gsl_bspline_deriv_workspace>(ws,
  //            ReleaseBSplineDerivativeWorkspace());
  //        }
  //#endif
  //
  //        EigenMatrix B(k, order + 1);
  //        double startX = getAttribute("StartX").asDouble();
  //        double endX = getAttribute("EndX").asDouble();
  //
  //        if (startX >= endX) {
  //            throw std::invalid_argument("BSpline: EndX must be greater than StartX.");
  //        }
  //
  //        for (size_t i = 0; i < nData; ++i) {
  //            double x = xValues[i];
  //            if (x < startX || x > endX) {
  //                out[i] = 0.0;
  //            }
  //            else {
  //                size_t jstart(0);
  //                size_t jend(0);
  //#if GSL_MAJOR_VERSION < 2
  //                gsl_matrix_view B_gsl = getGSLMatrixView(B.mutator());
  //                gsl_bspline_deriv_eval_nonzero(x, order, &B_gsl.matrix, &jstart, &jend, m_bsplineWorkspace.get(),
  //                    m_bsplineDerivWorkspace.get());
  //#else
  //                EigenMatrix B_tr(B.tr());
  //                gsl_matrix_view B_gsl_tr = getGSLMatrixView(B_tr.mutator());
  //                gsl_bspline_deriv_eval_nonzero(x, order, &B_gsl_tr.matrix, &jstart, &jend,
  //                m_bsplineWorkspace.get()); B = B_tr.tr();
  //#endif
  //
  //                double val = 0.0;
  //                for (size_t j = jstart; j <= jend; ++j) {
  //                    val += getParameter(j) * B.get(j - jstart, order);
  //                }
  //                out[i] = val;
  //            }
  //        }
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
    breakPoints = calcUniformBreakPoints(startX, endX);
    storeAttributeValue("BreakPoints", Attribute(breakPoints));

    knots = generateUniformKnotVector();
  } else {
    // set the break points from BreakPoints vector attribute, update other
    // attributes
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
  const double interval = (endX - startX) / (nBreak - 1);
  std::generate(breakPoints.begin(), breakPoints.end(), [n = 0, &interval]() mutable { return n++ * interval; });
  return breakPoints;
}

void EigenBSpline::initialiseSpline(std::vector<double> &knotVector, std::vector<double> &breakPoints) {
  EigenVector evKnots(knotVector);
  // EigenVector evBreakPoints(breakPoints);
  EigenMatrix mBP = test_make_break_points_2d(breakPoints);

  m_spline = Spline(evKnots.mutator(), mBP.mutator());
}

EigenMatrix EigenBSpline::test_make_break_points_2d(std::vector<double> breakPoints) {
  const int size = breakPoints.size();
  EigenMatrix m(2, size);
  for (int i = 0; i < size; i++) {
    m(0, i) = breakPoints[i];
    srand((unsigned)time(NULL));
    m(1, i) = (double)rand() / RAND_MAX;
  }

  return m;
}

std::vector<double> EigenBSpline::generateUniformKnotVector(bool clamped) {
  const int nKnots = getNKnots();
  const int clampedKnots = clamped ? getOrder() + 1 : 1;
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

int EigenBSpline::getNSpans() { return getNBreakPoints() + getOrder() + 1; }

std::vector<double> EigenBSpline::generateKnotVector(std::vector<double> breakPoints) {
  // Implementation
  const int nKnots = getNKnots();
  std::vector<double> knots(nKnots);
  return knots;
};

} // namespace Mantid::CurveFitting::Functions
