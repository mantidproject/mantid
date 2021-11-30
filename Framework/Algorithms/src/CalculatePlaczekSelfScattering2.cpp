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

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(CalculatePlaczekSelfScattering2)

void CalculatePlaczekSelfScattering2::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "Raw diffraction data workspace for associated correction to be "
      "calculated for. Workspace must have instrument and sample data.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("IncidentSpecta", "", Kernel::Direction::Input),
      "Workspace of fitted incident spectrum with it's first derivative. Must be in units of Wavelength.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Kernel::Direction::Output),
      "Workspace with the Self scattering correction");
  declareProperty("CrystalDensity", EMPTY_DBL(), "The crystalographic density of the sample material.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CalculatePlaczekSelfScattering2::exec() {
  const API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const API::MatrixWorkspace_sptr incidentWS = getProperty("IncidentSpecta");
  const double crystalDensity = getProperty("CrystalDensity");

  auto alg = createChildAlgorithm("CalculatePlaczek");
  alg->setProperty("IncidentSpectra", incidentWS);
  alg->setProperty("InputWorkspace", inWS);
  alg->setProperty("CrystalDensity", crystalDensity);
  alg->setProperty("Order", 1); // default order is one, just being explicit here
  alg->execute();
  API::MatrixWorkspace_sptr outputWS = alg->getProperty("OutputWorkspace");
  if (!bool(outputWS)) {
    throw std::runtime_error("Failed to get the outputworkspace");
  }

  // NOTE: the original version forces the output to be in TOF instead of matching the
  //       input. Therefore, we need to mimic that behaviour here by explicitly converting
  //       the unit of the output workspace to TOF.
  auto cvtalg = createChildAlgorithm("ConvertUnits");
  cvtalg->setProperty("InputWorkspace", outputWS);
  cvtalg->setProperty("outputWorkspace", outputWS);
  cvtalg->setProperty("Target", "TOF");
  cvtalg->execute();
  outputWS = cvtalg->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
