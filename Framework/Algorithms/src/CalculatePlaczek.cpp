// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CalculatePlaczek.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"

#include <utility>

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

namespace { // anonymous namespace

// calculate summation term w/ neutron mass over molecular mass ratio
// NOTE:
//  - this is directly borrowed from the original CalculatePlaczekSelfScattering
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

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculatePlaczek)

//----------------------------------------------------------------------------------------------
// NOTE: the template seems to prefer having the implemenation in the cpp file

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculatePlaczek::name() const { return "CalculatePlaczek"; }

/// Algorithm's version for identification. @see Algorithm::version
int CalculatePlaczek::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculatePlaczek::category() const { return "CorrectionFunctions"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculatePlaczek::summary() const {
  return "Perform 1st or 2nd order Placzek correction for given spectrum.";
}

/// Algorithm's see also for use in the GUI and help. @see Algorithm::seeAlso
const std::vector<std::string> CalculatePlaczek::seeAlso() const { return {"CalculatePlaczekSelfScattering"}; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CalculatePlaczek::init() {
  // Mandatory properties
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "Raw diffraction data workspace for associated correction to be "
      "calculated for. Workspace must have instrument and sample data.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("IncidentSpectra", "", Kernel::Direction::Input),
      "Workspace of fitted incident spectrum with its derivatives (1st &| 2nd).");

  // Optional properties
  declareProperty("CrystalDensity", EMPTY_DBL(), "The crystalographic density of the sample material.");
  declareProperty("Order", 1, "Placzek correction order (1 or 2), default to 1 (self scattering).");
  declareProperty("SampleTemperature", EMPTY_DBL(),
                  "Sample temperature in Kelvin."
                  "The input properties is prioritized over the temperature recorded in the sample log."
                  "The temperature is necessary for computing second order correction.");
  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "EfficiencySpectra", "", Kernel::Direction::Input, API::PropertyMode::Optional),
                  "Workspace of efficiency spectrum with its derivatives (1st &| 2nd)."
                  "Default to use the build-in efficiency profile for He3 tube if not provided.");

  // Output property
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "Workspace with the Self scattering correction");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> CalculatePlaczek::validateInputs() {
  std::map<std::string, std::string> issues;
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const API::SpectrumInfo specInfo = inWS->spectrumInfo();
  const int order = getProperty("Order");

  // Case0:missing detector info
  if (specInfo.size() == 0) {
    issues["InputWorkspace"] = "Input workspace does not have detector information";
  }

  // Case1: missing sample
  Kernel::Material::ChemicalFormula formula = inWS->sample().getMaterial().chemicalFormula();
  if (formula.size() == 0) {
    issues["InputWorkspace"] = "Input workspace does not have a valid sample.";
  }

  // Case2: cannot locate sample temperature
  if (isDefault("SampleTemperature") && (order == 2)) {
    const auto run = inWS->run();
    const auto sampleTempLogORNL = run.getLogData("SampleTemp");
    const auto sampleTempLogISIS = run.getLogData("sample_temp");
    if (sampleTempLogORNL && sampleTempLogISIS) {
      issues["SampleTemperature"] = "Cannot locate sample temperature in the run.";
    }
  }

  // NOTE: order range check is enforced with validator
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczek::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
