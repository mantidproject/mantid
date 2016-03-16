#include "MantidCurveFitting/Algorithms/CrystalFieldEnergies.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"

#include "MantidCurveFitting/Functions/CrystalElectricField.h"

#include <sstream>

using Mantid::CurveFitting::ComplexFortranMatrix;
using Mantid::CurveFitting::DoubleFortranMatrix;
using Mantid::CurveFitting::DoubleFortranVector;
using Mantid::CurveFitting::Functions::calculateEigesystem;

namespace Mantid {
namespace CurveFitting {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CrystalFieldEnergies)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CrystalFieldEnergies::name() const { return "CrystalFieldEnergies"; }

/// Algorithm's version for identification. @see Algorithm::version
int CrystalFieldEnergies::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CrystalFieldEnergies::category() const {
  return "Inelastic";
}

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
  declareProperty("Nre", 1,  "A rare earth ion.");
  auto threeElements =
      boost::make_shared<Kernel::ArrayLengthValidator<double>>(3);
  std::vector<double> defaultVector(3, 0);
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Bmol", defaultVector, threeElements), "Bmol.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Bbext", defaultVector, threeElements), "Bext.");
  declareProperty("B20", 0.0,  "B20.");
  declareProperty("B21", 0.0,  "B21.");
  declareProperty("B22", 0.0,  "B22.");
  declareProperty("B40", 0.0,  "B40.");
  declareProperty("B41", 0.0,  "B41.");
  declareProperty("B42", 0.0,  "B42.");
  declareProperty("B43", 0.0,  "B43.");
  declareProperty("B44", 0.0,  "B44.");
  declareProperty("B60", 0.0,  "B60.");
  declareProperty("B61", 0.0,  "B61.");
  declareProperty("B62", 0.0,  "B62.");
  declareProperty("B63", 0.0,  "B63.");
  declareProperty("B64", 0.0,  "B64.");
  declareProperty("B65", 0.0,  "B65.");
  declareProperty("B66", 0.0,  "B66.");

  // Output
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Energies", Kernel::Direction::Output), "Energies.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Eigenvectors", Kernel::Direction::Output), "The eigenvectors.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Hamiltonian", Kernel::Direction::Output), "The Hamiltonian.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CrystalFieldEnergies::exec() {
  int nre = getProperty("Nre");

  DoubleFortranVector bmol(1, 3);
  std::vector<double> bmolProp = getProperty("Bmol");

  DoubleFortranVector bext(1, 3);

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

  ComplexFortranMatrix bkq(0,6, 0,6);
  bkq(2, 0) = B20;
  bkq(2, 1) = B21;
  bkq(2, 2) = B22;
  bkq(4, 0) = B40;
  bkq(4, 1) = B41;
  bkq(4, 2) = B42;
  bkq(4, 3) = B43;
  bkq(4, 4) = B44;
  bkq(6, 0) = B60;
  bkq(6, 1) = B61;
  bkq(6, 2) = B62;
  bkq(6, 3) = B63;
  bkq(6, 4) = B64;
  bkq(6, 5) = B65;
  bkq(6, 6) = B66;

  DoubleFortranVector en;
  ComplexFortranMatrix wf;
  ComplexFortranMatrix ham;
  calculateEigesystem(en, wf, ham, nre, bmol, bext, bkq);

  setProperty("Energies", en.toStdVector());
  setProperty("Eigenvectors", wf.packToStdVector());
  setProperty("Hamiltonian", ham.packToStdVector());

}

} // namespace CurveFitting
} // namespace Mantid
