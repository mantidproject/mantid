//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadISISNexus.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadISISNexus)

using namespace Kernel;
using namespace API;

LoadISISNexus::LoadISISNexus() : Algorithm(), DeprecatedAlgorithm() {
  useAlgorithm("LoadISISNexus", 2);
}

/** Initialises the algorithm with the properties as they were when this
 * algorithm was removed from Mantid,
 *  though all validators have been removed
 */
void LoadISISNexus::init() {
  // Leaving the properties as they were when the algorithm body was removed,
  // but with validation removed so that people reach the error message in exec
  // more easily.
  declareProperty("Filename", "", "The name of the Nexus file to load");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "None",
                                                   Direction::Output));

  declareProperty("SpectrumMin", 0);
  declareProperty("SpectrumMax", Mantid::EMPTY_INT());
  declareProperty(new ArrayProperty<int>("SpectrumList"));
  declareProperty("EntryNumber", 0, "The particular entry number to read "
                                    "(default: Load all workspaces and creates "
                                    "a workspace group)");
}

/** Prints a message indicating that this algorithm has been removed.
 *  @throw Exception::NotImplementedError Always
 */
void LoadISISNexus::exec() {
  throw Kernel::Exception::NotImplementedError(
      "This version of LoadISISNexus has been removed from Mantid. "
      "You should use the current version of this algorithm or try an earlier "
      "release of Mantid.");
}

} // namespace DataHandling
} // namespace Mantid
