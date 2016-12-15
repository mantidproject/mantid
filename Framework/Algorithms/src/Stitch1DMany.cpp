#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/RebinParamsValidator.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {
DECLARE_ALGORITHM(Stitch1DMany)

/** Initialize the algorithm's properties.
 */
void Stitch1DMany::init() {

  declareProperty(
      make_unique<ArrayProperty<std::string>>("InputWorkspaces",
                                              Direction::Input),
      "Input Workspaces. List of histogram workspaces to stitch together.");

  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output stitched workspace.");

  declareProperty(make_unique<ArrayProperty<double>>(
                      "Params", boost::make_shared<RebinParamsValidator>(true),
                      Direction::Input),
                  "Rebinning Parameters. See Rebin for format.");

  declareProperty(
      make_unique<ArrayProperty<double>>("StartOverlaps", Direction::Input),
      "Start overlaps for stitched workspaces.");

  declareProperty(
      make_unique<ArrayProperty<double>>("EndOverlaps", Direction::Input),
      "End overlaps for stitched workspaces.");

  declareProperty(make_unique<PropertyWithValue<bool>>("ScaleRHSWorkspace",
                                                       true, Direction::Input),
                  "Scaling either with respect to workspace 1 or workspace 2");

  declareProperty(make_unique<PropertyWithValue<bool>>("UseManualScaleFactor",
                                                       false, Direction::Input),
                  "True to use a provided value for the scale factor.");

  auto manualScaleFactorValidator =
      boost::make_shared<BoundedValidator<double>>();
  manualScaleFactorValidator->setLower(0);
  manualScaleFactorValidator->setExclusive(true);
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ManualScaleFactor", 1.0, manualScaleFactorValidator,
                      Direction::Input),
                  "Provided value for the scale factor.");

  declareProperty(
      make_unique<ArrayProperty<double>>("OutScaleFactors", Direction::Output),
      "The actual used values for the scaling factors at each stitch step.");

  auto scaleFactorFromPeriodValidator =
      boost::make_shared<BoundedValidator<int>>();
  scaleFactorFromPeriodValidator->setLower(1);
  declareProperty(make_unique<PropertyWithValue<int>>(
                      "ScaleFactorFromPeriod", 1,
                      scaleFactorFromPeriodValidator, Direction::Input),
                  "Provided index of period to obtain scale factor from.");
}

/** Load and validate the algorithm's properties.
 */
std::map<std::string, std::string> Stitch1DMany::validateInputs() {
  std::map<std::string, std::string> errors;

  const std::vector<std::string> inputWorkspacesStr =
      this->getProperty("InputWorkspaces");

  // Check all workspaces exist
  std::vector<Workspace_sptr> inputWorkspaces;
  for (const auto &ws : inputWorkspacesStr) {
    if (AnalysisDataService::Instance().doesExist(ws)) {
      inputWorkspaces.push_back(
          AnalysisDataService::Instance().retrieveWS<Workspace>(ws));
    } else {
      errors["InputWorkspaces"] = ws + " is not a valid workspace.";
      break;
    }
  }
  m_inputWSMatrix.push_back(inputWorkspaces);

  // Add common errors
  auto commonErrors = validateCommonInputs();
  errors.insert(commonErrors.begin(), commonErrors.end());

  return errors;
}

/** Load and validate the algorithm's properties for workspace groups.
 */
