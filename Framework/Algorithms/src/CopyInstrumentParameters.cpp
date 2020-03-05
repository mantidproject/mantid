// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopyInstrumentParameters.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <algorithm>

namespace Mantid {
namespace Algorithms {

using std::size_t;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CopyInstrumentParameters)

using namespace Kernel;
using namespace API;
using namespace Geometry;
/// using namespace DataObjects;

void CopyInstrumentParameters::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "Name of the workspace giving the instrument");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::InOut),
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
    Instrument_const_sptr inst1 = m_givingWorkspace->getInstrument();
    Instrument_const_sptr inst2 = m_receivingWorkspace->getInstrument();
    auto Name1 = inst1->getName();
    auto Name2 = inst2->getName();

    Geometry::ParameterMap targMap;

    // Get legacy ParameterMap, i.e., including masking, positions, rotations
    // stored in map (instead of DetectorInfo).
    const auto &givParams = inst1->makeLegacyParameterMap();
    for (const auto &item : *givParams) {
      IComponent *oldComponent = item.first;

      const Geometry::IComponent *targComp = nullptr;

      auto *pOldDet = dynamic_cast<IDetector *>(oldComponent);
      if (pOldDet) {
        detid_t detID = pOldDet->getID();
        targComp = inst2->getBaseDetector(detID);
        if (!targComp) {
          g_log.warning() << "Target instrument does not have detector with ID "
                          << detID << '\n';
          continue;
        }
      } else {
        std::string source_name = oldComponent->getFullName();
        size_t nameStart = source_name.find(Name1);
        std::string targ_name =
            source_name.replace(nameStart, nameStart + Name1.size(), Name2);
        // existingComponents.
        auto spTargComp = inst2->getComponentByName(targ_name);
        if (!spTargComp) {
          g_log.warning()
              << "Target instrument does not have component with full name: "
              << targ_name << '\n';
          continue;
        }
        targComp = spTargComp->getBaseComponent();
      }

      // create shared pointer to independent copy of original parameter. Would
      // be easy and nice to have cow_pointer instead of shared_ptr in the
      // parameter map.
      auto param = Parameter_sptr(item.second->clone());
      // add new parameter to the maps for existing target component
      targMap.add(targComp, param);
    }

    // Clear old parameters. We also want to clear fields stored in DetectorInfo
    // (masking, positions, rotations). By setting the base instrument (which
    // does not include a ParameterMap or DetectorInfo) we make use of the
    // mechanism in ExperimentInfo that builds a clean DetectorInfo from the
    // instrument being set.
    m_receivingWorkspace->setInstrument(inst2->baseInstrument());
    // ExperimentInfo::readParameterMap deals with extracting legacy information
    // from ParameterMap.
    m_receivingWorkspace->readParameterMap(targMap.asString());
  } else {
    // Same base instrument, copying the instrument is equivalent to copying the
    // parameters in the ParameterMap and the DetectorInfo.
    m_receivingWorkspace->setInstrument(m_givingWorkspace->getInstrument());
  }
}

/** Retrieves the properties and checks that they have valid values.
 *  @throw std::invalid_argument If either workspace has no instrument or the
 * instruments have different base instruments.
 */
void CopyInstrumentParameters::checkProperties() {

  // Check that both workspaces have an instrument
  Instrument_const_sptr inst = m_givingWorkspace->getInstrument();
  if (!inst) {
    throw std::invalid_argument("Input workspace has no instrument");
  }
  Instrument_const_sptr inst2 = m_receivingWorkspace->getInstrument();
  if (!inst2) {
    throw std::invalid_argument("Output workspace has no instrument");
  }

  Instrument_const_sptr baseInstGiv = inst->baseInstrument();
  Instrument_const_sptr baseInstRec = inst2->baseInstrument();

  // Check that both workspaces have the same instrument name
  if (baseInstRec != baseInstGiv) {
    m_different_instrument_sp = true;
    g_log.warning() << "The base instrument in the output workspace is not the "
                       "same as the base instrument in the input workspace.\n";
  }
}

Parallel::ExecutionMode CopyInstrumentParameters::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  const auto in = storageModes.at("InputWorkspace");
  const auto out = storageModes.at("InputWorkspace");
  // Source instrument avaible only on master rank, so copying not possible if
  // target requires it on non-master ranks.
  if (in == Parallel::StorageMode::MasterOnly && in != out)
    return Parallel::ExecutionMode::Invalid;
  return Parallel::getCorrespondingExecutionMode(out);
}

} // namespace Algorithms
} // namespace Mantid
