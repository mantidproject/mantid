// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculatePlaczekSelfScattering.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"

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
  std::map<std::string, std::map<std::string, double>>::iterator atom =
      atomSpecies.begin();
  while (atom != atomSpecies.end()) {
    atom->second["concentration"] = atom->second["stoich"] / totalStoich;
    ++atom;
  }
  return atomSpecies;
}

DECLARE_ALGORITHM(CalculatePlaczekSelfScattering)

void CalculatePlaczekSelfScattering::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Kernel::Direction::Input),
      "Raw diffraction data workspace for associated correction to be "
      "calculated for. Workspace must have instument and sample data.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "IncidentSpecta", "", Kernel::Direction::Input),
      "Workspace of fitted incident spectrum with it's first derivative.");
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
  const API::SpectrumInfo specInfo = inWS->spectrumInfo();
  if (specInfo.size() == 0) {
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
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpecta");
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
  const MantidVec xLambda = incidentWS->readX(0);
  const MantidVec incident = incidentWS->readY(0);
  const MantidVec incidentPrime = incidentWS->readY(1);
  double dx = (xLambda[1] - xLambda[0]) / 2.0;
  std::vector<double> phi1;
  for (size_t i = 0; i < xLambda.size() - 1; i++) {
    phi1.push_back((xLambda[i] + dx) * incidentPrime[i] / incident[i]);
  }
  // set detector law term (eps1)
  std::vector<double> eps1;
  constexpr auto LambdaD = 1.44;
  for (size_t i = 0; i < xLambda.size() - 1; i++) {
    auto xTerm = -(xLambda[i] + dx) / LambdaD;
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
  const API::SpectrumInfo specInfo = inWS->spectrumInfo();
  for (size_t detIndex = 0; detIndex < specInfo.size(); detIndex++) {
    if (!(specInfo.isMonitor(detIndex)) && !(specInfo.l2(detIndex) == 0.0)) {
      nReserve += 1;
    }
  }
  xLambdas.reserve(nReserve);
  placzekCorrection.reserve(nReserve);
  API::MatrixWorkspace_sptr outputWS =
      DataObjects::create<API::HistoWorkspace>(*inWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Counts");
  for (size_t specIndex = 0; specIndex < specInfo.size(); specIndex++) {
    auto &y = outputWS->mutableY(specIndex);
    auto &x = outputWS->mutableX(specIndex);
    if (!specInfo.isMonitor(specIndex) && !(specInfo.l2(specIndex) == 0.0)) {
      const double pathLength = specInfo.l1() + specInfo.l2(specIndex);
      const double f = specInfo.l1() / pathLength;
      const double sinThetaBy2 = sin(specInfo.twoTheta(specIndex) / 2.0);
      Kernel::Units::Wavelength wavelength;
      wavelength.initialize(specInfo.l1(), specInfo.l2(specIndex),
                            specInfo.twoTheta(specIndex), 0, 1.0, 1.0);
      for (size_t xIndex = 0; xIndex < xLambda.size() - 1; xIndex++) {
        const double term1 = (f - 1.0) * phi1[xIndex];
        const double term2 = f * (1.0 - eps1[xIndex]);
        const double inelasticPlaczekSelfCorrection =
            2.0 * (term1 + term2 - 3) * sinThetaBy2 * sinThetaBy2 *
            summationTerm;
        x[xIndex] = wavelength.singleToTOF(xLambda[xIndex]);
        y[xIndex] = 1 + inelasticPlaczekSelfCorrection;
      }
      x.back() = wavelength.singleToTOF(xLambda.back());
    } else {
      for (size_t xIndex = 0; xIndex < xLambda.size() - 1; xIndex++) {
        x[xIndex] = xLambda[xIndex];
        y[xIndex] = 0;
      }
      x.back() = xLambda.back();
    }
  }
  auto incidentUnit = inWS->getAxis(0)->unit();
  outputWS->getAxis(0)->unit() = incidentUnit;
  outputWS->setDistribution(false);
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid