#include "MantidCurveFitting/Functions/CrystalFieldHeatCapacity.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

namespace {

// Does the actual calculation of the heat capacity
void calculate(double *out, const double *xValues, const size_t nData,
               const DoubleFortranVector &en) {
  const double k_B = PhysicalConstants::BoltzmannConstant; // in meV/K
  // Want output in J/K/mol
  const double convfact = PhysicalConstants::N_A * PhysicalConstants::meV;
  int nlevels = en.len();
  for (size_t iT = 0; iT < nData; iT++) {
    double Z = 0.;
    double U = 0.;
    double U2 = 0.;
    const double beta = 1 / (k_B * xValues[iT]);
    // Using fortran indexing...
    for (auto iE = 1; iE <= nlevels; iE++) {
      double expfact = exp(-beta * en(iE));
      Z += expfact;
      U += en(iE) * expfact;
      U2 += en(iE) * en(iE) * expfact;
    }
    U /= Z;
    U2 /= Z;
    out[iT] = ((U2 - U * U) / (k_B * xValues[iT] * xValues[iT])) * convfact;
  }
}
}

DECLARE_FUNCTION(CrystalFieldHeatCapacity)

CrystalFieldHeatCapacity::CrystalFieldHeatCapacity()
    : CrystalFieldPeaksBase(), API::IFunction1D(), m_setDirect(false) {
  declareAttribute("ScaleFactor", Attribute(1.0)); // Only for multi-site use
}

// Sets the eigenvectors / values directly
void CrystalFieldHeatCapacity::setEnergy(const DoubleFortranVector &en) {
  m_setDirect = true;
  m_en = en;
}

void CrystalFieldHeatCapacity::function1D(double *out, const double *xValues,
                                          const size_t nData) const {
  if (!m_setDirect) {
    // Because this method is const, we can't change the stored en / wf
    // Use temporary variables instead.
    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    int nre = 0;
    calculateEigenSystem(en, wf, nre);
    calculate(out, xValues, nData, en);
  } else {
    // Use stored values
    calculate(out, xValues, nData, m_en);
  }
  auto fact = getAttribute("ScaleFactor").asDouble();
  if (fact != 1.0) {
    for (size_t i = 0; i < nData; i++) {
      out[i] *= fact;
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
