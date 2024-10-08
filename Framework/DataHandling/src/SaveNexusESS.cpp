// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveNexusESS.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidNexusGeometry/NexusGeometrySave.h"
#include <H5Cpp.h>

namespace Mantid::DataHandling {
using Mantid::API::Workspace_sptr;
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
const std::string SaveNexusESS::category() const { return "DataHandling\\Nexus"; }

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

  // TODO: Due to the mixture of Nexus and HDF5 operations, and the original design of `SaveNexusESS`: this isn't as
  // efficient as it could be.

  const std::string filename = getProperty("Filename");
  NexusGeometry::LogAdapter<Kernel::Logger> adapter(&g_log);

  SaveNexusProcessed::processGroups();

  // Now loop over the workspace entries again and fill in their NXinstrument groups.
  // (Also see comments at: `SaveNexusProcessed::processGroups`.)
  const auto &workspaces = m_unrolledInputWorkspaces[0];
  if (!workspaces.empty()) {
    for (size_t entry = 0; entry < workspaces.size(); entry++) {
      const Workspace_sptr ws = workspaces[entry];
      auto matrixWs = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
      if (!matrixWs)
        throw std::runtime_error("SaveNexusESS::processGroups: workspace is not a MatrixWorkspace");

      saveNexusGeometry(*matrixWs, filename, entry + 1);
      g_log.information() << "Adding instrument to workspace at group index " << entry << "\n";
    }
  }

  return true;
}

void SaveNexusESS::saveNexusGeometry(const Mantid::API::MatrixWorkspace &ws, const std::string &filename,
                                     std::optional<size_t> entryNumber) {

  try {
    NexusGeometry::LogAdapter<Kernel::Logger> adapter(&g_log);
    NexusGeometry::NexusGeometrySave::saveInstrument(ws, filename, "mantid_workspace_", entryNumber, adapter, true);
  } catch (std::exception &e) {
    g_log.error(std::string(e.what()) + ":\n Nexus Geometry may be absent or incomplete "
                                        "from the processed Nexus file");
  } catch (H5::Exception &ex) {
    g_log.error(ex.getDetailMsg() + ":\n Nexus Geometry may be absent or incomplete "
                                    "from the processed Nexus file");
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
  // Re-use the same properties as those of the base class.
  SaveNexusProcessed::init();
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveNexusESS::exec() {

  API::Workspace_sptr ws = getProperty("InputWorkspace");
  const std::string filename = getProperty("Filename");
  auto matrixWs = std::dynamic_pointer_cast<API::MatrixWorkspace>(ws);
  if (!matrixWs)
    throw std::runtime_error("SaveNexusESS expects a MatrixWorkspace as input");

  // First: call the base-class `exec` method.
  SaveNexusProcessed::exec();

  // Next: append the NeXus geometry
  saveNexusGeometry(*matrixWs, filename);

  return;
}
} // namespace Mantid::DataHandling
