#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/LoadTomoConfig.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadTomoConfig);

using Kernel::Direction;
using namespace Mantid::API;

LoadTomoConfig::LoadTomoConfig(): m_file(NULL) {

}

LoadTomoConfig::~LoadTomoConfig() {

}

/** 
 * Standard Initialisation method. Declares properties.
 */
void LoadTomoConfig::init() {
  // Required input properties
  std::vector<std::string> exts;
  exts.push_back(".xml");
  exts.push_back(".nxs");
  exts.push_back(".nx5");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus parameters file to read, as a full "
                  "or relative path.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The name of the workspace to be created as output of"
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service.");
}

/** 
 * Executes the algorithm: reads in the parameters file and creates
 * and fills the output workspace
 *
 * @throw runtime_error Thrown if execution fails
 */
void LoadTomoConfig::exec() {
  progress(0, "Opening file...");
  MatrixWorkspace_sptr ws;

  // Throws an approriate exception if there is a problem with file access
  Mantid::NeXus::NXRoot root(getPropertyValue("Filename"));

  // "Open" the same file but with the C++ interface
  m_file = new ::NeXus::File(root.m_fileID);

  progress(1.0, "Loading finished.");
}

} // namespace DataHandling
} // namespace Mantid
