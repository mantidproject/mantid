#include "MantidCrystal/ClearUB.h"
#include "MantidAPI/MultipleExperimentInfos.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ClearUB)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ClearUB::ClearUB() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ClearUB::~ClearUB() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ClearUB::name() const { return "ClearUB"; };

/// Algorithm's version for identification. @see Algorithm::version
int ClearUB::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string ClearUB::category() const { return "Crystal"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ClearUB::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("Workspace", "", Direction::InOut),
      "Workspace to clear the UB from.");
  declareProperty(
      new PropertyWithValue<bool>("DoesClear", "", Direction::Output),
      "Indicates action performed. DoesClear returns true only if one or more "
      "OrientedLattices have been removed.");
}

/**
 * Clear the Oriented Lattice from a single experiment info.
 * @param experimentInfo : Experiment info to clear.
 * @param dryRun : Flag to indicate that this is a dry run, and that no clearing
 * should take place.
 * @return true only if the UB was cleared.
 */
bool ClearUB::clearSingleExperimentInfo(ExperimentInfo *const experimentInfo,
                                        const bool dryRun) const {
  bool doesClear = false;
  Sample &sampleObject = experimentInfo->mutableSample();
  if (!sampleObject.hasOrientedLattice()) {
    this->g_log.notice("Experiment Info has no oriented lattice.");
  } else {
    this->g_log.notice("Experiment Info has an oriented lattice.");
    // Only actually clear the orientedlattice if this is NOT a dry run.
    if (!dryRun) {
      sampleObject.clearOrientedLattice();
    }
    doesClear = true;
  }
  return doesClear;
}

/**
 * Perform the working for the algorithm.
 * @param ws : Input or input/output workspace
 * @param dryRun : Flag to indicate that this is a dry run.
 * @return : True if the algorithm does result in the clearing of at least one
 * UB.
 */
bool ClearUB::doExecute(Workspace *const ws, bool dryRun) {
  bool doesClear = false;
  ExperimentInfo *experimentInfo = dynamic_cast<ExperimentInfo *>(ws);
  if (experimentInfo) {
    doesClear = clearSingleExperimentInfo(experimentInfo, dryRun);
  } else {
    MultipleExperimentInfos *experimentInfos =
        dynamic_cast<MultipleExperimentInfos *>(ws);
    if (!experimentInfos) {
      if (!dryRun) {
        throw std::invalid_argument("Input workspace is neither of type "
                                    "ExperimentInfo or MultipleExperimentInfo, "
                                    "cannot process.");
      }
    } else {
      const uint16_t nInfos = experimentInfos->getNumExperimentInfo();
      for (uint16_t i = 0; i < nInfos; ++i) {
        ExperimentInfo_sptr info = experimentInfos->getExperimentInfo(i);
        doesClear = clearSingleExperimentInfo(info.get(), dryRun) || doesClear;
      }
    }
  }
  return doesClear;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ClearUB::exec() {
  Workspace_sptr ws = getProperty("Workspace");
  bool doesClear = doExecute(ws.get(), false /* Not a dry run*/);
  this->setProperty("DoesClear", doesClear);
}

} // namespace Crystal
} // namespace Mantid
