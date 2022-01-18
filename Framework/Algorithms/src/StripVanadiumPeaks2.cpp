// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/StripVanadiumPeaks2.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::Algorithms {

namespace { // anonymous namespace
std::vector<double> POSITIONS_IN_D{0.41192, 0.4279, 0.49076, 0.5044, 0.5191, 0.5350, 0.5526, 0.5936, 0.6178, 0.6453,
                                   0.6768,  0.7134, 0.7566,  0.8089, 0.8737, 0.9571, 1.0701, 1.2356, 1.5133, 2.1401};
}

DECLARE_ALGORITHM(StripVanadiumPeaks2)

void StripVanadiumPeaks2::init() {
  // Declare inputs and output.  Copy most from StripPeaks
  auto algStripPeaks = AlgorithmManager::Instance().createUnmanaged("StripPeaks");
  algStripPeaks->initialize();

  auto dSpaceValidator = std::make_shared<WorkspaceUnitValidator>("dSpacing");
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, dSpaceValidator),
                  "Name of the input workspace. If you use the default vanadium peak "
                  "positions are used, the workspace must be in units of d-spacing.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the "
                  "algorithm.\n"
                  "If the input workspace is an EventWorkspace, then the output must be "
                  "different (and will be made into a Workspace2D).");

  copyProperty(algStripPeaks, "FWHM");
  copyProperty(algStripPeaks, "Tolerance");
  // peak positions are hard-coded
  copyProperty(algStripPeaks, "BackgroundType");
  copyProperty(algStripPeaks, "HighBackground");
  copyProperty(algStripPeaks, "PeakPositionTolerance");
  copyProperty(algStripPeaks, "WorkspaceIndex");
}

void StripVanadiumPeaks2::exec() {
  // get the input workspace
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // setup StripPeaks with hard-coded positions
  constexpr double progressStart{0.0};
  constexpr double progressStop{1.0};
  const bool enableLogging{true};
  auto stripPeaks = createChildAlgorithm("StripPeaks", progressStart, progressStop, enableLogging);
  stripPeaks->setProperty("InputWorkspace", inputWS);
  stripPeaks->setPropertyValue("OutputWorkspace", getProperty("OutputWorkspace")); // allows for in-place operation
  stripPeaks->setProperty<int>("FWHM", getProperty("FWHM"));
  stripPeaks->setProperty<int>("Tolerance", getProperty("Tolerance"));
  stripPeaks->setProperty("PeakPositions", POSITIONS_IN_D);
  stripPeaks->setProperty<std::string>("BackgroundType", getProperty("BackgroundType"));
  stripPeaks->setProperty<bool>("HighBackground", getProperty("HighBackground"));
  if (!isDefault("WorkspaceIndex")) {
    stripPeaks->setProperty<int>("WorkspaceIndex", getProperty("WorkspaceIndex"));
  }
  stripPeaks->setProperty<double>("PeakPositionTolerance", getProperty("PeakPositionTolerance"));

  // Call StripPeaks
  stripPeaks->executeAsChildAlg();

  // Get and set output workspace
  API::MatrixWorkspace_sptr outputWS = stripPeaks->getProperty("OutputWorkspace");
  this->setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
