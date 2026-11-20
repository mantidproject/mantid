// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/GridDetectorPixel.h"
#include "MantidKernel/Exception.h"

#include <deque>

namespace Mantid::DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MoveInstrumentComponent)

using namespace Kernel;
using namespace Geometry;
using namespace API;

namespace {
/// The component given by a name or a '/'-delimited path of names, or ComponentInfo::invalidIndex if there is none.
/// As in CompAssembly::getComponentByName, each name is searched for breadth-first from the previous match
/// (inclusive), so a path may skip steps.
size_t indexOfPath(ComponentInfo const &componentInfo, std::string const &path) {
  if (componentInfo.size() == 0) {
    return ComponentInfo::invalidIndex;
  }
  size_t current = componentInfo.root();
  size_t segmentStart = 0;
  while (segmentStart <= path.size()) {
    auto const separator = path.find('/', segmentStart);
    auto const segment =
        path.substr(segmentStart, separator == std::string::npos ? separator : separator - segmentStart);
    std::deque<size_t> queue{current};
    size_t found = ComponentInfo::invalidIndex;
    while (!queue.empty() && found == ComponentInfo::invalidIndex) {
      auto const index = queue.front();
      queue.pop_front();
      if (componentInfo.name(index) == segment) {
        found = index;
      } else {
        auto const &children = componentInfo.children(index);
        queue.insert(queue.end(), children.cbegin(), children.cend());
      }
    }
    if (found == ComponentInfo::invalidIndex || separator == std::string::npos) {
      return found;
    }
    current = found;
    segmentStart = separator + 1;
  }
  return ComponentInfo::invalidIndex;
}
} // namespace

/// Empty default constructor
MoveInstrumentComponent::MoveInstrumentComponent() = default;

/// Initialisation method.
void MoveInstrumentComponent::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace for which the new instrument "
                  "configuration will have an effect. Any other workspaces "
                  "stored in the analysis data service will be unaffected.");
  declareProperty("ComponentName", "",
                  "The name of the component to move. Component names are "
                  "defined in the instrument definition files. A pathname "
                  "delited by '/' may be used for non-unique name.");
  declareProperty("DetectorID", -1,
                  "The ID of the detector to move. If both "
                  "the component name and the detector ID "
                  "are set the latter will be used.");
  declareProperty("X", 0.0, "The x-part of the new location vector.");
  declareProperty("Y", 0.0, "The y-part of the new location vector.");
  declareProperty("Z", 0.0, "The z-part of the new location vector.");
  declareProperty("RelativePosition", true,
                  "The property defining how the (X,Y,Z) vector should be "
                  "interpreted. If true it is a vector relative to the initial "
                  "component's position. Otherwise it is a new position in the "
                  "absolute co-ordinates.");
  declareProperty("MoveFixedDetectors", false,
                  "Whether to allow moving of individual detector pixels located within a structured bank. "
                  "If set to false then a request to move a detector pixel within a structured bank will be ignored. "
                  "The default value for this property is set to false to maintain backwards compatibility.");
}

/** Executes the algorithm.
 *
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void MoveInstrumentComponent::exec() {
  // Get the input workspace
  Workspace_sptr ws = getProperty("Workspace");
  MatrixWorkspace_sptr inputW = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  DataObjects::PeaksWorkspace_sptr inputP = std::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ws);

  if (!inputW && !inputP) {
    throw std::runtime_error("The workspace does not seem to be valid as input (must be either MatrixWorkspace or "
                             "PeaksWorkspace");
  }
  const std::string ComponentName = getProperty("ComponentName");
  const int DetID = getProperty("DetectorID");
  const double X = getProperty("X");
  const double Y = getProperty("Y");
  const double Z = getProperty("Z");
  const bool relativePosition = getProperty("RelativePosition");
  const bool moveFixedDetectors = getProperty("MoveFixedDetectors");

  auto &componentInfo = inputW ? inputW->mutableComponentInfo() : inputP->mutableComponentInfo();
  size_t compIndex = ComponentInfo::invalidIndex;
  // Find the component to move
  if (DetID != -1) {
    auto const &detectorInfo = inputW ? inputW->detectorInfo() : inputP->detectorInfo();
    try {
      compIndex = detectorInfo.indexOf(DetID);
    } catch (std::out_of_range const &) {
      std::ostringstream mess;
      mess << "Detector with ID " << DetID << " was not found.";
      g_log.error(mess.str());
      throw std::runtime_error(mess.str());
    }
  } else if (!ComponentName.empty()) {
    compIndex = indexOfPath(componentInfo, ComponentName);
    if (compIndex == ComponentInfo::invalidIndex) {
      std::ostringstream mess;
      mess << "Component with name " << ComponentName << " was not found.";
      g_log.error(mess.str());
      throw std::runtime_error(mess.str());
    }
  } else {
    g_log.error("DetectorID or ComponentName must be given.");
    throw std::invalid_argument("DetectorID or ComponentName must be given.");
  }

  if (!moveFixedDetectors && ComponentInfoBankHelpers::isDetectorFixedInBank(componentInfo, compIndex)) {
    // DetectorInfo makes changing positions possible but we keep the old
    // behavior of ignoring position changes for Structured banks.
    g_log.warning("Component is fixed within a structured bank, moving is not "
                  "possible, doing nothing.");
    return;
  }

  // Do the move
  V3D position(X, Y, Z);
  if (relativePosition)
    position += componentInfo.position(compIndex);

  componentInfo.setPosition(compIndex, position);
}

} // namespace Mantid::DataHandling
