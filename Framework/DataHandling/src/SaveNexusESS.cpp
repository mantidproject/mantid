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
        ws, filename, "mantid_workspace_1", adapter, true);
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

bool SaveNexusESS::saveLegacyInstrument() {
  /*A hard No on this one. Mantids's current NXDetector, NXMonitor ... types do
   * not have information needed for loading and just cause down-stream
   * problems. Best not to save them in the first place.*/
  return false;
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

  API::Workspace_sptr ws = getProperty("InputWorkspace");
  const std::string filename = getProperty("Filename");
  auto matrixWs = boost::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
  if (!matrixWs)
    throw std::runtime_error("SaveNexusESS expects a MatrixWorkspace as input");
  SaveNexusProcessed::exec();

  // Now append nexus geometry
  saveNexusGeometry(*matrixWs, filename);
  // Now write spectrum to detector maps;
  return;
}
} // namespace DataHandling
} // namespace Mantid
