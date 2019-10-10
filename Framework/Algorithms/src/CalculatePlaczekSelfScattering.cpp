// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculatePlaczekSelfScattering.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/Material.h"

#include <utility>

namespace Mantid {
namespace Algorithms {

std::map<std::string, std::map<std::string, double>>
getSampleSpeciesInfo(const API::MatrixWorkspace_const_sptr ws) {
  // get sample information : mass, total scattering length, and concentration
  // of each species
  double totalStoich = 0.0;
  std::map<std::string, std::map<std::string, double>> atomSpecies;
  const Kernel::Material::ChemicalFormula formula =
      ws->sample().getMaterial().chemicalFormula();
  const double xSection = ws->sample().getMaterial().totalScatterXSection();
  const double bSqrdBar = xSection / (4.0 * M_PI);

  for (const Kernel::Material::Material::FormulaUnit element : formula) {
    const std::map<std::string, double> atomMap{
        {"mass", element.atom->mass},
        {"stoich", element.multiplicity},
        {"bSqrdBar", bSqrdBar}};
    atomSpecies[element.atom->symbol] = atomMap;
    totalStoich += element.multiplicity;
  }
  for (auto atom : atomSpecies) {
    atom.second["concentration"] = atom.second["stoich"] / totalStoich;
  }
  return atomSpecies;
}

DECLARE_ALGORITHM(CalculatePlaczekSelfScattering)

void CalculatePlaczekSelfScattering::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Kernel::Direction::Input),
      "Workspace of fitted incident spectrum with it's first derivative. "
      "Workspace must have instument and sample data.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "Workspace with the Self scattering correction");
}
//----------------------------------------------------------------------------------------------
/** Validate inputs.
 */
std::map<std::string, std::string>
CalculatePlaczekSelfScattering::validateInputs() {
  std::map<std::string, std::string> issues;
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const Geometry::DetectorInfo detInfo = inWS->detectorInfo();
  if (detInfo.size() == 0) {
    issues["InputWorkspace"] =
        "Input workspace does not have detector information";
  }
  Kernel::Material::ChemicalFormula formula =
      inWS->sample().getMaterial().chemicalFormula();
  if (formula.size() == 0) {
    issues["InputWorkspace"] = "Input workspace does not have a valid sample";
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczekSelfScattering::exec() {
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  API::MatrixWorkspace_sptr outWS = getProperty("OutputWorkspace");
  constexpr double factor =
      1.0 / 1.66053906660e-27; // atomic mass unit-kilogram relationship
  constexpr double neutronMass = factor * 1.674927471e-27; // neutron mass
  // get sample information : mass, total scattering length, and concentration
  // of each species
  auto atomSpecies = getSampleSpeciesInfo(inWS);
  // calculate summation term w/ neutron mass over molecular mass ratio
  double summationTerm = 0.0;
  for (auto atom : atomSpecies) {
    summationTerm += atom.second["concentration"] * atom.second["bSqrdBar"] *
                     neutronMass / atom.second["mass"];
  }
  // get incident spectrum and 1st derivative
  const MantidVec xLambda = inWS->readX(0);
  const MantidVec incident = inWS->readY(0);
  const MantidVec incidentPrime = inWS->readY(1);
  double dx = (xLambda[1] - xLambda[0]) / 2.0;
  std::vector<double> phi1;
  for (size_t i = 0; i < xLambda.size() - 1; i++) {
    phi1.push_back((xLambda[i] + dx) * incidentPrime[i] / incident[i]);
  }
  // set detector law term (eps1)
  const double lambdaD = 1.44;
  std::vector<double> eps1;
  for (size_t i = 0; i < xLambda.size(); i++) {
    double xTerm = -xLambda[i] / lambdaD;
    eps1.push_back(xTerm * exp(xTerm) / (1.0 - exp(xTerm)));
  }
  /* Placzek
     Original Placzek inelastic correction Ref (for constant wavelength, reactor
     source): Placzek, Phys. Rev v86, (1952), pp. 377-388 First Placzek
     correction for time-of-flight, pulsed source (also shows reactor eqs.):
     Powles, Mol. Phys., v6 (1973), pp.1325-1350
     Nomenclature and calculation for this program follows Ref:
     Howe, McGreevy, and Howells, J. Phys.: Condens. Matter v1, (1989), pp.
     3433-3451 NOTE: Powles's Equation for inelastic self-scattering is equal to
     Howe's Equation for P(theta) by adding the elastic self-scattering
  */

  MantidVec xLambdas;
  MantidVec placzekCorrection;
  size_t nReserve = 0;
  const Geometry::DetectorInfo detInfo = inWS->detectorInfo();
  for (size_t detIndex = 0; detIndex < detInfo.size(); detIndex++) {
    if (!detInfo.isMonitor(detIndex)) {
      nReserve += 1;
    }
  }
  xLambdas.reserve(nReserve);
  placzekCorrection.reserve(nReserve);

  int nSpec = 0;
  for (size_t detIndex = 0; detIndex < detInfo.size(); detIndex++) {
    if (!detInfo.isMonitor(detIndex)) {
      const double pathLength = detInfo.l1() + detInfo.l2(detIndex);
      const double f = detInfo.l1() / pathLength;
      const double sinThetaBy2 = sin(detInfo.twoTheta(detIndex) / 2.0);
      for (size_t xIndex = 0; xIndex < xLambda.size() - 1; xIndex++) {
        const double term1 = (f - 1.0) * phi1[xIndex];
        const double term2 = f * eps1[xIndex];
        const double term3 = f - 3;
        const double inelasticPlaczekSelfCorrection =
            2.0 * (term1 - term2 + term3) * sinThetaBy2 * sinThetaBy2 *
            summationTerm;
        xLambdas.push_back(xLambda[xIndex]);
        placzekCorrection.push_back(inelasticPlaczekSelfCorrection);
      }
      xLambdas.push_back(xLambda.back());
      nSpec += 1;
    }
  }
  Mantid::API::Algorithm_sptr childAlg =
      createChildAlgorithm("CreateWorkspace");
  childAlg->setProperty("DataX", xLambdas);
  childAlg->setProperty("DataY", placzekCorrection);
  childAlg->setProperty("UnitX", "Wavelength");
  childAlg->setProperty("NSpec", nSpec);
  childAlg->setProperty("ParentWorkspace", inWS);
  childAlg->setProperty("Distribution", true);
  childAlg->execute();
  outWS = childAlg->getProperty("OutputWorkspace");
  setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
