#include "WorkspaceAlgorithm.h"

namespace Mantid {
namespace Algorithms {

// Algorithm must be declared
DECLARE_ALGORITHM(WorkspaceAlgorithm)

using namespace Kernel;
using namespace API;

/**  Initialization code
 *
 *   Properties have to be declared here before they can be used
*/
void WorkspaceAlgorithm::init() {

  // Declare a 1D workspace property.
  declareProperty(new WorkspaceProperty<>("Workspace", "", Direction::Input));
}

/** Executes the algorithm
 */
void WorkspaceAlgorithm::exec() {
  // g_log is a reference to the logger. It is used to print out information,
  // warning, and error messages
  g_log.information() << "Running algorithm " << name() << " version "
                      << version() << std::endl;

  // Get the input workspace
  MatrixWorkspace_const_sptr workspace = getProperty("Workspace");

  // Number of single indexable items in the workspace
  g_log.information() << "Number of items = " << workspace->size() << std::endl;

  int count = 0;
  size_t histogramCount = workspace->getNumberHistograms();
  for (size_t i = 0; i < histogramCount; ++i) {
    const MantidVec &XValues = workspace->readX(i);
    const MantidVec &YValues = workspace->readY(i);
    const MantidVec &EValues = workspace->readE(i);

    for (size_t j = 0; j < workspace->blocksize(); ++j) {
      g_log.information() << "Point number " << count++
                          << " values: " << XValues[j] << ' ' << YValues[j]
                          << ' ' << EValues[j] << std::endl;
    }
  }
}
}
}
