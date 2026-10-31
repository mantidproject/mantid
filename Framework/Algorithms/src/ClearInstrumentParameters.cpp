// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ClearInstrumentParameters.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearInstrumentParameters)

using namespace Kernel;
using namespace API;

/// Algorithm's name for identification. @see Algorithm::name
const std::string ClearInstrumentParameters::name() const { return "ClearInstrumentParameters"; }

/// Summary of the algorithm's purpose. @see Algorithm::summary
const std::string ClearInstrumentParameters::summary() const {
  return "Clears all the parameters associated with a workspace's instrument.";
}

/// Algorithm's version for identification. @see Algorithm::version
int ClearInstrumentParameters::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearInstrumentParameters::category() const { return "DataHandling\\Instrument"; }

/** Initialize the algorithm's properties.
 */
void ClearInstrumentParameters::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("Workspace", "", Direction::InOut, std::make_shared<InstrumentValidator>()),
      "Workspace whose instrument parameters are to be cleared.");
}

/** Execute the algorithm.
 */
void ClearInstrumentParameters::exec() {
  const MatrixWorkspace_sptr ws = getProperty("Workspace");
  auto &detectorInfo = ws->mutableDetectorInfo();
  if (detectorInfo.isScanning()) {
    // The base instrument has a single time index, so there is no unparametrized geometry to restore each scan
    // point to, and the scan positions are data rather than parameters.
    throw std::runtime_error("ClearInstrumentParameters cannot be used on a workspace with a detector scan.");
  }

  auto &componentInfo = ws->mutableComponentInfo();
  componentInfo.clearParameters();
  detectorInfo.clearMaskFlags();

  // Restore the geometry as loaded from the instrument definition. Moving or rotating a component carries its
  // subtree with it, so parents must be restored before their children. Every parent has a higher component index
  // than its children, so walking down from the root visits them in that order.
  const auto &baseComponentInfo = ws->baseComponentInfo();
  // Set unconditionally: V3D and Quat compare with a tolerance, which would leave small calibration offsets behind.
  for (size_t index = componentInfo.size(); index-- > 0;) {
    componentInfo.setRotation(index, baseComponentInfo.rotation(index));
    componentInfo.setPosition(index, baseComponentInfo.position(index));
    componentInfo.setScaleFactor(index, baseComponentInfo.scaleFactor(index));
  }
}

} // namespace Mantid::Algorithms
