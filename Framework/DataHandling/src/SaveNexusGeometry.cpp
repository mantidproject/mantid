// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNexusGeometry.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

namespace Mantid {
namespace DataHandling {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNexusGeometry)

//----------------------------------------------------------------------------------------------

/** Initialize the algorithm's properties.
 */
void SaveNexusGeometry::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "workspace containing the Instrument.");
  declareProperty(std::make_unique<WorkspaceProperty<std::string>>(
                      "SaveDestination", "", Direction::Input),
                  "full save destination path.");
  declareProperty(std::make_unique<WorkspaceProperty<std::string>>(
                      "FileRootName", "", Direction::Input),
                  "name of file root.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveNexusGeometry::exec() {
  boost::shared_ptr<API::MatrixWorkspace> workspace =
      getProperty("InputWorkspace");
  std::string destinationFile = getPropertyValue("SaveDestination");
  std::string rootFileName = getPropertyValue("FileRootName");
  auto &instrument = workspace->getInstrument();
  auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
  Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(
      instr, destinationFile, rootFileName);
}

} // namespace DataHandling
} // namespace Mantid
