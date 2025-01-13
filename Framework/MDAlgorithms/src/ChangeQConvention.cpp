// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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
#include <Poco/File.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid::MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ChangeQConvention)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ChangeQConvention::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDWorkspace>>("InputWorkspace", "", Direction::InOut),
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
  transform_alg->setProperty("InputWorkspace", std::dynamic_pointer_cast<IMDWorkspace>(ws));
  transform_alg->setProperty("Scaling", "-1.0");
  transform_alg->executeAsChildAlg();
  ws = transform_alg->getProperty("OutputWorkspace");
  ws->setConvention(convention);
  ws->changeQConvention();

  setProperty("InputWorkspace", ws);
}

} // namespace Mantid::MDAlgorithms
