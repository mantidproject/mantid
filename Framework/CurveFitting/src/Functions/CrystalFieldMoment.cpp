#include "MantidCurveFitting/Functions/CrystalFieldMoment.h"
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

// Does the actual calculation of the magnetic moment
void calculate(double *out, const double *xValues, const size_t nData,
               const ComplexFortranMatrix &ham, const int nre,
               const DoubleFortranVector Hdir, const double Hmag,
               const double convfact) {
  const double k_B = PhysicalConstants::BoltzmannConstant;
  DoubleFortranVector en;
  ComplexFortranMatrix ev;
  DoubleFortranVector H = Hdir;
  H *= Hmag;
  calculateZeemanEigensystem(en, ev, ham, nre, H);
  // Calculates the diagonal of the magnetic moment operator <wf|mu|wf>
  DoubleFortranVector moment;
  calculateMagneticMoment(ev, Hdir, nre, moment);
  int nlevels = ham.len1();
  // x-data is the temperature.
  for (size_t iT = 0; iT < nData; iT++) {
    double expfact;
    double Z = 0.;
    double M = 0.; 
    const double beta = 1 / (k_B * xValues[iT]);
    for (auto iE = 1; iE <= nlevels; iE++) {
      expfact = exp(-beta * en(iE));
      Z += expfact;
      M += moment(iE) * expfact;
    }
    out[iT] = convfact * M / Z;
  }
}

// Powder averaging - sum over calculations along x, y, z
void calculate_powder(double *out, const double *xValues, const size_t nData,
                      const ComplexFortranMatrix &ham, const int nre,
                      const double Hmag, const double convfact) {
  for (size_t j = 0; j < nData; j++) {
    out[j] = 0.;
  }
  DoubleFortranVector Hd(1, 3);
  std::vector<double> tmp(nData, 0.);
  for (int i = 1; i <= 3; i++) {
    Hd.zero();
    Hd(i) = 1.;
    calculate(&tmp[0], xValues, nData, ham, nre, Hd, Hmag, convfact);
    for (size_t j = 0; j < nData; j++) {
      out[j] += tmp[j];
    }
  }
  for (size_t j = 0; j < nData; j++) {
    out[j] /= 3.;
  }
}

}

DECLARE_FUNCTION(CrystalFieldMoment)

CrystalFieldMoment::CrystalFieldMoment()
    : CrystalFieldPeaksBase(), API::IFunction1D(), setDirect(false) {
  declareAttribute("Hdir", Attribute(std::vector<double>{0., 0., 1.}));
  declareAttribute("Hmag", Attribute(1.0));
  declareAttribute("Unit", Attribute("bohr")); // others = "SI", "cgs"
  declareAttribute("inverse", Attribute(false));
  declareAttribute("powder", Attribute(false));
}

// Sets the base crystal field Hamiltonian matrix
void CrystalFieldMoment::set_hamiltonian(const ComplexFortranMatrix &ham_in,
                                         const int nre_in) {
  setDirect = true;
  ham = ham_in;
  nre = nre_in;
}

void CrystalFieldMoment::function1D(double *out,
                                    const double *xValues,
                                    const size_t nData) const {
  // Get the field direction
  auto Hdir = getAttribute("Hdir").asVector();
  auto Hmag = getAttribute("Hmag").asDouble();
  auto powder = getAttribute("powder").asBool();
  double Hnorm = sqrt(Hdir[0]*Hdir[0] + Hdir[1]*Hdir[1] + Hdir[2]*Hdir[2]);
  DoubleFortranVector H(1, 3);
  for (auto i = 0; i < 3; i++) {
    H(i+1) = Hdir[i] / Hnorm;
  }
  auto unit = getAttribute("Unit").asString();
  const double NAMUB = 5.5849397;  // N_A*mu_B - J/T/mol
  // Converts to different units - SI is in J/T/mol or Am^2/mol. cgs in emu/mol.
  // NB. Atomic ("bohr") units gives magnetisation in bohr magneton/ion, but
  // other units give the molar magnetisation.
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
    if (powder) {
      calculate_powder(out, xValues, nData, ham_, nre_, Hmag, convfact);
    } else {
      calculate(out, xValues, nData, ham_, nre_, H, Hmag, convfact);
    }
  }
  else {
    // Use stored values
    if (powder) {
      calculate_powder(out, xValues, nData, ham, nre, Hmag, convfact);
    } else {
      calculate(out, xValues, nData, ham, nre, H, Hmag, convfact);
    }
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

