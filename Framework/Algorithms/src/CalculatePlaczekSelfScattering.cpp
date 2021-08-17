// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"

#include <utility>

namespace Mantid {
namespace Algorithms {

namespace { // anonymous namespace

// calculate summation term w/ neutron mass over molecular mass ratio
double calculateSummationTerm(const Kernel::Material &material) {
  // add together the weighted sum
  const auto &formula = material.chemicalFormula();
  auto sumLambda = [](double sum, auto &formula_unit) {
    return sum + formula_unit.multiplicity * formula_unit.atom->neutron.tot_scatt_xs / formula_unit.atom->mass;
  };
  const double unnormalizedTerm = std::accumulate(formula.begin(), formula.end(), 0.0, sumLambda);

  // neutron mass converted to atomic mass comes out of the sum
  constexpr double neutronMass = PhysicalConstants::NeutronMass / PhysicalConstants::AtomicMassUnit;
  // normalizing by totalStoich (number of atoms) comes out of the sum
  const double totalStoich = material.totalAtoms();
  // converting scattering cross section to scattering length square comes out of the sum
  return neutronMass * unnormalizedTerm / (4. * M_PI * totalStoich);
}

} // anonymous namespace

DECLARE_ALGORITHM(CalculatePlaczekSelfScattering)

void CalculatePlaczekSelfScattering::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "Raw diffraction data workspace for associated correction to be "
      "calculated for. Workspace must have instument and sample data.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("IncidentSpecta", "", Kernel::Direction::Input),
      "Workspace of fitted incident spectrum with it's first derivative.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "Workspace with the Self scattering correction");
  declareProperty("CrystalDensity", EMPTY_DBL(), "The crystalographic density of the sample material.");
}
//----------------------------------------------------------------------------------------------
/** Validate inputs.
 */
std::map<std::string, std::string> CalculatePlaczekSelfScattering::validateInputs() {
  std::map<std::string, std::string> issues;
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const API::SpectrumInfo specInfo = inWS->spectrumInfo();
  if (specInfo.size() == 0) {
    issues["InputWorkspace"] = "Input workspace does not have detector information";
  }
  Kernel::Material::ChemicalFormula formula = inWS->sample().getMaterial().chemicalFormula();
  if (formula.size() == 0) {
    issues["InputWorkspace"] = "Input workspace does not have a valid sample";
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
double CalculatePlaczekSelfScattering::getPackingFraction(const API::MatrixWorkspace_const_sptr &ws) {
  // get a handle to the material
  const auto &material = ws->sample().getMaterial();

  // default value is packing fraction
  double packingFraction = material.packingFraction();

  // see if the user thinks the material wasn't setup right
  const double crystalDensity = getProperty("CrystalDensity");
  if (crystalDensity > 0.) {
    // assume that the number density set in the Material is the effective number density
    packingFraction = material.numberDensity() / crystalDensity;
  }

  return packingFraction;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczekSelfScattering::exec() {
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpecta");

  // calculate summation term w/ neutron mass over molecular mass ratio
  const double summationTerm = calculateSummationTerm(inWS->sample().getMaterial());
  const double packingFraction = getPackingFraction(inWS);

  // get incident spectrum and 1st derivative
  const MantidVec xLambda = incidentWS->readX(0);
  const MantidVec incident = incidentWS->readY(0);
  const MantidVec incidentPrime = incidentWS->readY(1);
  const double dx = (xLambda[1] - xLambda[0]) / 2.0; // assume constant bin width
  std::vector<double> phi1;
  for (size_t i = 0; i < xLambda.size() - 1; i++) {
    phi1.emplace_back((xLambda[i] + dx) * incidentPrime[i] / incident[i]);
  }
  // set detector law term (eps1)
  std::vector<double> eps1;
  constexpr auto LambdaD = 1.44;
  for (size_t i = 0; i < xLambda.size() - 1; i++) {
    const auto xTerm = -(xLambda[i] + dx) / LambdaD;
    eps1.emplace_back(xTerm * exp(xTerm) / (1.0 - exp(xTerm)));
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
  API::MatrixWorkspace_sptr outputWS = DataObjects::create<API::HistoWorkspace>(*inWS);
  // The algorithm computes the signal values at bin centres so they should
  // be treated as a distribution
  outputWS->setDistribution(true);
  outputWS->setYUnit("");
  outputWS->setYUnitLabel("Counts");
  for (size_t specIndex = 0; specIndex < specInfo.size(); specIndex++) {
    auto &y = outputWS->mutableY(specIndex);
    auto &x = outputWS->mutableX(specIndex);
    if (!specInfo.isMonitor(specIndex) && !(specInfo.l2(specIndex) == 0.0)) {
      Kernel::Units::Wavelength wavelength;
      Kernel::Units::TOF tof;
      Kernel::UnitParametersMap pmap{};
      double l1 = specInfo.l1();
      specInfo.getDetectorValues(wavelength, tof, Kernel::DeltaEMode::Elastic, false, specIndex, pmap);
      double l2 = 0., twoTheta = 0.;
      if (pmap.find(Kernel::UnitParams::l2) != pmap.end()) {
        l2 = pmap[Kernel::UnitParams::l2];
      }
      if (pmap.find(Kernel::UnitParams::twoTheta) != pmap.end()) {
        twoTheta = pmap[Kernel::UnitParams::twoTheta];
      }

      const double sinThetaBy2 = sin(twoTheta / 2.0);
      const double f = l1 / (l1 + l2);
      wavelength.initialize(specInfo.l1(), 0, pmap);
      for (size_t xIndex = 0; xIndex < xLambda.size() - 1; xIndex++) {
        const double term1 = (f - 1.0) * phi1[xIndex];
        const double term2 = f * (1.0 - eps1[xIndex]);
        const double inelasticPlaczekSelfCorrection =
            2.0 * (term1 + term2 - 3) * sinThetaBy2 * sinThetaBy2 * summationTerm;
        x[xIndex] = wavelength.singleToTOF(xLambda[xIndex]);
        y[xIndex] = (1 + inelasticPlaczekSelfCorrection) * packingFraction;
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
