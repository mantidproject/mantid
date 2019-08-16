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

#include "MantidAPI/FileProperty.h"
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
  const std::vector<std::string> exts{".nxs", ".hdf5"};
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "workspace containing the Instrument.");

  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, exts),
                  "full path save destination");

  declareProperty("H5Path", "", "name of the H5 path save destination group.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveNexusGeometry::exec() {
  API::MatrixWorkspace_const_sptr workspace = getProperty("InputWorkspace");
  std::string destinationFile = getPropertyValue("FileName");
  std::string rootFileName = getPropertyValue("H5Path");
  const auto instrument = workspace->getInstrument();
  auto instr = Mantid::Geometry::InstrumentVisitor::makeWrappers(*instrument);
  Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(
      instr, destinationFile, rootFileName);
}

} // namespace DataHandling
} // namespace Mantid
