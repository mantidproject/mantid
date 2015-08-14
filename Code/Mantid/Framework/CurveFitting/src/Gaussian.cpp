//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include "MantidCurveFitting/SimpleChebfun.h"
#include <cmath>
#include <numeric>

#include <boost/lexical_cast.hpp>
#include "C:/Users/hqs74821/Work/Mantid_stuff/Testing/class/MyTestDef.h"

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Gaussian)

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

void Gaussian::setInitialValues(const API::FunctionDomain &domain, API::FunctionValues &values) {
  if (!isExplicitlySet(2)) {

    auto &domain1d = dynamic_cast<const API::FunctionDomain1D&>(domain);
    size_t n = domain1d.size();

    std::vector<double> x(n);
    std::vector<double> y(n);
    for(size_t i = 0; i < n; ++i) {
      x[i] = domain1d[i];
      y[i] = values.getFitData(i);
    }

    SimpleChebfun fun(x, y);
    auto der1 = fun.derivative();
    auto der2 = der1.derivative();

    auto &xp = der2.xPoints();
    const double centre = getParameter("PeakCentre");
    auto icentre = std::lower_bound(xp.begin(), xp.end(), centre);
    if (icentre != xp.end()) {
      auto ic = static_cast<size_t>(std::distance(xp.begin(), icentre));
      const double d2max = der2(centre);
      auto PD2 = der2.yPoints();
      double left = 0.0;
      for(auto i = ic; i > 0; --i) {
        if (d2max * PD2[i] < 0.0) break;
        left = centre - xp[i];
      }
      double right = 0.0;
      for(auto i = ic; i < n; ++i) {
        if (d2max * PD2[i] < 0.0) break;
        right = xp[i] - centre;
      }

      double sigma = fabs(right + left) / 2;
      setParameter(2, sigma);
    }
  }
}

} // namespace CurveFitting
} // namespace Mantid
