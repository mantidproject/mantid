#include "MantidCurveFitting/Functions/CrystalElectricField.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"

#include <sstream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/// Constructor
CrystalFieldPeaks::CrystalFieldPeaks()
    : API::IFunctionGeneral(), API::ParamFunction() {
  declareAttribute("Nre", Attribute(0));
  declareAttribute("Temperature", Attribute(1.0));

  declareParameter("Bmol_x", 0.0, "The x-component of the molecular field.");
  declareParameter("Bmol_y", 0.0, "The y-component of the molecular field.");
  declareParameter("Bmol_z", 0.0, "The z-component of the molecular field.");

  declareParameter("Bext_x", 0.0, "The x-component of the external field.");
  declareParameter("Bext_y", 0.0, "The y-component of the external field.");
  declareParameter("Bext_z", 0.0, "The z-component of the external field.");

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

  declareParameter("IB20", 0.0, "Imaginary part of the B20 field parameter.");
  declareParameter("IB21", 0.0, "Imaginary part of the B21 field parameter.");
  declareParameter("IB22", 0.0, "Imaginary part of the B22 field parameter.");
  declareParameter("IB40", 0.0, "Imaginary part of the B40 field parameter.");
  declareParameter("IB41", 0.0, "Imaginary part of the B41 field parameter.");
  declareParameter("IB42", 0.0, "Imaginary part of the B42 field parameter.");
  declareParameter("IB43", 0.0, "Imaginary part of the B43 field parameter.");
  declareParameter("IB44", 0.0, "Imaginary part of the B44 field parameter.");
  declareParameter("IB60", 0.0, "Imaginary part of the B60 field parameter.");
  declareParameter("IB61", 0.0, "Imaginary part of the B61 field parameter.");
  declareParameter("IB62", 0.0, "Imaginary part of the B62 field parameter.");
  declareParameter("IB63", 0.0, "Imaginary part of the B63 field parameter.");
  declareParameter("IB64", 0.0, "Imaginary part of the B64 field parameter.");
  declareParameter("IB65", 0.0, "Imaginary part of the B65 field parameter.");
  declareParameter("IB66", 0.0, "Imaginary part of the B66 field parameter.");
}

std::string CrystalFieldPeaks::name() const { return "CrystalFieldPeaks"; }

size_t CrystalFieldPeaks::getNumberDomainColumns() const {return 0;}

size_t CrystalFieldPeaks::getNumberValuesPerArgument() const { return 2; }

void CrystalFieldPeaks::functionGeneral(
    const API::FunctionDomainGeneral &generalDomain,
    API::FunctionValues &values) const {

  int nre = getAttribute("Nre").asInt();

  DoubleFortranVector bmol(1, 3);
  bmol(1) = getParameter("Bmol_x");
  bmol(2) = getParameter("Bmol_y");
  bmol(3) = getParameter("Bmol_z");

  DoubleFortranVector bext(1, 3);
  bext(1) = getParameter("Bext_x");
  bext(2) = getParameter("Bext_y");
  bext(3) = getParameter("Bext_z");

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

  double IB20 = getParameter("IB20");
  double IB21 = getParameter("IB21");
  double IB22 = getParameter("IB22");
  double IB40 = getParameter("IB40");
  double IB41 = getParameter("IB41");
  double IB42 = getParameter("IB42");
  double IB43 = getParameter("IB43");
  double IB44 = getParameter("IB44");
  double IB60 = getParameter("IB60");
  double IB61 = getParameter("IB61");
  double IB62 = getParameter("IB62");
  double IB63 = getParameter("IB63");
  double IB64 = getParameter("IB64");
  double IB65 = getParameter("IB65");
  double IB66 = getParameter("IB66");

  ComplexFortranMatrix bkq(0, 6, 0, 6);
  bkq(2, 0) = ComplexType(B20, IB20);
  bkq(2, 1) = ComplexType(B21, IB21);
  bkq(2, 2) = ComplexType(B22, IB22);
  bkq(4, 0) = ComplexType(B40, IB40);
  bkq(4, 1) = ComplexType(B41, IB41);
  bkq(4, 2) = ComplexType(B42, IB42);
  bkq(4, 3) = ComplexType(B43, IB43);
  bkq(4, 4) = ComplexType(B44, IB44);
  bkq(6, 0) = ComplexType(B60, IB60);
  bkq(6, 1) = ComplexType(B61, IB61);
  bkq(6, 2) = ComplexType(B62, IB62);
  bkq(6, 3) = ComplexType(B63, IB63);
  bkq(6, 4) = ComplexType(B64, IB64);
  bkq(6, 5) = ComplexType(B65, IB65);
  bkq(6, 6) = ComplexType(B66, IB66);

  DoubleFortranVector en;
  ComplexFortranMatrix wf;
  ComplexFortranMatrix ham;
  calculateEigesystem(en, wf, ham, nre, bmol, bext, bkq);

  size_t n = en.size();
  if (n * 2 != values.size()) {
    std::ostringstream msg;
    msg << "Values object has wrong size (" << values.size() << "). Expected "
        << n * 2;
    throw std::runtime_error(msg.str());
  }

  auto temperature = getAttribute("Temperature").asDouble();
  DoubleFortranMatrix intensities = calculateIntensities(nre, en, wf, temperature);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
