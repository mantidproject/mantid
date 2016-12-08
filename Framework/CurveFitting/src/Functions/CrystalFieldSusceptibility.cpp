#include "MantidCurveFitting/Functions/CrystalFieldSusceptibility.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"
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
#include <boost/algorithm/string/predicate.hpp>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

namespace {

// Get a complex conjugate of the value returned by
// ComplexMatrix::operator(i,j)
ComplexType conj(const ComplexMatrixValueConverter &conv) {
  return std::conj(static_cast<ComplexType>(conv));
}

// Does the actual calculation of the susceptibility
void calculate(double *out, const double *xValues, const size_t nData,
               const DoubleFortranVector &en, const ComplexFortranMatrix &wf, 
               const int nre, const std::vector<double> &H) {
  // Some constants
  const double k_B = PhysicalConstants::BoltzmannConstant; // in meV/K
  const double eps = 1.e-6; // for degeneracy calculations
  // First calculates the matrix elements of the magnetic moment operator
  ComplexFortranMatrix mumat;
  calculateMagneticMomentMatrix(wf, H, nre, mumat);
  int nlevels = en.len();
  DoubleFortranVector mu(nlevels);
  DoubleFortranVector mu2(nlevels);
  mu.zero();
  mu2.zero();
  for (auto i = 1; i <= nlevels; i++) {
    for (auto j = 1; j <= nlevels; j++) {
      const double den = en(i) - en(j);
      if (fabs(den) < eps) {  // First order term
        mu(i) += real(mumat(i,j) * conj(mumat(i,j)));
      } else { // Second order term
        mu2(i) += real(mumat(i,j) * conj(mumat(i,j))) / den;
      }
    }
  }
  // Now calculates the temperature dependence.
  for (size_t iT = 0; iT < nData; iT++) {
    double expfact; 
    double Z = 0.;
    double U = 0.;
    const double beta = 1 / (k_B * xValues[iT]);
    for (auto i = 1; i <= nlevels; i++) {
      expfact = exp(-beta * en(i));
      Z += expfact;
      U += ((mu(i) * beta) - (2 * mu2(i))) * expfact;
    }
    out[iT] = U / Z;
  }
}

}

DECLARE_FUNCTION(CrystalFieldSusceptibility)

CrystalFieldSusceptibility::CrystalFieldSusceptibility()
    : CrystalFieldPeaksBase(), API::IFunction1D(), setDirect(false) {
  declareAttribute("Hdir", Attribute(std::vector<double>{0., 0., 1.}));
  declareAttribute("Unit", Attribute("bohr"));
  declareAttribute("inverse", Attribute(false));
}

// Sets the eigenvectors / values directly
void CrystalFieldSusceptibility::set_eigensystem(const DoubleFortranVector &en_in,
                                                 const ComplexFortranMatrix &wf_in,
                                                 const int nre_in) {
  setDirect = true;
  en = en_in;
  wf = wf_in;
  nre = nre_in;
}

void CrystalFieldSusceptibility::function1D(double *out,
                                          const double *xValues,
                                          const size_t nData) const {
  auto H = getAttribute("Hdir").asVector();
  double Hnorm = sqrt(H[0]*H[0] + H[1]*H[1] + H[2]*H[2]);
  for (auto i = 0; i < 3; i++) {
    H[i] /= Hnorm;
  }
  if (!setDirect) {
    // Because this method is const, we can't change the stored en / wf
    // Use temporary variables instead.
    DoubleFortranVector en_;
    ComplexFortranMatrix wf_;
    int nre_ = 0; 
    calculateEigenSystem(en_, wf_, nre_);
    calculate(out, xValues, nData, en_, wf_, nre_, H);
  }
  else {
    // Use stored values
    calculate(out, xValues, nData, en, wf, nre, H);
  }
  if (getAttribute("inverse").asBool()) {
    for (size_t i = 0; i < nData; i++) {
      out[i] = 1. / out[i];
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

