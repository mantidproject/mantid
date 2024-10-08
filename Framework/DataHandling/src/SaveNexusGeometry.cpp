// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"

#include <memory>
#include <utility>

namespace Mantid::DataHandling {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNexusGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name

const std::string SaveNexusGeometry::name() const { return "SaveNexusGeometry"; }

/// Algorithm's version for identification. @see Algorithm::version

int SaveNexusGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category

const std::string SaveNexusGeometry::category() const { return "DataHandling\\Instrument"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary

const std::string SaveNexusGeometry::summary() const {
  return "Save the instrument from a workspace to a Nexus-format"
         " HDF file. WARNING: shapes are NOT saved in the present version "
         "(1.0).";
}

//----------------------------------------------------------------------------------------------

/** Initialize the algorithm's properties.
 */
void SaveNexusGeometry::init() {

  // TODO resolve linkererror for experimentinfo, replace MatrixWorkspace with
  // base class ExperimentInfo.
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Workspace containing the Instrument.");

  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::OptionalSave),
                  "Full path to save destination file");

  declareProperty("EntryName", "entry" /*default*/,
                  "(optional) Name of the H5 root group in which the "
                  "Instrument is to be saved. Default name: 'entry'.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveNexusGeometry::exec() {
  API::MatrixWorkspace_const_sptr workspace = getProperty("InputWorkspace");
  std::string destinationFile = getPropertyValue("FileName");
  std::string parentEntryName = getPropertyValue("EntryName");

  auto ws = workspace.get();
  const auto &compInfo = ws->componentInfo();
  const auto &detInfo = ws->detectorInfo();

  NexusGeometry::LogAdapter<Kernel::Logger> adapter(&g_log);
  Mantid::NexusGeometry::NexusGeometrySave::saveInstrument(compInfo, detInfo, destinationFile, parentEntryName,
                                                           adapter);
}

} // namespace Mantid::DataHandling
