// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

/* SaveNexusGeometry : A thin Algorithm wrapper over
 * NexusGeometry::saveInstrument allowing user to save the geometry from
 * instrument attached to a workspace.
 *
 * @author Takudzwa Makoni, RAL (UKRI), ISIS
 * @date 16/08/2019
 */
#include "MantidDataHandling/SaveNexusGeometry.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"

#include <memory>
#include <utility>

namespace Mantid {
namespace DataHandling {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNexusGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name

const std::string SaveNexusGeometry::name() const {
  return "SaveNexusGeometry";
}

/// Algorithm's version for identification. @see Algorithm::version

int SaveNexusGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category

const std::string SaveNexusGeometry::category() const {
  return "DataHandling\\Instrument";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary

const std::string SaveNexusGeometry::summary() const {
  return "Reads the instrument from a workspace, and saves it to a Nexus "
         "file with the full path file "
         "destination and root name.";
}

//----------------------------------------------------------------------------------------------

/** Initialize the algorithm's properties.
 */
void SaveNexusGeometry::init() {
  const std::vector<std::string> exts{".nxs", ".hdf5"};
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Workspace containing the Instrument.");

  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, exts),
                  "Full path to save destination file");

  declareProperty("H5Path", "entry" /*default*/,
                  "(optional) Name of the H5 root group. Default: 'entry'.");
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
  /*
  const std::pair<std::unique_ptr<Mantid::Geometry::ComponentInfo>,
                  std::unique_ptr<Geometry::DetectorInfo>>
      instrument= std::make_pair(std::make_unique<Geometry::ComponentInfo>(
      workspace->componentInfo()),
  std::make_unique<Geometry::DetectorInfo>(workspace->detectorInfo()));
  */

  Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(
      instr, destinationFile, rootFileName);
}

} // namespace DataHandling
} // namespace Mantid
