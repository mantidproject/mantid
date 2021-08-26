// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CalculatePlaczekSelfScattering2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CalculatePlaczek.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"

#include <utility>

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CalculatePlaczekSelfScattering2)

void CalculatePlaczekSelfScattering2::init() {
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
std::map<std::string, std::string> CalculatePlaczekSelfScattering2::validateInputs() {
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
double CalculatePlaczekSelfScattering2::getPackingFraction(const API::MatrixWorkspace_const_sptr &ws) {
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
void CalculatePlaczekSelfScattering2::exec() {
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpecta");

  CalculatePlaczek alg;
  alg.initialize();
  if (alg.isInitialized()) {
    alg.setProperty("IncidentSpectra", incidentWS);
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "correction_ws");

    alg.execute();
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    setProperty("OutputWorkspace", outputWS);
  } else {
    g_log.error() << "CalculatePlaczek failed to initialize, aborting CalculatePlaczekSelfScattering";
  }
}

} // namespace Algorithms
} // namespace Mantid
