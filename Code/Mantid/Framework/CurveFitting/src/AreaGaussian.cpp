#include "MantidCurveFitting/AreaGaussian.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {

using namespace API;

DECLARE_FUNCTION(AreaGaussian)

void AreaGaussian::functionLocal(double *out, const double *xValues,
                                 const size_t nData) const {
  double area = getParameter("Area");
  double sigma = getParameter("Sigma");
  double centre = getParameter("Centre");

  for (size_t i = 0; i < nData; ++i) {
    out[i] = area / (sqrt(2.0 * M_PI) * sigma) *
             exp(-0.5 * pow((xValues[i] - centre) / sigma, 2.0));
  }
}

void AreaGaussian::functionDerivLocal(Jacobian *out, const double *xValues,
                                      const size_t nData) {
  double area = getParameter("Area");
  double sigma = getParameter("Sigma");
  double centre = getParameter("Centre");

  for (size_t i = 0; i < nData; ++i) {
    double diff = (xValues[i] - centre) / sigma;
    double expTerm = exp(-0.5 * diff * diff);
    double sigmaTerm = sqrt(2.0 * M_PI) * sigma;

    out->set(i, 0, diff * area * expTerm / (sigmaTerm * sigma));
    out->set(i, 1, area * expTerm / (sigmaTerm * sigma) * (diff * diff - 1.0));
    out->set(i, 2, expTerm / sigmaTerm);
  }
}

void AreaGaussian::init() {
  declareParameter("Centre");
  declareParameter("Sigma");
  declareParameter("Area");
}

} // namespace CurveFitting
} // namespace Mantid
