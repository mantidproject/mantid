#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/CreateMD.h"
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CreateMD::CreateMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
CreateMD::~CreateMD() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CreateMD::name() const { return "CreateMD"; }

/// Algorithm's version for identification. @see Algorithm::version
int CreateMD::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateMD::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CreateMD::summary() const {
  return "TODO: FILL IN A SUMMARY";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateMD::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateMD::exec() {
  // TODO Auto-generated execute stub
}

/*
 * Load data from file
 */
Mantid::API::Workspace_sptr CreateMD::loadWs(const std::string &filename,
                                             const std::string &wsname) {
  Algorithm_sptr load_alg = createChildAlgorithm("Load");

  load_alg->setProperty("Filename", filename);
  load_alg->setPropertyValue("OutputWorkspace", wsname);
  load_alg->executeAsChildAlg();

  return load_alg->getProperty("OutputWorkspace");
}

/*
 * Set the sample log for the workspace
 */
void CreateMD::addSampleLog(Mantid::API::Workspace_sptr workspace,
                            const std::string &log_name, int log_number) {
  Algorithm_sptr log_alg = createChildAlgorithm("AddSampleLog");

  std::stringstream log_num_str;
  log_num_str << log_number;

  log_alg->setProperty("Workspace", workspace);
  log_alg->setProperty("LogName", log_name);
  log_alg->setProperty("LogText", log_num_str.str());
  log_alg->setProperty("LogType", "Number");

  log_alg->executeAsChildAlg();
}

/*
 * Set the goniometer values for the workspace
 */
void CreateMD::setGoniometer(Mantid::API::Workspace_sptr workspace) {
  Algorithm_sptr log_alg = createChildAlgorithm("SetGoniometer");

  log_alg->setProperty("Workspace", workspace);
  log_alg->setProperty("Axis0", "gl,0,0,1,1");
  log_alg->setProperty("Axis1", "gs,1,0,0,1");
  log_alg->setProperty("Axis2", "psi,0,1,0,1");

  log_alg->executeAsChildAlg();
}

} // namespace MDAlgorithms
} // namespace Mantid
