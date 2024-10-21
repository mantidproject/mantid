// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/CrystalFieldEnergies.h"
// #include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include "MantidCurveFitting/Functions/CrystalElectricField.h"

#include <sstream>

namespace Mantid::CurveFitting {

using Mantid::CurveFitting::ComplexFortranMatrix;
using Mantid::CurveFitting::DoubleFortranMatrix;
using Mantid::CurveFitting::DoubleFortranVector;
using Mantid::CurveFitting::Functions::calculateEigensystem;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CrystalFieldEnergies)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CrystalFieldEnergies::name() const { return "CrystalFieldEnergies"; }

/// Algorithm's version for identification. @see Algorithm::version
int CrystalFieldEnergies::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CrystalFieldEnergies::category() const { return "Inelastic"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CrystalFieldEnergies::summary() const {
  return "Calculates crystal field energies and wave functions for rare earth "
         "ions given the field parameters.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CrystalFieldEnergies::init() {

  // Input
  auto bounds = std::make_shared<Kernel::BoundedValidator<int>>(-99, 13);
  declareProperty("Nre", 1, bounds,
                  "A rare earth ion. Possible values are: "
                  "1=Ce 2=Pr 3=Nd 4=Pm 5=Sm 6=Eu 7=Gd 8=Tb "
                  "9=Dy 10=Ho 11=Er 12=Tm 13=Yb, or "
                  "negative values for arbitrary J with "
                  "J=-nre/2 up to nre=-99 (J=99/2)");

  declareProperty("BmolX", 0.0, "The x-component of the molecular field.");
  declareProperty("BmolY", 0.0, "The y-component of the molecular field.");
  declareProperty("BmolZ", 0.0, "The z-component of the molecular field.");

  declareProperty("BextX", 0.0, "The x-component of the external field.");
  declareProperty("BextY", 0.0, "The y-component of the external field.");
  declareProperty("BextZ", 0.0, "The z-component of the external field.");

  declareProperty("B20", 0.0, "Real part of the B20 field parameter.");
  declareProperty("B21", 0.0, "Real part of the B21 field parameter.");
  declareProperty("B22", 0.0, "Real part of the B22 field parameter.");
  declareProperty("B40", 0.0, "Real part of the B40 field parameter.");
  declareProperty("B41", 0.0, "Real part of the B41 field parameter.");
  declareProperty("B42", 0.0, "Real part of the B42 field parameter.");
  declareProperty("B43", 0.0, "Real part of the B43 field parameter.");
  declareProperty("B44", 0.0, "Real part of the B44 field parameter.");
  declareProperty("B60", 0.0, "Real part of the B60 field parameter.");
  declareProperty("B61", 0.0, "Real part of the B61 field parameter.");
  declareProperty("B62", 0.0, "Real part of the B62 field parameter.");
  declareProperty("B63", 0.0, "Real part of the B63 field parameter.");
  declareProperty("B64", 0.0, "Real part of the B64 field parameter.");
  declareProperty("B65", 0.0, "Real part of the B65 field parameter.");
  declareProperty("B66", 0.0, "Real part of the B66 field parameter.");

  declareProperty("IB20", 0.0, "Imaginary part of the B20 field parameter.");
  declareProperty("IB21", 0.0, "Imaginary part of the B21 field parameter.");
  declareProperty("IB22", 0.0, "Imaginary part of the B22 field parameter.");
  declareProperty("IB40", 0.0, "Imaginary part of the B40 field parameter.");
  declareProperty("IB41", 0.0, "Imaginary part of the B41 field parameter.");
  declareProperty("IB42", 0.0, "Imaginary part of the B42 field parameter.");
  declareProperty("IB43", 0.0, "Imaginary part of the B43 field parameter.");
  declareProperty("IB44", 0.0, "Imaginary part of the B44 field parameter.");
  declareProperty("IB60", 0.0, "Imaginary part of the B60 field parameter.");
  declareProperty("IB61", 0.0, "Imaginary part of the B61 field parameter.");
  declareProperty("IB62", 0.0, "Imaginary part of the B62 field parameter.");
  declareProperty("IB63", 0.0, "Imaginary part of the B63 field parameter.");
  declareProperty("IB64", 0.0, "Imaginary part of the B64 field parameter.");
  declareProperty("IB65", 0.0, "Imaginary part of the B65 field parameter.");
  declareProperty("IB66", 0.0, "Imaginary part of the B66 field parameter.");

  // Output
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Energies", Kernel::Direction::Output),
                  "The energies starting at 0 in ascending order.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Eigenvectors", Kernel::Direction::Output),
                  "The eigenvectors.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Hamiltonian", Kernel::Direction::Output),
                  "The Hamiltonian.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CrystalFieldEnergies::exec() {
  int nre = getProperty("Nre");

  double BmolX = getProperty("BmolX");
  double BmolY = getProperty("BmolY");
  double BmolZ = getProperty("BmolZ");
  DoubleFortranVector bmol(1, 3);
  bmol(1) = BmolX;
  bmol(2) = BmolY;
  bmol(3) = BmolZ;

  double BextX = getProperty("BextX");
  double BextY = getProperty("BextY");
  double BextZ = getProperty("BextZ");
  DoubleFortranVector bext(1, 3);
  bext(1) = BextX;
  bext(2) = BextY;
  bext(3) = BextZ;

  double B20 = getProperty("B20");
  double B21 = getProperty("B21");
  double B22 = getProperty("B22");
  double B40 = getProperty("B40");
  double B41 = getProperty("B41");
  double B42 = getProperty("B42");
  double B43 = getProperty("B43");
  double B44 = getProperty("B44");
  double B60 = getProperty("B60");
  double B61 = getProperty("B61");
  double B62 = getProperty("B62");
  double B63 = getProperty("B63");
  double B64 = getProperty("B64");
  double B65 = getProperty("B65");
  double B66 = getProperty("B66");

  double IB20 = getProperty("IB20");
  double IB21 = getProperty("IB21");
  double IB22 = getProperty("IB22");
  double IB40 = getProperty("IB40");
  double IB41 = getProperty("IB41");
  double IB42 = getProperty("IB42");
  double IB43 = getProperty("IB43");
  double IB44 = getProperty("IB44");
  double IB60 = getProperty("IB60");
  double IB61 = getProperty("IB61");
  double IB62 = getProperty("IB62");
  double IB63 = getProperty("IB63");
  double IB64 = getProperty("IB64");
  double IB65 = getProperty("IB65");
  double IB66 = getProperty("IB66");

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
  calculateEigensystem(en, wf, ham, nre, bmol, bext, bkq);

  setProperty("Energies", en.toStdVector());
  setProperty("Eigenvectors", wf.packToStdVector());
  setProperty("Hamiltonian", ham.packToStdVector());
}

} // namespace Mantid::CurveFitting
