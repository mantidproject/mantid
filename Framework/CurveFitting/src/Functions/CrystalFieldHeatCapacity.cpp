#include "MantidCurveFitting/Functions/CrystalFieldHeatCapacity.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"
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
} // namespace

CrystalFieldHeatCapacityBase::CrystalFieldHeatCapacityBase()
    : API::IFunction1D() {
  declareAttribute("ScaleFactor", Attribute(1.0));
}

void CrystalFieldHeatCapacityBase::function1D(double *out,
                                              const double *xValues,
                                              const size_t nData) const {
  // Use stored values
  calculate(out, xValues, nData, m_en);
  auto fact = getAttribute("ScaleFactor").asDouble();
  if (fact != 1.0) {
    for (size_t i = 0; i < nData; i++) {
      out[i] *= fact;
    }
  }
}

DECLARE_FUNCTION(CrystalFieldHeatCapacity)

CrystalFieldHeatCapacity::CrystalFieldHeatCapacity()
    : CrystalFieldPeaksBase(), CrystalFieldHeatCapacityBase(),
      m_setDirect(false) {}

// Sets the eigenvectors / values directly
void CrystalFieldHeatCapacity::setEnergy(const DoubleFortranVector &en) {
  m_setDirect = true;
  m_en = en;
}

void CrystalFieldHeatCapacity::function1D(double *out, const double *xValues,
                                          const size_t nData) const {
  if (!m_setDirect) {
    ComplexFortranMatrix wf;
    int nre = 0;
    calculateEigenSystem(m_en, wf, nre);
  }
  CrystalFieldHeatCapacityBase::function1D(out, xValues, nData);
}

CrystalFieldHeatCapacityCalculation::CrystalFieldHeatCapacityCalculation()
    : API::ParamFunction(), CrystalFieldHeatCapacityBase() {}

// Sets the eigenvectors / values directly
void CrystalFieldHeatCapacityCalculation::setEnergy(
    const DoubleFortranVector &en) {
  m_en = en;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
