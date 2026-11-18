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

    // The legacy parameters (including masking, positions and rotations), written as
    // ParameterMap::asString() writes them but naming components in the receiving instrument.
    // readParameterMap() warns about and skips any component the receiving instrument lacks.
    std::string targParams;
    for (const auto &[index, param] : m_givingWorkspace->getInstrument()->legacyParameters()) {
      std::string targComponent;
      if (componentInfoGiv.isDetector(index)) {
        targComponent = "detID:" + std::to_string(detectorInfoGiv.detectorIDs()[index]);
      } else {
        targComponent = componentInfoGiv.fullName(index);
        targComponent.replace(targComponent.find(Name1), Name1.size(), Name2);
      }
      targParams += ParameterMap::asStringEntry(targComponent, *param);
    }

    // Clear old parameters, and the fields stored in DetectorInfo and ComponentInfo (masking, positions, rotations),
    // returning the receiving instrument to its base geometry before the copied parameters are applied.
    auto clear = createChildAlgorithm("ClearInstrumentParameters");
    clear->setProperty("Workspace", m_receivingWorkspace);
    clear->executeAsChildAlg();
    // ExperimentInfo::readParameterMap deals with extracting legacy information
    // from ParameterMap.
    m_receivingWorkspace->readParameterMap(targParams);
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

  // Workspaces of the same base instrument clone their ComponentInfo from the same cached base
  if (!m_givingWorkspace->componentInfo().hasSameBase(m_receivingWorkspace->componentInfo())) {
    m_different_instrument_sp = true;
    g_log.warning() << "The base instrument in the output workspace is not the "
                       "same as the base instrument in the input workspace.\n";
  }
}
} // namespace Mantid::Algorithms