void Stitch1DMany::validateGroupWorkspacesInputs() {
  std::string error;

  const std::vector<std::string> inputWorkspacesStr =
      this->getProperty("InputWorkspaces");

  // Check all workspace groups and their constituent workspaces exist
  for (const auto &groupWSName : inputWorkspacesStr) {
    if (AnalysisDataService::Instance().doesExist(groupWSName)) {

      std::vector<Workspace_sptr> inputWorkspaces;
      auto groupWS = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
          groupWSName);

      for (size_t i = 0; i < groupWS->size(); i++) {
        std::string wsName = groupWS->getItem(i)->name();
        if (AnalysisDataService::Instance().doesExist(wsName)) {
          inputWorkspaces.push_back(
              AnalysisDataService::Instance().retrieveWS<Workspace>(wsName));
        } else {
          throw std::invalid_argument(groupWSName +
                                      " is not a valid workspace.");
        }
      }

      m_inputWSMatrix.push_back(inputWorkspaces);
      m_inputWSGroups.push_back(groupWS);
    } else {
      throw std::invalid_argument(groupWSName +
                                  " is not a valid workspace group.");
    }
  }

  size_t numWSInGroup = m_inputWSMatrix[0].size();

  // Check all workspace groups are the same size
  for (auto &inputWsGroup : m_inputWSGroups) {
    if (inputWsGroup->size() != numWSInGroup) {
      throw std::runtime_error("All workspace groups must be the same size.");
    }
  }

  int scaleFactorFromPeriod = this->getProperty("ScaleFactorFromPeriod");
  m_scaleFactorFromPeriod = (size_t)scaleFactorFromPeriod;
  m_scaleFactorFromPeriod--; // To account for period being indexed from 1
  if (m_scaleFactorFromPeriod >= m_inputWSGroups.size())
    throw std::runtime_error("Period index out of range");

  // Throw if a common error is found
  auto commonErrors = validateCommonInputs();
  if (commonErrors.size() > 0) {
    auto error = commonErrors.begin();
    throw std::runtime_error(error->second);
  }
}

/** Load and validate properties common to both group and non-group workspaces.
*/
std::map<std::string, std::string> Stitch1DMany::validateCommonInputs() {
  std::map<std::string, std::string> errors;

  const std::vector<std::string> inputWorkspacesStr =
      this->getProperty("InputWorkspaces");
  if (inputWorkspacesStr.size() < 2)
    errors["InputWorkspaces"] = "At least 2 input workspaces required.";

  // Check that all the workspaces are of the same type
  if (!m_inputWSMatrix.empty() && !m_inputWSMatrix[0].empty()) {
    const std::string id = m_inputWSMatrix[0][0]->id();
    for (auto &period : m_inputWSMatrix) {
      for (auto &inputWS : period) {
        if (inputWS->id() != id) {
          errors["InputWorkspaces"] = "All workspaces must be the same type.";
          break;
        }
      }
    }
  } else {
    errors["InputWorkspaces"] = "Input workspaces must be given";
  }

  m_startOverlaps = this->getProperty("StartOverlaps");
  m_endOverlaps = this->getProperty("EndOverlaps");
  m_scaleRHSWorkspace = this->getProperty("ScaleRHSWorkspace");
  m_params = this->getProperty("Params");

  m_numWSPerGroup = m_inputWSMatrix[0].size();
  m_numWSPerPeriod = m_inputWSMatrix.size();

  size_t numStitchableWS =
      (m_numWSPerPeriod > 1) ? m_numWSPerPeriod : m_numWSPerGroup;

  if (!m_startOverlaps.empty() && m_startOverlaps.size() != numStitchableWS - 1)
    errors["StartOverlaps"] = "If given, StartOverlaps must have one fewer "
                              "entries than the number of input workspaces.";

  if (m_startOverlaps.size() != m_endOverlaps.size())
    errors["EndOverlaps"] =
        "EndOverlaps must have the same number of entries as StartOverlaps.";

  if (m_params.empty())
    errors["Params"] = "At least one parameter must be given.";

  m_useManualScaleFactor = this->getProperty("UseManualScaleFactor");
  m_manualScaleFactor = this->getProperty("ManualScaleFactor");

  return errors;
}

/** Execute the algorithm.
 */
void Stitch1DMany::exec() {
  MatrixWorkspace_sptr lhsWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(m_inputWSMatrix[0][0]);

  for (size_t i = 1; i < m_numWSPerGroup; i++) {
    MatrixWorkspace_sptr rhsWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(m_inputWSMatrix[0][i]);
    double outScaleFactor;

    doStitch1D(lhsWS, rhsWS, i, m_startOverlaps, m_endOverlaps, m_params,
               m_scaleRHSWorkspace, m_useManualScaleFactor, m_manualScaleFactor,
               lhsWS, outScaleFactor);

    m_scaleFactors.push_back(outScaleFactor);
  }
  m_outputWorkspace = lhsWS;

  // Save output
  this->setProperty("OutputWorkspace", m_outputWorkspace);
  this->setProperty("OutScaleFactors", m_scaleFactors);
}

