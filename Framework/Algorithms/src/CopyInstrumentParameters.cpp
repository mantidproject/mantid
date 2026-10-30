// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopyInstrumentParameters.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <algorithm>
#include <unordered_map>

namespace Mantid::Algorithms {

using std::size_t;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CopyInstrumentParameters)

using namespace Kernel;
using namespace API;
using namespace Geometry;
/// using namespace DataObjects;

void CopyInstrumentParameters::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
                  "Name of the workspace giving the instrument");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::InOut),
                  "Name of the workspace receiving the instrument");
}

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the
 * input workspace
 */
void CopyInstrumentParameters::exec() {

  // Get the giving workspace
  m_givingWorkspace = getProperty("InputWorkspace");

  // Get the receiving workspace
  m_receivingWorkspace = getProperty("OutputWorkspace");

  // Retrieve and validate the input properties
  this->checkProperties();

  if (m_different_instrument_sp) {
    auto Name1 = m_givingWorkspace->getInstrumentName();
    auto Name2 = m_receivingWorkspace->getInstrumentName();
    const auto &componentInfoGiv = m_givingWorkspace->componentInfo();
    const auto &detectorInfoGiv = m_givingWorkspace->detectorInfo();
    const auto &componentInfoRec = m_receivingWorkspace->componentInfo();
    const auto &detectorInfoRec = m_receivingWorkspace->detectorInfo();

    // The receiving components that are not detectors, by full name, built when first needed
    std::unordered_map<std::string, size_t> nonDetectorsRec;
    auto const nonDetectorRec = [&](std::string const &fullName) {
      if (nonDetectorsRec.empty()) {
        for (size_t index = 0; index < componentInfoRec.size(); ++index) {
          if (!componentInfoRec.isDetector(index)) {
            nonDetectorsRec.try_emplace(componentInfoRec.fullName(index), index);
          }
        }
      }
      auto const found = nonDetectorsRec.find(fullName);
      return found != nonDetectorsRec.end() ? found->second : ComponentInfo::invalidIndex;
    };

    Geometry::ParameterMap targMap;

    // Get legacy ParameterMap, i.e., including masking, positions, rotations
    // stored in map (instead of DetectorInfo).
    const auto &givParams = m_givingWorkspace->getInstrument()->makeLegacyParameterMap();
    for (const auto &item : givParams->entries()) {
      const size_t oldIndex = item.first;

      size_t targIndex = ComponentInfo::invalidIndex;
      if (componentInfoGiv.isDetector(oldIndex)) {
        const detid_t detID = detectorInfoGiv.detectorIDs()[oldIndex];
        try {
          targIndex = detectorInfoRec.indexOf(detID);
        } catch (const std::out_of_range &) {
          g_log.warning() << "Target instrument does not have detector with ID " << detID << '\n';
          continue;
        }
      } else {
        std::string source_name = componentInfoGiv.fullName(oldIndex);
        size_t nameStart = source_name.find(Name1);
        std::string targ_name = source_name.replace(nameStart, nameStart + Name1.size(), Name2);
        targIndex = nonDetectorRec(targ_name);
        if (targIndex == ComponentInfo::invalidIndex) {
          g_log.warning() << "Target instrument does not have component with full name: " << targ_name << '\n';
          continue;
        }
      }
      const auto *targComp = componentInfoRec.componentID(targIndex);

      // create shared pointer to independent copy of original parameter. Would
      // be easy and nice to have cow_pointer instead of shared_ptr in the
      // parameter map.
      auto param = Parameter_sptr(item.second->clone());
      // add new parameter to the maps for existing target component
      targMap.add(targComp, param);
    }

    // Clear old parameters, and the fields stored in DetectorInfo and ComponentInfo (masking, positions, rotations),
    // returning the receiving instrument to its base geometry before the copied parameters are applied.
    auto clear = createChildAlgorithm("ClearInstrumentParameters");
    clear->setProperty("Workspace", m_receivingWorkspace);
    clear->executeAsChildAlg();
    // ExperimentInfo::readParameterMap deals with extracting legacy information
    // from ParameterMap.
    m_receivingWorkspace->readParameterMap(targMap.asString());
  } else {
    // Same base instrument, copying the instrument is equivalent to copying the
    // parameters in the ParameterMap and the DetectorInfo.
    m_receivingWorkspace->setInstrument(m_givingWorkspace->getInstrument());
  }
}

/** Checks whether the two workspaces have the same base instrument, recording the result in
 * m_different_instrument_sp.
 */
void CopyInstrumentParameters::checkProperties() {

  // A component ID is the address of the base component, so the IDs of the roots identify the base instruments
  const auto &componentInfoGiv = m_givingWorkspace->componentInfo();
  const auto &componentInfoRec = m_receivingWorkspace->componentInfo();
  if (componentInfoGiv.componentID(componentInfoGiv.root()) != componentInfoRec.componentID(componentInfoRec.root())) {
    m_different_instrument_sp = true;
    g_log.warning() << "The base instrument in the output workspace is not the "
                       "same as the base instrument in the input workspace.\n";
  }
}
} // namespace Mantid::Algorithms
