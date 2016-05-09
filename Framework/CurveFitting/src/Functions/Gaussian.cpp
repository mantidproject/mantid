//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidAPI/FunctionFactory.h"
#include <boost/math/special_functions/fpclassify.hpp>

#include <cmath>
#include <numeric>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Gaussian)

Gaussian::Gaussian() : IPeakFunction(), m_intensityCache(0.0) {}

void Gaussian::init() {
  declareParameter("Height", 0.0, "Height of peak");
  declareParameter("PeakCentre", 0.0, "Centre of peak");
  declareParameter("Sigma", 0.0, "Width parameter");
}

void Gaussian::functionLocal(double *out, const double *xValues,
                             const size_t nData) const {
  const double height = getParameter("Height");
  const double peakCentre = getParameter("PeakCentre");
  const double weight = pow(1 / getParameter("Sigma"), 2);

  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - peakCentre;
    out[i] = height * exp(-0.5 * diff * diff * weight);
  }
}

void Gaussian::functionDerivLocal(Jacobian *out, const double *xValues,
                                  const size_t nData) {
  const double height = getParameter("Height");
  const double peakCentre = getParameter("PeakCentre");
  const double weight = pow(1 / getParameter("Sigma"), 2);

  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - peakCentre;
    double e = exp(-0.5 * diff * diff * weight);
    out->set(i, 0, e);
    out->set(i, 1, diff * height * e * weight);
    out->set(i, 2, -0.5 * diff * diff * height *
                       e); // derivative with respect to weight not sigma
  }
}

void Gaussian::setActiveParameter(size_t i, double value) {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma")
    setParameter(i, sqrt(fabs(1. / value)), false);
  else
    setParameter(i, value, false);
}

double Gaussian::activeParameter(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma")
    return 1. / pow(getParameter(i), 2);
  else
    return getParameter(i);
}

double Gaussian::centre() const { return getParameter("PeakCentre"); }
double Gaussian::height() const { return getParameter("Height"); }
double Gaussian::fwhm() const {
  return 2.0 * sqrt(2.0 * M_LN2) * getParameter("Sigma");
}
double Gaussian::intensity() const {
  auto sigma = getParameter("Sigma");
  if (sigma == 0.0) {
    auto height = getParameter("Height");
    if (boost::math::isfinite(height)) {
      m_intensityCache = height;
    }
  } else {
    m_intensityCache =
        getParameter("Height") * getParameter("Sigma") * sqrt(2.0 * M_PI);
  }
  return m_intensityCache;
}

void Gaussian::setCentre(const double c) { setParameter("PeakCentre", c); }
void Gaussian::setHeight(const double h) { setParameter("Height", h); }
void Gaussian::setFwhm(const double w) {
  setParameter("Sigma", w / (2.0 * sqrt(2.0 * M_LN2)));
}
void Gaussian::setIntensity(const double i) {
  m_intensityCache = i;
  auto sigma = getParameter("Sigma");
  if (sigma == 0.0) {
    setParameter("Height", i);
  } else {
    setParameter("Height", i / (sigma * sqrt(2.0 * M_PI)));
  }
}

void Gaussian::fixCentre() { fixParameter("PeakCentre"); }

void Gaussian::unfixCentre() { unfixParameter("PeakCentre"); }

void Gaussian::fixIntensity() {
  std::string formula =
      std::to_string(intensity() / sqrt(2.0 * M_PI)) + "/Sigma";
  tie("Height", formula);
}

void Gaussian::unfixIntensity() { removeTie("Height"); }

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
