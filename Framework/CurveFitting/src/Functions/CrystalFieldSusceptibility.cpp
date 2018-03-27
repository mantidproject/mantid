#include "MantidCurveFitting/Functions/CrystalFieldSusceptibility.h"
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

// Get a complex conjugate of the value returned by
// ComplexMatrix::operator(i,j)
ComplexType conj(const ComplexMatrixValueConverter &conv) {
  return std::conj(static_cast<ComplexType>(conv));
}

// Does the actual calculation of the susceptibility
void calculate(double *out, const double *xValues, const size_t nData,
               const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
               const int nre, const std::vector<double> &H,
               const double convfact) {
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
      if (fabs(den) < eps) { // First order term
        mu(i) += real(mumat(i, j) * conj(mumat(i, j)));
      } else { // Second order term
        mu2(i) += real(mumat(i, j) * conj(mumat(i, j))) / den;
      }
    }
  }
  // Now calculates the temperature dependence.
  mu *= convfact;
  mu2 *= convfact;
  for (size_t iT = 0; iT < nData; iT++) {
    double Z = 0.;
    double U = 0.;
    const double beta = 1 / (k_B * xValues[iT]);
    for (auto i = 1; i <= nlevels; i++) {
      double expfact = exp(-beta * en(i));
      Z += expfact;
      U += ((mu(i) * beta) - (2 * mu2(i))) * expfact;
    }
    out[iT] = U / Z;
  }
}

// Calculate powder average - Mpowder = (Mx + My + Mz)/3
void calculate_powder(double *out, const double *xValues, const size_t nData,
                      const DoubleFortranVector &en,
                      const ComplexFortranMatrix &wf, const int nre,
                      const double convfact) {
  for (size_t j = 0; j < nData; j++) {
    out[j] = 0.;
  }
  // Loop over the x, y, z directions
  std::vector<double> H;
  std::vector<double> tmp(nData, 0.);
  for (int i = 0; i < 3; i++) {
    H.assign(3, 0.);
    H[i] = 1.;
    calculate(&tmp[0], xValues, nData, en, wf, nre, H, convfact);
    for (size_t j = 0; j < nData; j++) {
      out[j] += tmp[j];
    }
  }
  for (size_t j = 0; j < nData; j++) {
    out[j] /= 3.;
  }
}
} // namespace

CrystalFieldSusceptibilityBase::CrystalFieldSusceptibilityBase() : m_nre(0) {
  // Declare attributes inherited by subclasses
  declareAttribute("Hdir", Attribute(std::vector<double>{0., 0., 1.}));
  declareAttribute("Unit", Attribute("cgs"));
  declareAttribute("inverse", Attribute(false));
  declareAttribute("powder", Attribute(false));
  declareAttribute("ScaleFactor", Attribute(1.0)); // Only for multi-site use
  // Cannot declare fitting parameters as this is not a ParamFunction
}

void CrystalFieldSusceptibilityBase::function1D(double *out,
                                                const double *xValues,
                                                const size_t nData) const {
  auto H = getAttribute("Hdir").asVector();
  if (H.size() != 3) {
    throw std::invalid_argument("Hdir must be a three-element vector.");
  }
  auto powder = getAttribute("powder").asBool();
  double Hnorm = sqrt(H[0] * H[0] + H[1] * H[1] + H[2] * H[2]);
  if (fabs(Hnorm) > 1.e-6) {
    for (auto i = 0; i < 3; i++) {
      H[i] /= Hnorm;
    }
  }
  // Get the unit conversion factor.
  auto unit = getAttribute("Unit").asString();
  const double NAMUB2cgs = 0.03232776; // N_A * muB(erg/G) * muB(meV/G)
  // The constant above is in strange units because we need the output
  // to be in erg/G^2/mol==cm^3/mol, but in the calculation all energies
  // are in meV and the expression for chi has a energy denominator.
  const double NAMUB2si = 4.062426e-7; // N_A * muB(J/T) * muB(meV/T) * mu0
  // Again, for SI units, we need to have one of the muB in meV not J.
  // The additional factor of mu0 is due to the different definitions of
  // the magnetisation, B- and H-fields in the SI and cgs systems.
  double convfact = boost::iequals(unit, "bohr")
                        ? 0.057883818
                        : (boost::iequals(unit, "SI") ? NAMUB2si : NAMUB2cgs);
  // Note the constant for "bohr" is the bohr magneton in meV/T, this will
  // give the susceptibility in "atomic" units of uB/T/ion.
  // Note that chi_SI = (4pi*10^-6)chi_cgs
  // Default unit is cgs (cm^3/mol).
  // Use stored values
  if (powder) {
    calculate_powder(out, xValues, nData, m_en, m_wf, m_nre, convfact);
  } else {
    calculate(out, xValues, nData, m_en, m_wf, m_nre, H, convfact);
  }
  const double lambda = getParameter("Lambda");
  const double EPS = 1.e-6;
  if (fabs(lambda) > EPS) {
    for (size_t i = 0; i < nData; i++) {
      out[i] /= (1. - lambda * out[i]); // chi = chi_cf/(1 - lambda.chi_cf)
    }
  }
  const double chi0 = getParameter("Chi0");
  if (fabs(lambda) > 1.e-6) {
    for (size_t i = 0; i < nData; i++) {
      out[i] += chi0;
    }
  }
  if (getAttribute("inverse").asBool()) {
    for (size_t i = 0; i < nData; i++) {
      out[i] = 1. / out[i];
    }
  }
  auto fact = getAttribute("ScaleFactor").asDouble();
  if (fact != 1.0) {
    for (size_t i = 0; i < nData; i++) {
      out[i] *= fact;
    }
  }
}

DECLARE_FUNCTION(CrystalFieldSusceptibility)

CrystalFieldSusceptibility::CrystalFieldSusceptibility()
    : CrystalFieldPeaksBase(), CrystalFieldSusceptibilityBase(),
      m_setDirect(false) {
  declareParameter("Lambda", 0.0, "Effective exchange interaction");
  declareParameter("Chi0", 0.0, "Background or remnant susceptibility");
}

// Sets the eigenvectors / values directly
void CrystalFieldSusceptibility::setEigensystem(const DoubleFortranVector &en,
                                                const ComplexFortranMatrix &wf,
                                                const int nre) {
  m_en = en;
  m_wf = wf;
  m_nre = nre;
  m_setDirect = true;
}

void CrystalFieldSusceptibility::function1D(double *out, const double *xValues,
                                            const size_t nData) const {
  if (!m_setDirect) {
    calculateEigenSystem(m_en, m_wf, m_nre);
  }
  CrystalFieldSusceptibilityBase::function1D(out, xValues, nData);
}

CrystalFieldSusceptibilityCalculation::CrystalFieldSusceptibilityCalculation() {
  declareParameter("Lambda", 0.0, "Effective exchange interaction");
  declareParameter("Chi0", 0.0, "Background or remnant susceptibility");
}

// Sets the eigenvectors / values directly
void CrystalFieldSusceptibilityCalculation::setEigensystem(
    const DoubleFortranVector &en, const ComplexFortranMatrix &wf,
    const int nre) {
  m_en = en;
  m_wf = wf;
  m_nre = nre;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
