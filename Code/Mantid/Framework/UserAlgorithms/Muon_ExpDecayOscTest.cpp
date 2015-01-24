//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "Muon_ExpDecayOscTest.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Muon_ExpDecayOscTest)

void Muon_ExpDecayOscTest::init() {
  declareParameter("A", 0.2);
  declareParameter("lambda", 0.2);
  declareParameter("frequency", 0.5);
  declareParameter("phi", 0.0);
}

void Muon_ExpDecayOscTest::functionLocal(double *out, const double *xValues,
                                         const size_t nData) const {
  const double &gA0 = getParameter("A");
  const double &gs = getParameter("lambda");
  const double &gf = getParameter("frequency");
  const double &gphi = getParameter("phi");

  for (size_t i = 0; i < nData; i++) {
    out[i] = gA0 * exp(-gs * xValues[i]) *
             cos(2 * 3.1415926536 * gf * xValues[i] + gphi);
  }
}
void Muon_ExpDecayOscTest::functionDeriv(const API::FunctionDomain &domain,
                                         API::Jacobian &out) {
  calNumericalDeriv(domain, out);
}

} // namespace CurveFitting
} // namespace Mantid
