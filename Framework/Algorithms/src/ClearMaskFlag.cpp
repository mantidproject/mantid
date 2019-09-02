// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ClearMaskFlag.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

namespace Mantid {
namespace Algorithms {

using namespace Geometry;
using namespace API;
using Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearMaskFlag)

/// Algorithm's name for identification. @see Algorithm::name
const std::string ClearMaskFlag::name() const { return "ClearMaskFlag"; }

/// Algorithm's version for identification. @see Algorithm::version
int ClearMaskFlag::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearMaskFlag::category() const {
  return "Transforms\\Masking";
}

/** Initialize the algorithm's properties.
 */
void ClearMaskFlag::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("Workspace", "", Direction::InOut),
      "Workspace to clear the mask flag of.");
  declareProperty("ComponentName", "",
                  "Specify the instrument component to clear the "
                  "mask. If empty clears the mask flag for "
                  "the whole instrument.");
}

/** Execute the algorithm.
 */
void ClearMaskFlag::exec() {
  MatrixWorkspace_sptr ws = getProperty("Workspace");
  std::string componentName = getPropertyValue("ComponentName");
  auto &detectorInfo = ws->mutableDetectorInfo();

  if (!componentName.empty()) {
    std::vector<IDetector_const_sptr> detectors;
    ws->getInstrument()->getDetectorsInBank(detectors, componentName);
    for (const auto &det : detectors) {
      auto index = detectorInfo.indexOf(det->getID());
      detectorInfo.setMasked(index, false);
    }
  } else {
    detectorInfo.clearMaskFlags();
  }
}

} // namespace Algorithms
} // namespace Mantid
