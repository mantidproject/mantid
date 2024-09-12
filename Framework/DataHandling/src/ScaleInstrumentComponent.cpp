// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ScaleInstrumentComponent.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(ScaleInstrumentComponent)

using namespace Kernel;
using namespace Geometry;
using namespace API;

ScaleInstrumentComponent::ScaleInstrumentComponent() = default;

void ScaleInstrumentComponent::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::InOut),
                  "The name of the workspace containing the instrument component to be scaled.");
  declareProperty("ComponentName", "",
                  "The name of the component to scale. Component names are "
                  "defined in the instrument definition files. A pathname "
                  "delineated by '/' may be used for non-unique name.");
  declareProperty(std::make_unique<ArrayProperty<double>>("Scalings", "1.0, 1.0, 1.0"),
                  "A 3D vector specifying the scaling factors for the component.");

  declareProperty("ScalePixelSizes", true, "Scale the pixel dimensions of the detector.");
}
std::map<std::string, std::string> ScaleInstrumentComponent::validateInputs() {
  std::map<std::string, std::string> result;

  // Retrieve the workspace property and attempt to cast it to appropriate types
  Workspace_sptr ws = getProperty("Workspace");
  MatrixWorkspace_sptr inputW = std::dynamic_pointer_cast<MatrixWorkspace>(ws);
  DataObjects::PeaksWorkspace_sptr inputP = std::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(ws);

  Instrument_const_sptr inst = nullptr;

  // Determine the type of workspace and get the instrument
  if (inputW) {
    inst = inputW->getInstrument();
    m_componentInfo = inst ? &inputW->mutableComponentInfo() : nullptr;
  } else if (inputP) {
    inst = inputP->getInstrument();
    m_componentInfo = inst ? &inputP->mutableComponentInfo() : nullptr;
  } else {
    result["Workspace"] = "Input workspace must be either MatrixWorkspace or PeaksWorkspace.";
    return result;
  }

  // Validate instrument extraction
  if (!inst) {
    result["Workspace"] = "Could not get a valid instrument from the provided workspace.";
    return result;
  }

  // Validate and retrieve the component by name
  std::string componentName = getProperty("ComponentName");
  if (componentName.empty()) {
    result["ComponentName"] = "ComponentName must be provided.";
    return result;
  }

  m_comp = inst->getComponentByName(componentName);
  if (!m_comp) {
    result["ComponentName"] = "Component with name " + componentName + " was not found.";
    return result;
  }

  // Validate component info
  if (!m_componentInfo) {
    result["ComponentInfo"] = "Could not get component info from the workspace.";
    return result;
  }

  // Check if the component is a detector
  const auto componentIndex = m_componentInfo->indexOf(m_comp->getComponentID());
  if (m_componentInfo->isDetector(componentIndex)) {
    result["ComponentName"] = "Cannot scale a detector. Please provide a non-detector component name.";
  }

  return result;
}

/** Executes the algorithm.
 */
void ScaleInstrumentComponent::exec() {

  std::vector<double> scalingsXYZ = getProperty("Scalings");
  Kernel::V3D scalings(scalingsXYZ[0], scalingsXYZ[1], scalingsXYZ[2]);
  const bool scalePixels = getProperty("ScalePixelSizes");

  const auto componentId = m_comp->getComponentID();
  m_componentInfo->scaleComponent(m_componentInfo->indexOf(componentId), scalings);

  if (scalePixels) {
    std::vector<size_t> detectors;
    detectors = m_componentInfo->detectorsInSubtree(m_componentInfo->indexOf(componentId));
    for (const auto &detector : detectors) {
      auto oldScale = m_componentInfo->scaleFactor(detector);
      m_componentInfo->setScaleFactor(
          detector, V3D(oldScale.X() * scalings.X(), oldScale.Y() * scalings.Y(), oldScale.Z() * scalings.Z()));
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
