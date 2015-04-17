#include "MantidCrystal/SavePeaksFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
// DECLARE_ALGORITHM(SavePeaksFile)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SavePeaksFile::SavePeaksFile() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SavePeaksFile::~SavePeaksFile() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SavePeaksFile::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("InputWorkspace", "", Direction::Input),
      "An input PeaksWorkspace.");
  std::vector<std::string> exts;
  exts.push_back(".peaks");

  declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
                  "Path to an ISAW-style .peaks filename.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SavePeaksFile::exec() {
  // Retrieve the workspace
  Workspace_sptr ws_in = getProperty("InputWorkspace");
  PeaksWorkspace_sptr ws = boost::dynamic_pointer_cast<PeaksWorkspace>(ws_in);
  if (!ws)
    throw std::invalid_argument(
        "Workspace given as input is invalid or not a PeaksWorkspace.");

  // std::string filename = getPropertyValue("Filename");

  throw std::runtime_error("NOT YET IMPLEMENTED."); // TODO!
                                                    // ws->write(filename);
}

} // namespace Mantid
} // namespace Crystal
