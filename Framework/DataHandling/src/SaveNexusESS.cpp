// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNexusESS.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNexusESS)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SaveNexusESS::name() const { return "SaveNexusESS"; }

/// Algorithm's version for identification. @see Algorithm::version
int SaveNexusESS::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveNexusESS::category() const {
  return "DataHandling\\Nexus";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SaveNexusESS::summary() const {
  return "Saves intermediate, also known as 'processed' nexus file with data "
         "and geometry";
}

/**
 * @brief SaveNexusESS::processGroups
 * @return
 */
bool SaveNexusESS::processGroups() {
  throw std::invalid_argument(
      "SaveNexusESS does not currently support operations on groups");
}

void SaveNexusESS::saveNexusGeometry(const Mantid::API::MatrixWorkspace &ws,
                                     const std::string &filename) {

  try {
    NexusGeometry::LogAdapter<Kernel::Logger> adapter(&g_log);
    NexusGeometry::NexusGeometrySave::saveInstrument(
        ws.componentInfo(), ws.detectorInfo(), filename, "mantid_workspace_1",
        adapter, false);
  } catch (std::exception &e) {
    g_log.error(std::string(e.what()) +
                " Nexus Geometry may be absent or incomplete "
                "from processed Nexus file");
  } catch (H5::Exception &ex) {
    g_log.error(ex.getDetailMsg() +
                " Nexus Geometry may be absent or incomplete "
                "from processed Nexus file");
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveNexusESS::init() {
  // Take same properties as base.
  SaveNexusProcessed::init();
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveNexusESS::exec() {
  // Run the base algorithm. Template method approach used to call ESS
  // specifics.
  return SaveNexusProcessed::exec();
}

} // namespace DataHandling
} // namespace Mantid
