#include "MantidCurveFitting/Functions/CrystalFieldMagnetisation.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/Jacobian.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"
#include <boost/algorithm/string/predicate.hpp>
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

namespace {

// Does the actual calculation of the magnetisation
void calculate(double *out, const double *xValues, const size_t nData,
               const ComplexFortranMatrix &ham, const int nre,
               const DoubleFortranVector Hmag, const double T,
               const double convfact, const bool iscgs) {
  const double beta = 1 / (PhysicalConstants::BoltzmannConstant * T);
  // x-data is the applied field magnitude. We need to recalculate
  // the Zeeman term and diagonalise the Hamiltonian at each x-point.
  int nlevels = ham.len1();
  for (size_t iH = 0; iH < nData; iH++) {
    DoubleFortranVector en;
    ComplexFortranMatrix ev;
    DoubleFortranVector H = Hmag;
    H *= xValues[iH];
    if (iscgs) {
      H *= 0.0001; // Converts from Gauss to Tesla.
    }
    calculateZeemanEigensystem(en, ev, ham, nre, H);
    // Calculates the diagonal of the magnetic moment operator <wf|mu|wf>
    DoubleFortranVector moment;
    calculateMagneticMoment(ev, Hmag, nre, moment);
    double Z = 0.;
    double M = 0.;
    for (auto iE = 1; iE <= nlevels; iE++) {
      double expfact = exp(-beta * en(iE));
      Z += expfact;
      M += moment(iE) * expfact;
    }
    out[iH] = convfact * M / Z;
  }
}

// Calculate powder average - Mpowder = (Mx + My + Mz)/3
void calculate_powder(double *out, const double *xValues, const size_t nData,
                      const ComplexFortranMatrix &ham, const int nre,
                      const double T, const double convfact, const bool cgs) {
  for (size_t j = 0; j < nData; j++) {
    out[j] = 0.;
  }
  // Loop over the x, y, z directions
  DoubleFortranVector Hmag(1, 3);
  std::vector<double> tmp(nData, 0.);
  for (int i = 1; i <= 3; i++) {
    Hmag.zero();
    Hmag(i) = 1.;
    calculate(&tmp[0], xValues, nData, ham, nre, Hmag, T, convfact, cgs);
    for (size_t j = 0; j < nData; j++) {
      out[j] += tmp[j];
    }
  }
  for (size_t j = 0; j < nData; j++) {
    out[j] /= 3.;
  }
}
} // namespace

CrystalFieldMagnetisationBase::CrystalFieldMagnetisationBase()
    : API::IFunction1D(), m_nre(0) {
  declareAttribute("Hdir", Attribute(std::vector<double>{0., 0., 1.}));
  declareAttribute("Temperature", Attribute(1.0));
  declareAttribute("Unit", Attribute("bohr")); // others = "SI", "cgs"
  declareAttribute("powder", Attribute(false));
  declareAttribute("ScaleFactor", Attribute(1.0)); // Only for multi-site use
}

void CrystalFieldMagnetisationBase::function1D(double *out,
                                               const double *xValues,
                                               const size_t nData) const {
  // Get the field direction
  auto Hdir = getAttribute("Hdir").asVector();
  if (Hdir.size() != 3) {
    throw std::invalid_argument("Hdir must be a three-element vector.");
  }
  auto T = getAttribute("Temperature").asDouble();
  auto powder = getAttribute("powder").asBool();
  double Hnorm =
      sqrt(Hdir[0] * Hdir[0] + Hdir[1] * Hdir[1] + Hdir[2] * Hdir[2]);
  DoubleFortranVector H(1, 3);
  if (fabs(Hnorm) > 1.e-6) {
    for (auto i = 0; i < 3; i++) {
      H(i + 1) = Hdir[i] / Hnorm;
    }
  }
  auto unit = getAttribute("Unit").asString();
  const double NAMUB = 5.5849397; // N_A*mu_B - J/T/mol
  // Converts to different units - SI is in J/T/mol or Am^2/mol.
  // cgs is in erg/Gauss/mol (emu/mol). The value of uB in erg/G is 1000x in J/T
  // NB. Atomic ("bohr") units gives magnetisation in uB/ion, but other units
  // give the molar magnetisation.
  double convfact = boost::iequals(unit, "SI")
                        ? NAMUB
                        : (boost::iequals(unit, "cgs") ? NAMUB * 1000. : 1.);
  const bool iscgs = boost::iequals(unit, "cgs");
  // Use stored values
  if (powder) {
    calculate_powder(out, xValues, nData, m_ham, m_nre, T, convfact, iscgs);
  } else {
    calculate(out, xValues, nData, m_ham, m_nre, H, T, convfact, iscgs);
  }
  auto fact = getAttribute("ScaleFactor").asDouble();
  if (fact != 1.0) {
    for (size_t i = 0; i < nData; i++) {
      out[i] *= fact;
    }
  }
}

DECLARE_FUNCTION(CrystalFieldMagnetisation)

CrystalFieldMagnetisation::CrystalFieldMagnetisation()
    : CrystalFieldPeaksBase(), CrystalFieldMagnetisationBase(),
      m_setDirect(false) {}

// Sets the base crystal field Hamiltonian matrix
void CrystalFieldMagnetisation::setHamiltonian(const ComplexFortranMatrix &ham,
                                               const int nre) {
  m_setDirect = true;
  m_ham = ham;
  m_nre = nre;
}

void CrystalFieldMagnetisation::function1D(double *out, const double *xValues,
                                           const size_t nData) const {
  if (!m_setDirect) {
    DoubleFortranVector en;
    ComplexFortranMatrix wf;
    ComplexFortranMatrix hz;
    calculateEigenSystem(en, wf, m_ham, hz, m_nre);
    m_ham += hz;
  }
  CrystalFieldMagnetisationBase::function1D(out, xValues, nData);
}

CrystalFieldMagnetisationCalculation::CrystalFieldMagnetisationCalculation()
    : API::ParamFunction(), CrystalFieldMagnetisationBase() {}

// Sets the base crystal field Hamiltonian matrix
void CrystalFieldMagnetisationCalculation::setHamiltonian(
    const ComplexFortranMatrix &ham, const int nre) {
  m_ham = ham;
  m_nre = nre;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