/** Performs the Stitch1D algorithm at a specific workspace index.
 * @param lhsWS :: The left-hand workspace to be stitched
 * @param rhsWS :: The right-hand workspace to be stitched
 * @param wsIndex :: The index of the rhs workspace being stitched
 * @param startOverlaps :: Start overlaps for stitched workspaces
 * @param endOverlaps :: End overlaps for stitched workspaces
 * @param params :: Rebinning parameters
 * @param scaleRhsWS :: Scaling either with respect to left or right workspaces
 * @param useManualScaleFactor :: True to use a provided value for scale factor
 * @param manualScaleFactor :: Provided value for scaling factor
 * @param outWS :: Output stitched workspace
 * @param outScaleFactor :: Actual value used for scale factor
 */
void Stitch1DMany::doStitch1D(
    MatrixWorkspace_sptr lhsWS, MatrixWorkspace_sptr rhsWS,
    const size_t wsIndex, const std::vector<double> &startOverlaps,
    const std::vector<double> &endOverlaps, const std::vector<double> &params,
    const bool scaleRhsWS, const bool useManualScaleFactor,
    const double manualScaleFactor, MatrixWorkspace_sptr &outWS,
    double &outScaleFactor) {

  IAlgorithm_sptr alg = createChildAlgorithm("Stitch1D");
  alg->initialize();
  alg->setProperty("LHSWorkspace", lhsWS);
  alg->setProperty("RHSWorkspace", rhsWS);
  if (startOverlaps.size() > wsIndex - 1) {
    alg->setProperty("StartOverlap", startOverlaps[wsIndex - 1]);
    alg->setProperty("EndOverlap", endOverlaps[wsIndex - 1]);
  }
  alg->setProperty("Params", params);
  alg->setProperty("ScaleRHSWorkspace", scaleRhsWS);
  alg->setProperty("UseManualScaleFactor", useManualScaleFactor);
  if (useManualScaleFactor)
    alg->setProperty("ManualScaleFactor", manualScaleFactor);
  alg->execute();

  outWS = alg->getProperty("OutputWorkspace");
  outScaleFactor = alg->getProperty("OutScaleFactor");

  if (!isChild()) {
    // Copy each input workspace's history into our output workspace's history
    for (auto &inputWS : m_inputWSMatrix[0]) {
      outWS->history().addHistory(inputWS->getHistory());
    }
  }
}

/** Performs the Stitch1DMany algorithm at a specific period
 * @param inputWSGroups :: The set of workspace groups to be stitched
 * @param period :: The period index we are stitching at
 * @param storeInADS :: True to store in the AnalysisDataService
 * @param startOverlaps :: Start overlaps for stitched workspaces
 * @param endOverlaps :: End overlaps for stitched workspaces
 * @param params :: Rebinning parameters
 * @param scaleRhsWS :: Scaling either with respect to left or right workspaces
 * @param useManualScaleFactor :: True to use a provided value for scale factor
 * @param manualScaleFactor :: Provided value for scaling factor
 * @param outName :: Output stitched workspace name
 * @param outScaleFactors :: Actual values used for scale factors
 */
void Stitch1DMany::doStitch1DMany(
    std::vector<WorkspaceGroup_sptr> inputWSGroups, const size_t period,
    const bool storeInADS, const std::vector<double> &startOverlaps,
    const std::vector<double> &endOverlaps, const std::vector<double> &params,
    const bool scaleRhsWS, const bool useManualScaleFactor,
    const double manualScaleFactor, std::string &outName,
    std::vector<double> &outScaleFactors) {

  // List of workspaces to stitch
  std::vector<std::string> toProcess;

  for (auto &groupWs : inputWSGroups) {
    const std::string wsName = groupWs->getItem(period)->name();
    toProcess.push_back(wsName);
    outName += "_" + wsName;
  }

  IAlgorithm_sptr alg = createChildAlgorithm("Stitch1DMany");
  alg->initialize();
  alg->setChild(false);
  alg->setAlwaysStoreInADS(storeInADS);
  alg->setProperty("InputWorkspaces", toProcess);
  alg->setProperty("OutputWorkspace", outName);
  alg->setProperty("StartOverlaps", startOverlaps);
  alg->setProperty("EndOverlaps", endOverlaps);
  alg->setProperty("Params", params);
  alg->setProperty("ScaleRHSWorkspace", scaleRhsWS);
  alg->setProperty("UseManualScaleFactor", useManualScaleFactor);
  if (useManualScaleFactor)
    alg->setProperty("ManualScaleFactor", manualScaleFactor);
  alg->execute();

  outScaleFactors = alg->getProperty("OutScaleFactors");
}

