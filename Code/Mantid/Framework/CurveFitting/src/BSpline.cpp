//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BSpline.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidAPI/FunctionFactory.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Mantid {
namespace CurveFitting {
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(BSpline)

namespace {
// shared pointer deleter for bspline workspace
struct ReleaseBSplineWorkspace {
  void operator()(gsl_bspline_workspace *ws) { gsl_bspline_free(ws); }
};
// shared pointer deleter for bspline derivative workspace
struct ReleaseBSplineDerivativeWorkspace {
  void operator()(gsl_bspline_deriv_workspace *ws) {
    gsl_bspline_deriv_free(ws);
  }
};
}

/**
 * Constructor
 */
BSpline::BSpline() : m_bsplineWorkspace(), m_bsplineDerivWorkspace() {
  const size_t nbreak = 10;
  declareAttribute("Uniform", Attribute(true));
  declareAttribute("Order", Attribute(3));
  declareAttribute("NBreak", Attribute(static_cast<int>(nbreak)));

  declareAttribute("StartX", Attribute(0.0));
  declareAttribute("EndX", Attribute(1.0));
  declareAttribute("BreakPoints", Attribute(std::vector<double>(nbreak)));

  resetGSLObjects();
  resetParameters();
  resetKnots();
}

/**
 * Destructor
 */
BSpline::~BSpline() {}

/** Execute the function
 *
 * @param out :: The array to store the calculated y values
 * @param xValues :: The array of x values to interpolate
 * @param nData :: The size of the arrays
 */
void BSpline::function1D(double *out, const double *xValues,
                         const size_t nData) const {
  size_t np = nParams();
  GSLVector B(np);
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
      gsl_bspline_eval(x, B.gsl(), m_bsplineWorkspace.get());
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
void BSpline::derivative1D(double *out, const double *xValues, size_t nData,
                           const size_t order) const {

  int splineOrder = getAttribute("Order").asInt();
  size_t k = static_cast<size_t>(splineOrder);
  if (!m_bsplineDerivWorkspace) {
    gsl_bspline_deriv_workspace *ws = gsl_bspline_deriv_alloc(k);
    m_bsplineDerivWorkspace = boost::shared_ptr<gsl_bspline_deriv_workspace>(
        ws, ReleaseBSplineDerivativeWorkspace());
  }

  GSLMatrix B(k, order + 1);
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
      size_t jstart(0);
      size_t jend(0);
      gsl_bspline_deriv_eval_nonzero(x, order, B.gsl(), &jstart, &jend,
                                     m_bsplineWorkspace.get(),
                                     m_bsplineDerivWorkspace.get());
      double val = 0.0;
      for (size_t j = jstart; j <= jend; ++j) {
        val += getParameter(j) * B.get(j - jstart, order);
      }
      out[i] = val;
    }
  }
}

/** Set an attribute for the function
 *
 * @param attName :: The name of the attribute to set
 * @param att :: The attribute to set
 */
void BSpline::setAttribute(const std::string &attName,
                           const API::IFunction::Attribute &att) {
  bool isUniform = attName == "Uniform" && att.asBool();

  storeAttributeValue(attName, att);

  if (attName == "BreakPoints" || isUniform || attName == "StartX" ||
      attName == "EndX") {
    resetKnots();
  } else if (attName == "NBreak" || attName == "Order") {
    resetGSLObjects();
    resetParameters();
    resetKnots();
  }
}

/**
 * @return Names of all declared attributes in correct order.
 */
std::vector<std::string> BSpline::getAttributeNames() const {
  std::vector<std::string> names;
  names.push_back("Uniform");
  names.push_back("Order");
  names.push_back("NBreak");
  names.push_back("StartX");
  names.push_back("EndX");
  names.push_back("BreakPoints");
  return names;
}

/**
 * Initialize the GSL objects.
 */
void BSpline::resetGSLObjects() {
  int order = getAttribute("Order").asInt();
  int nbreak = getAttribute("NBreak").asInt();
  if (order <= 0) {
    throw std::invalid_argument("BSpline: Order must be greater than zero.");
  }
  if (nbreak < 2) {
    throw std::invalid_argument("BSpline: NBreak must be at least 2.");
  }
  gsl_bspline_workspace *ws = gsl_bspline_alloc(static_cast<size_t>(order),
                                                static_cast<size_t>(nbreak));
  m_bsplineWorkspace =
      boost::shared_ptr<gsl_bspline_workspace>(ws, ReleaseBSplineWorkspace());
  m_bsplineDerivWorkspace.reset();
}

/**
 * Reset fitting parameters after changes to some attributes.
 */
void BSpline::resetParameters() {
  if (nParams() > 0) {
    clearAllParameters();
  }
  size_t np = gsl_bspline_ncoeffs(m_bsplineWorkspace.get());
  for (size_t i = 0; i < np; ++i) {
    std::string pname = "A" + boost::lexical_cast<std::string>(i);
    declareParameter(pname);
  }
}

/**
 * Recalculate the B-spline knots
 */
void BSpline::resetKnots() {
  bool isUniform = getAttribute("Uniform").asBool();

  std::vector<double> breakPoints;
  if (isUniform) {
    // create uniform knots in the interval [StartX, EndX]
    double startX = getAttribute("StartX").asDouble();
    double endX = getAttribute("EndX").asDouble();
    gsl_bspline_knots_uniform(startX, endX, m_bsplineWorkspace.get());
    getGSLBreakPoints(breakPoints);
    storeAttributeValue("BreakPoints", Attribute(breakPoints));
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
    int nbreaks = getAttribute("NBreak").asInt();
    // if number of break points change do necessary updates
    if (static_cast<size_t>(nbreaks) != breakPoints.size()) {
      storeAttributeValue("NBreak",
                          Attribute(static_cast<int>(breakPoints.size())));
      resetGSLObjects();
      resetParameters();
    }
    GSLVector bp = breakPoints;
    gsl_bspline_knots(bp.gsl(), m_bsplineWorkspace.get());
    storeAttributeValue("StartX", Attribute(breakPoints.front()));
    storeAttributeValue("EndX", Attribute(breakPoints.back()));
  }
}

/**
 * Copy break points from GSL internal objects
 * @param bp :: A vector to accept the break points.
 */
void BSpline::getGSLBreakPoints(std::vector<double> &bp) const {
  size_t n = gsl_bspline_nbreak(m_bsplineWorkspace.get());
  bp.resize(n);
  for (size_t i = 0; i < n; ++i) {
    bp[i] = gsl_bspline_breakpoint(i, m_bsplineWorkspace.get());
  }
}

} // namespace CurveFitting
} // namespace Mantid
