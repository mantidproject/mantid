// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"
#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidKernel/Exception.h"

#include <cctype>
#include <functional>
#include <map>

namespace Mantid::CurveFitting::Functions {

namespace {

// Maps ion name to its int code.
const std::map<std::string, int> ION_2_NRE{{"Ce", 1},  {"Pr", 2},  {"Nd", 3}, {"Pm", 4}, {"Sm", 5},
                                           {"Eu", 6},  {"Gd", 7},  {"Tb", 8}, {"Dy", 9}, {"Ho", 10},
                                           {"Er", 11}, {"Tm", 12}, {"Yb", 13}};

const bool REAL_PARAM_PART = true;
const bool IMAG_PARAM_PART = false;

void fixx(API::IFunction &fun, const std::string &par) {
  fun.setParameter(par, 0.0);
  fun.fixParameter(par);
  const std::string ipar = "I" + par;
  fun.setParameter(ipar, 0.0);
  fun.fixParameter(ipar);
}

void free(API::IFunction &fun, const std::string &par, bool realOnly) {
  fun.unfixParameter(par);
  const std::string ipar = "I" + par;
  if (realOnly) {
    fun.setParameter(ipar, 0.0);
    fun.fixParameter(ipar);
  } else {
    fun.unfixParameter(ipar);
  }
}

// Set symmetry C1 or Ci
void setSymmetryC1(API::IFunction &fun) {
  fun.clearTies();
  auto i = fun.parameterIndex("B20");
  for (; i < fun.nParams(); ++i) {
    fun.unfix(i);
  }
}

// Set symmetry C2, Cs or C2h
void setSymmetryC2(API::IFunction &fun) {
  fun.clearTies();
  fun.unfixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  free(fun, "B22", REAL_PARAM_PART);
  fixx(fun, "B41");
  free(fun, "B42", IMAG_PARAM_PART);
  fixx(fun, "B43");
  free(fun, "B44", IMAG_PARAM_PART);
  fixx(fun, "B61");
  free(fun, "B62", IMAG_PARAM_PART);
  fixx(fun, "B63");
  free(fun, "B64", IMAG_PARAM_PART);
  fixx(fun, "B65");
  free(fun, "B66", IMAG_PARAM_PART);
}

// Set symmetry C2v, D2 or D2h
void setSymmetryC2v(API::IFunction &fun) {
  fun.clearTies();
  fun.unfixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  free(fun, "B22", REAL_PARAM_PART);
  fixx(fun, "B41");
  free(fun, "B42", REAL_PARAM_PART);
  fixx(fun, "B43");
  free(fun, "B44", REAL_PARAM_PART);
  fixx(fun, "B61");
  free(fun, "B62", REAL_PARAM_PART);
  fixx(fun, "B63");
  free(fun, "B64", REAL_PARAM_PART);
  fixx(fun, "B65");
  free(fun, "B66", REAL_PARAM_PART);
}

// Set symmetry C4, S4 or C4h
void setSymmetryC4(API::IFunction &fun) {
  fun.clearTies();
  fun.unfixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  fixx(fun, "B22");
  fixx(fun, "B41");
  fixx(fun, "B42");
  fixx(fun, "B43");
  free(fun, "B44", REAL_PARAM_PART);
  fixx(fun, "B61");
  fixx(fun, "B62");
  fixx(fun, "B63");
  free(fun, "B64", IMAG_PARAM_PART);
  fixx(fun, "B65");
  fixx(fun, "B66");
}

// Set symmetry D4, C4v, D2d or D4h
void setSymmetryD4(API::IFunction &fun) {
  fun.clearTies();
  fun.unfixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  fixx(fun, "B22");
  fixx(fun, "B41");
  fixx(fun, "B42");
  fixx(fun, "B43");
  free(fun, "B44", REAL_PARAM_PART);
  fixx(fun, "B61");
  fixx(fun, "B62");
  fixx(fun, "B63");
  free(fun, "B64", REAL_PARAM_PART);
  fixx(fun, "B65");
  fixx(fun, "B66");
}

// Set symmetry C3 or S6
void setSymmetryC3(API::IFunction &fun) {
  fun.clearTies();
  fun.unfixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  fixx(fun, "B22");
  fixx(fun, "B41");
  fixx(fun, "B42");
  free(fun, "B43", REAL_PARAM_PART);
  fixx(fun, "B44");
  fixx(fun, "B61");
  fixx(fun, "B62");
  free(fun, "B63", IMAG_PARAM_PART);
  fixx(fun, "B64");
  fixx(fun, "B65");
  free(fun, "B66", IMAG_PARAM_PART);
}

// Set symmetry D3, C3v or D3d
void setSymmetryD3(API::IFunction &fun) {
  fun.clearTies();
  fun.unfixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  fixx(fun, "B22");
  fixx(fun, "B41");
  fixx(fun, "B42");
  free(fun, "B43", REAL_PARAM_PART);
  fixx(fun, "B44");
  fixx(fun, "B61");
  fixx(fun, "B62");
  free(fun, "B63", REAL_PARAM_PART);
  fixx(fun, "B64");
  fixx(fun, "B65");
  free(fun, "B66", REAL_PARAM_PART);
}

// Set symmetry C6, C3h, C6h, D6, C6v, D3h, or D6h
void setSymmetryC6(API::IFunction &fun) {
  fun.clearTies();
  fun.unfixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  fixx(fun, "B22");
  fixx(fun, "B41");
  fixx(fun, "B42");
  fixx(fun, "B43");
  fixx(fun, "B44");
  fixx(fun, "B61");
  fixx(fun, "B62");
  fixx(fun, "B63");
  fixx(fun, "B64");
  fixx(fun, "B65");
  free(fun, "B66", REAL_PARAM_PART);
}

// Set symmetry T, Td, Th, O, or Oh
void setSymmetryT(API::IFunction &fun) {
  fun.clearTies();
  fun.setParameter("B20", 0.0);
  fun.fixParameter("B20");
  fun.unfixParameter("B40");
  fun.unfixParameter("B60");
  fixx(fun, "B21");
  fixx(fun, "B22");
  fixx(fun, "B41");
  fixx(fun, "B42");
  fixx(fun, "B43");
  free(fun, "B44", REAL_PARAM_PART);
  fixx(fun, "B61");
  fixx(fun, "B62");
  fixx(fun, "B63");
  free(fun, "B64", REAL_PARAM_PART);
  fixx(fun, "B65");
  fixx(fun, "B66");
  fun.tie("B44", "5*B40");
  fun.tie("B64", "-21*B60");
}

/// Maps symmetry group names to the symmetry setting functions
const std::map<std::string, std::function<void(API::IFunction &)>> SYMMETRY_MAP{
    // Set symmetry C1 or Ci
    {"C1", setSymmetryC1},
    {"Ci", setSymmetryC1},
    // Set symmetry C2, Cs or C2h
    {"C2", setSymmetryC2},
    {"Cs", setSymmetryC2},
    {"C2h", setSymmetryC2},
    // Set symmetry C2v, D2 or D2h
    {"C2v", setSymmetryC2v},
    {"D2", setSymmetryC2v},
    {"D2h", setSymmetryC2v},
    // Set symmetry C4, S4 or C4h
    {"C4", setSymmetryC4},
    {"S4", setSymmetryC4},
    {"C4h", setSymmetryC4},
    // Set symmetry D4, C4v, D2d or D4h
    {"D4", setSymmetryD4},
    {"C4v", setSymmetryD4},
    {"D2d", setSymmetryD4},
    {"D4h", setSymmetryD4},
    // Set symmetry C3 or S6
    {"C3", setSymmetryC3},
    {"S6", setSymmetryC3},
    // Set symmetry D3, C3v or D3d
    {"D3", setSymmetryD3},
    {"C3v", setSymmetryD3},
    {"D3d", setSymmetryD3},
    // Set symmetry C6, C3h, C6h, D6, C6v, D3h, or D6h
    {"C6", setSymmetryC6},
    {"C3h", setSymmetryC6},
    {"C6h", setSymmetryC6},
    {"D6", setSymmetryC6},
    {"C6v", setSymmetryC6},
    {"D3h", setSymmetryC6},
    {"D6h", setSymmetryC6},
    // Set symmetry T, Td, Th, O, or Oh
    {"T", setSymmetryT},
    {"Td", setSymmetryT},
    {"Th", setSymmetryT},
    {"O", setSymmetryT},
    {"Oh", setSymmetryT}};

} // anonymous namespace

/// Constructor
CrystalFieldPeaksBase::CrystalFieldPeaksBase() : API::ParamFunction(), m_defaultDomainSize(0) {

  declareAttribute("Ion", Attribute("Ce"));
  declareAttribute("Symmetry", Attribute("Ci"));
  declareAttribute("ToleranceEnergy", Attribute(1.0e-10));
  declareAttribute("ToleranceIntensity", Attribute(1.0e-1));
  declareAttribute("MaxPeakCount", Attribute(0));

  declareParameter("BmolX", 0.0, "The x-component of the molecular field.");
  declareParameter("BmolY", 0.0, "The y-component of the molecular field.");
  declareParameter("BmolZ", 0.0, "The z-component of the molecular field.");

  declareParameter("BextX", 0.0, "The x-component of the external field.");
  declareParameter("BextY", 0.0, "The y-component of the external field.");
  declareParameter("BextZ", 0.0, "The z-component of the external field.");

  declareParameter("B20", 0.0, "Real part of the B20 field parameter.");
  declareParameter("B21", 0.0, "Real part of the B21 field parameter.");
  declareParameter("B22", 0.0, "Real part of the B22 field parameter.");
  declareParameter("B40", 0.0, "Real part of the B40 field parameter.");
  declareParameter("B41", 0.0, "Real part of the B41 field parameter.");
  declareParameter("B42", 0.0, "Real part of the B42 field parameter.");
  declareParameter("B43", 0.0, "Real part of the B43 field parameter.");
  declareParameter("B44", 0.0, "Real part of the B44 field parameter.");
  declareParameter("B60", 0.0, "Real part of the B60 field parameter.");
  declareParameter("B61", 0.0, "Real part of the B61 field parameter.");
  declareParameter("B62", 0.0, "Real part of the B62 field parameter.");
  declareParameter("B63", 0.0, "Real part of the B63 field parameter.");
  declareParameter("B64", 0.0, "Real part of the B64 field parameter.");
  declareParameter("B65", 0.0, "Real part of the B65 field parameter.");
  declareParameter("B66", 0.0, "Real part of the B66 field parameter.");

  declareParameter("IB21", 0.0, "Imaginary part of the B21 field parameter.");
  declareParameter("IB22", 0.0, "Imaginary part of the B22 field parameter.");
  declareParameter("IB41", 0.0, "Imaginary part of the B41 field parameter.");
  declareParameter("IB42", 0.0, "Imaginary part of the B42 field parameter.");
  declareParameter("IB43", 0.0, "Imaginary part of the B43 field parameter.");
  declareParameter("IB44", 0.0, "Imaginary part of the B44 field parameter.");
  declareParameter("IB61", 0.0, "Imaginary part of the B61 field parameter.");
  declareParameter("IB62", 0.0, "Imaginary part of the B62 field parameter.");
  declareParameter("IB63", 0.0, "Imaginary part of the B63 field parameter.");
  declareParameter("IB64", 0.0, "Imaginary part of the B64 field parameter.");
  declareParameter("IB65", 0.0, "Imaginary part of the B65 field parameter.");
  declareParameter("IB66", 0.0, "Imaginary part of the B66 field parameter.");

  setSymmetryC1(*this);
}

/// Calculate the crystal field eigensystem
/// @param en :: Output eigenvalues.
/// @param wf :: Output eigenvectors.
/// @param ham :: Output crystal field hamiltonian (without external field)
/// @param hz :: Output Zeeman hamiltonian (external field term)
/// @param nre :: Output ion code.
void CrystalFieldPeaksBase::calculateEigenSystem(DoubleFortranVector &en, ComplexFortranMatrix &wf,
                                                 ComplexFortranMatrix &ham, ComplexFortranMatrix &hz, int &nre) const {

  auto ion = getAttribute("Ion").asString();
  if (ion.empty()) {
    throw std::runtime_error("Ion name must be specified.");
  }

  auto ionIter = ION_2_NRE.find(ion);
  if (ionIter == ION_2_NRE.end()) {
    // If 'Ion=S2', or 'Ion=J2.5' etc, interpret as arbitrary J values with gJ=2
    // Allow lower case, but values must be half-integral. E.g. 'Ion=S2.4' fails
    switch (ion[0]) {
    case 'S':
    case 's':
    case 'J':
    case 'j': {
      if (ion.size() > 1 && std::isdigit(static_cast<unsigned char>(ion[1]))) {
        // Need to store as 2J to allow half-integer values
        try {
          auto J2 = std::stof(ion.substr(1)) * 2.;
          if (J2 > 99.) {
            throw std::out_of_range("");
          }
          if (fabs(J2 - (int)J2) < 0.001) {
            nre = -(int)J2;
            break;
          }
          // Catch exceptions thrown by stof so we get a more meaningful error
        } catch (const std::invalid_argument &) {
          throw std::runtime_error("Invalid value '" + ion.substr(1) + "' of J passed to CrystalFieldPeaks.");
        } catch (const std::out_of_range &) {
          throw std::runtime_error("Value of J: '" + ion.substr(1) + "' passed to CrystalFieldPeaks is too big.");
        }
      }
      // fall through
    }
    default:
      throw std::runtime_error("Unknown ion name '" + ion + "' passed to CrystalFieldPeaks.");
    }
  } else {
    nre = ionIter->second;
  }

  DoubleFortranVector bmol(1, 3);
  bmol(1) = getParameter("BmolX");
  bmol(2) = getParameter("BmolY");
  bmol(3) = getParameter("BmolZ");

  // For CrystalFieldSusceptibility and CrystalFieldMagnetisation we need
  //   to be able to override the external field set here, since in these
  //   measurements, a different external field is applied.
  DoubleFortranVector bext(1, 3);
  bext(1) = getParameter("BextX");
  bext(2) = getParameter("BextY");
  bext(3) = getParameter("BextZ");

  double B20 = getParameter("B20");
  double B21 = getParameter("B21");
  double B22 = getParameter("B22");
  double B40 = getParameter("B40");
  double B41 = getParameter("B41");
  double B42 = getParameter("B42");
  double B43 = getParameter("B43");
  double B44 = getParameter("B44");
  double B60 = getParameter("B60");
  double B61 = getParameter("B61");
  double B62 = getParameter("B62");
  double B63 = getParameter("B63");
  double B64 = getParameter("B64");
  double B65 = getParameter("B65");
  double B66 = getParameter("B66");

  double IB21 = getParameter("IB21");
  double IB22 = getParameter("IB22");
  double IB41 = getParameter("IB41");
  double IB42 = getParameter("IB42");
  double IB43 = getParameter("IB43");
  double IB44 = getParameter("IB44");
  double IB61 = getParameter("IB61");
  double IB62 = getParameter("IB62");
  double IB63 = getParameter("IB63");
  double IB64 = getParameter("IB64");
  double IB65 = getParameter("IB65");
  double IB66 = getParameter("IB66");

  ComplexFortranMatrix bkq(0, 6, 0, 6);
  bkq(2, 0) = ComplexType(B20, 0.0);
  bkq(2, 1) = ComplexType(B21, IB21);
  bkq(2, 2) = ComplexType(B22, IB22);
  bkq(4, 0) = ComplexType(B40, 0.0);
  bkq(4, 1) = ComplexType(B41, IB41);
  bkq(4, 2) = ComplexType(B42, IB42);
  bkq(4, 3) = ComplexType(B43, IB43);
  bkq(4, 4) = ComplexType(B44, IB44);
  bkq(6, 0) = ComplexType(B60, 0.0);
  bkq(6, 1) = ComplexType(B61, IB61);
  bkq(6, 2) = ComplexType(B62, IB62);
  bkq(6, 3) = ComplexType(B63, IB63);
  bkq(6, 4) = ComplexType(B64, IB64);
  bkq(6, 5) = ComplexType(B65, IB65);
  bkq(6, 6) = ComplexType(B66, IB66);

  calculateEigensystem(en, wf, ham, hz, nre, bmol, bext, bkq);
  // MaxPeakCount is a read-only "mutable" attribute.
  const_cast<CrystalFieldPeaksBase *>(this)->setAttributeValue("MaxPeakCount", static_cast<int>(en.size()));
}

/// Perform a castom action when an attribute is set.
void CrystalFieldPeaksBase::setAttribute(const std::string &name, const IFunction::Attribute &attr) {
  if (name == "Symmetry") {
    auto symmIter = SYMMETRY_MAP.find(attr.asString());
    if (symmIter == SYMMETRY_MAP.end()) {
      throw std::runtime_error("Unknown symmetry passed to CrystalFieldPeaks: " + attr.asString());
    }
    symmIter->second(*this);
  }
  IFunction::setAttribute(name, attr);
}

std::string CrystalFieldPeaksBaseImpl::name() const { return "CrystalFieldPeaksBaseImpl"; }

void CrystalFieldPeaksBaseImpl::function(const API::FunctionDomain & /*domain*/,
                                         API::FunctionValues & /*values*/) const {
  throw Kernel::Exception::NotImplementedError("Method is intentionally not implemented.");
}

} // namespace Mantid::CurveFitting::Functions
