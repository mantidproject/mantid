#include "MantidMDAlgorithms/ChangeQConvention.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDBoxFlatTree.h"
#include "MantidDataObjects/MDBoxIterator.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include <Poco/File.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ChangeQConvention)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ChangeQConvention::init() {
  declareProperty(make_unique<WorkspaceProperty<IMDWorkspace>>(
                      "InputWorkspace", "", Direction::InOut),
                  "An input MDEventWorkspace or MDHistoWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ChangeQConvention::exec() {
  IMDWorkspace_sptr ws = getProperty("InputWorkspace");
  std::string convention = ws->getConvention();

  g_log.information() << "Transforming Q in workspace\n";

  Algorithm_sptr transform_alg = createChildAlgorithm("TransformMD");
  transform_alg->setProperty("InputWorkspace",
                             boost::dynamic_pointer_cast<IMDWorkspace>(ws));
  transform_alg->setProperty("Scaling", "-1.0");
  transform_alg->executeAsChildAlg();
  ws = transform_alg->getProperty("OutputWorkspace");
  ws->setConvention(convention);
  ws->changeQConvention();

  setProperty("InputWorkspace", ws);
}

} // namespace MDAlgorithms
} // namespace Mantid
