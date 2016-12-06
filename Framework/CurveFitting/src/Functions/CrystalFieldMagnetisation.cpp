#include "MantidCurveFitting/Functions/CrystalFieldMagnetisation.h"
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
#include <iostream>
#include <boost/algorithm/string/predicate.hpp>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

namespace {

// Does the actual calculation of the magnetisation
void calculate(double *out, const double *xValues, const size_t nData,
               const ComplexFortranMatrix &ham, const int nre,
               const DoubleFortranVector Hmag, const double T,
               const double convfact) {
  const double beta = 1 / (PhysicalConstants::BoltzmannConstant * T);
  // x-data is the applied field magnitude. We need to recalculate
  // the Zeeman term and diagonalise the Hamiltonian at each x-point.
  int nlevels = ham.len1();
  for (size_t iH = 0; iH < nData; iH++) {
    DoubleFortranVector en;
    ComplexFortranMatrix ev;
    DoubleFortranVector H = Hmag;
    H *= xValues[iH];
    calculateZeemanEigensystem(en, ev, ham, nre, H);
    // Calculates the diagonal of the magnetic moment operator <wf|mu|wf>
    DoubleFortranVector moment;
    double gJ;
    calculateMagneticMoment(ev, Hmag, nre, gJ, moment);
    double expfact;
    double Z = 0.;
    double M = 0.; 
    for (auto iE = 1; iE <= nlevels; iE++) {
      expfact = exp(-beta * en(iE));
      Z += expfact;
      M += moment(iE) * expfact;
    }
    out[iH] = convfact * gJ * M / Z;
  }
}

}

DECLARE_FUNCTION(CrystalFieldMagnetisation)

CrystalFieldMagnetisation::CrystalFieldMagnetisation()
    : CrystalFieldPeaksBase(), API::IFunction1D(), setDirect(false) {
  declareAttribute("Hdir", Attribute(std::vector<double>{0., 0., 1.}));
  declareAttribute("Temperature", Attribute(1.0));
  declareAttribute("Unit", Attribute("bohr")); // others = "SI", "cgs"
}

// Sets the base crystal field Hamiltonian matrix
void CrystalFieldMagnetisation::set_hamiltonian(const ComplexFortranMatrix &ham_in,
                                                const int nre_in) {
  setDirect = true;
  ham = ham_in;
  nre = nre_in;
}

void CrystalFieldMagnetisation::function1D(double *out,
                                           const double *xValues,
                                           const size_t nData) const {
  // Get the field direction
  auto Hdir = getAttribute("Hdir").asVector();
  auto T = getAttribute("Temperature").asDouble();
  double Hnorm = sqrt(Hdir[0]*Hdir[0] + Hdir[1]*Hdir[1] + Hdir[2]*Hdir[2]);
  DoubleFortranVector H(1, 3);
  for (auto i = 0; i < 3; i++) {
    H(i+1) = Hdir[i] / Hnorm;
  }
  auto unit = getAttribute("Unit").asString();
  const double NAMUB = 5.5849397;  // N_A*mu_B - J/T/mol
  // Converts to different units - SI is in J/T/mol or Am^2/mol. cgs in emu/mol.
  // NB. Atomic ("bohr") units gives magnetisation in uB/ion, but other units
  // give the molar magnetisation.
  double convfact = boost::iequals(unit, "SI") ? NAMUB : 
                    (boost::iequals(unit, "cgs") ? NAMUB*1000. : 1.);
  if (!setDirect) {
    // Because this method is const, we can't change the stored en / wf
    // Use temporary variables instead.
    DoubleFortranVector en_;
    ComplexFortranMatrix wf_;
    ComplexFortranMatrix ham_;
    ComplexFortranMatrix hz_;
    int nre_ = 0; 
    calculateEigenSystem(en_, wf_, ham_, hz_, nre_);
    ham_ += hz_;
    calculate(out, xValues, nData, ham_, nre_, H, T, convfact);
  }
  else {
    // Use stored values
    calculate(out, xValues, nData, ham, nre, H, T, convfact);
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