bool Stitch1DMany::checkGroups() {
  std::vector<std::string> wsNames = getProperty("InputWorkspaces");

  try {
    if (AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsNames[0]))
      return true;
  } catch (...) {
  }
  return false;
}

bool Stitch1DMany::processGroups() {
  validateGroupWorkspacesInputs();

  std::vector<std::string> toGroup; // List of workspaces to be grouped
  std::string groupName = this->getProperty("OutputWorkspace");
  std::string outName;

  // Determine whether or not we are using a global scale factor
  Property *manualSF = this->getProperty("ManualScaleFactor");
  bool usingScaleFromPeriod = m_useManualScaleFactor && manualSF->isDefault();

  if (!usingScaleFromPeriod) {
    for (size_t i = 0; i < m_numWSPerGroup; ++i) {
      outName = groupName;
      std::vector<double> scaleFactors;
      doStitch1DMany(m_inputWSGroups, i, true, m_startOverlaps, m_endOverlaps,
                     m_params, m_scaleRHSWorkspace, m_useManualScaleFactor,
                     m_manualScaleFactor, outName, scaleFactors);

      // Add the resulting workspace to the list to be grouped together
      toGroup.push_back(outName);

      // Add the scalefactors to the list so far
      m_scaleFactors.insert(m_scaleFactors.end(), scaleFactors.begin(),
                            scaleFactors.end());
    }
  } else {
    // Obtain scale factors for the specified period
    outName = groupName;
    std::vector<double> periodScaleFactors;
    doStitch1DMany(m_inputWSGroups, m_scaleFactorFromPeriod, false,
                   m_startOverlaps, m_endOverlaps, m_params,
                   m_scaleRHSWorkspace, false, m_manualScaleFactor, outName,
                   periodScaleFactors);

    // Iterate over each period
    for (size_t i = 0; i < m_numWSPerGroup; i++) {
      auto lhsWS =
          boost::dynamic_pointer_cast<MatrixWorkspace>(m_inputWSMatrix[0][i]);
      outName = groupName + "_" + lhsWS->name();

      // Perform stiching on the workspace for each group of that period
      for (size_t j = 1; j < m_numWSPerPeriod; j++) {
        auto rhsWS =
            boost::dynamic_pointer_cast<MatrixWorkspace>(m_inputWSMatrix[j][i]);
        outName += "_" + rhsWS->name(); // add name
        double outScaleFactor;

        doStitch1D(lhsWS, rhsWS, j, m_startOverlaps, m_endOverlaps, m_params,
                   m_scaleRHSWorkspace, m_useManualScaleFactor,
                   periodScaleFactors[j - 1], lhsWS, outScaleFactor);

        m_scaleFactors.push_back(outScaleFactor);
      }

      // Add name of stitched workspaces to group list and ADS
      toGroup.push_back(outName);
      AnalysisDataService::Instance().addOrReplace(outName, lhsWS);
    }
  }

  IAlgorithm_sptr groupAlg = createChildAlgorithm("GroupWorkspaces");
  groupAlg->initialize();
  groupAlg->setAlwaysStoreInADS(true);
  groupAlg->setProperty("InputWorkspaces", toGroup);
  groupAlg->setProperty("OutputWorkspace", groupName);
  groupAlg->execute();

  m_outputWorkspace =
      AnalysisDataService::Instance().retrieveWS<Workspace>(groupName);

  this->setProperty("OutputWorkspace", m_outputWorkspace);
  this->setProperty("OutScaleFactors", m_scaleFactors);
  return true;
}
} // namespace Algorithms
} // namespace Mantid
