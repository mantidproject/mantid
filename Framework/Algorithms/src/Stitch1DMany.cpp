#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

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
      Kernel::make_unique<ArrayProperty<std::string>>(
          "InputWorkspaces", boost::make_shared<ADSValidator>()),
      "Input Workspaces. List of histogram workspaces to stitch together. At "
      "least 2 workspaces must be supplied for stitching and all must be "
      "either Matrix Workspaces or Workspace Groups containing Matrix "
      "Workspaces only.");

  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output stitched workspace.");

  declareProperty(make_unique<ArrayProperty<double>>(
                      "Params", boost::make_shared<RebinParamsValidator>(false),
                      Direction::Input),
                  "Rebinning Parameters. See Rebin for format.");

  declareProperty(
      make_unique<ArrayProperty<double>>("StartOverlaps", Direction::Input),
      "Start overlaps for stitched workspaces. If specified, the number of "
      "StartOverlaps must be 1 less than the number of input workspaces. "
      "Optional.");

  declareProperty(
      make_unique<ArrayProperty<double>>("EndOverlaps", Direction::Input),
      "End overlaps for stitched workspaces. If specified, the number of "
      "EndOverlaps must be the same as the number of StartOverlaps. Optional.");

  declareProperty(make_unique<PropertyWithValue<bool>>("ScaleRHSWorkspace",
                                                       true, Direction::Input),
                  "Scaling either with respect to workspace 1 or workspace 2");

  declareProperty(make_unique<PropertyWithValue<bool>>("UseManualScaleFactors",
                                                       false, Direction::Input),
                  "True to use provided values for the scale factor.");

  declareProperty(make_unique<ArrayProperty<double>>("ManualScaleFactors",
                                                     Direction::Input),
                  "Provided values for the scale factors. If specified, the "
                  "number of ManualScaleFactors must either be one (in which "
                  "case the provided value is applied to all input workspaces) "
                  "or 1 less than the number of input workspaces");
  setPropertySettings("ManualScaleFactors",
                      make_unique<VisibleWhenProperty>("UseManualScaleFactors",
                                                       IS_EQUAL_TO, "1"));

  declareProperty(
      make_unique<ArrayProperty<double>>("OutScaleFactors", Direction::Output),
      "The actual used values for the scaling factors at each stitch step.");

  auto scaleFactorFromPeriodValidator =
      boost::make_shared<BoundedValidator<int>>();
  scaleFactorFromPeriodValidator->setLower(1);
  declareProperty(make_unique<PropertyWithValue<int>>(
                      "ScaleFactorFromPeriod", 1,
                      scaleFactorFromPeriodValidator, Direction::Input),
                  "Provided index of period to obtain scale factor from. "
                  "Periods are indexed from 1. Used only if stitching group "
                  "workspaces, UseManualScaleFactors is true and "
                  "ManualScaleFactors is set to default.");

  auto useManualScaleFactorsTrue =
      VisibleWhenProperty("UseManualScaleFactors", IS_EQUAL_TO, "1");
  auto manualScaleFactorsDefault =
      VisibleWhenProperty("ManualScaleFactors", IS_DEFAULT);
  auto scaleFactorFromPeriodVisible = make_unique<VisibleWhenProperty>(
      useManualScaleFactorsTrue, manualScaleFactorsDefault, AND);

  setPropertySettings("ScaleFactorFromPeriod",
                      std::move(scaleFactorFromPeriodVisible));
}

/** Load and validate the algorithm's properties.
 */
std::map<std::string, std::string> Stitch1DMany::validateInputs() {
  std::map<std::string, std::string> errors;

  const std::vector<std::string> inputWorkspacesStr =
      this->getProperty("InputWorkspaces");

  // Add all input workspaces to a single row in the matrix
  // Each 'row' are the workspaces belonging to a specific group
  // Each 'column' are the workspaces belonging to a specific period
  std::vector<MatrixWorkspace_sptr> inputWorkspaces;
  for (const auto &ws : inputWorkspacesStr) {

    auto inputMatrixWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(ws);
    if (inputMatrixWs) {
      inputWorkspaces.push_back(inputMatrixWs);
    } else {
      errors["InputWorkspaces"] = "Input workspaces must either be matrix "
                                  "workspaces or group workspaces containing "
                                  "matrix workspaces only";
      break;
    }
  }
  // Do not add workspaces if an error was found
  if (!errors.count("InputWorkspaces"))
    m_inputWSMatrix.push_back(inputWorkspaces);

  m_numWSPerGroup = inputWorkspacesStr.size();
  m_numWSPerPeriod = 1;

  // Add common errors
  validateCommonInputs(errors);
  errors.insert(errors.begin(), errors.end());

  return errors;
}

/** Load and validate the algorithm's properties for workspace groups.
 */
void Stitch1DMany::validateGroupWorkspacesInputs() {
  std::map<std::string, std::string> errors;

  const std::vector<std::string> inputWorkspacesStr =
      this->getProperty("InputWorkspaces");

  // Add all group workspaces and their constituent workspaces to their
  // respective containers
  for (const auto &groupWSName : inputWorkspacesStr) {

    auto groupWS =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(groupWSName);
    if (groupWS) {
      m_inputWSGroups.push_back(groupWS);
    } else {
      errors["InputWorkspaces"] =
          "Input workspaces must either be matrix workspaces or group "
          "workspaces containing matrix workspaces only";
      break;
    }
  }

  m_numWSPerGroup = m_inputWSGroups.front()->size();
  m_numWSPerPeriod = inputWorkspacesStr.size();

  // Check all workspace groups are the same size
  if (!errors.count("InputWorkspaces")) {
    for (const auto &inputWsGroup : m_inputWSGroups) {
      if (inputWsGroup->size() != m_numWSPerGroup) {
        errors["InputWorkspaces"] =
            "All workspace groups must be the same size.";
        break;
      }
    }
  }

  if (!errors.count("InputWorkspaces")) {
    // Each 'row' are the workspaces belonging to a specific period
    // Each 'column' are the workspaces belonging to a specific group
    for (size_t i = 0; i < m_numWSPerGroup; i++) {

      std::vector<MatrixWorkspace_sptr> inputWorkspaces;
      for (const auto &groupWS : m_inputWSGroups) {

        auto inputMatrixWs =
            boost::dynamic_pointer_cast<MatrixWorkspace>(groupWS->getItem(i));
        if (inputMatrixWs) {
          inputWorkspaces.push_back(inputMatrixWs);
        } else {
          errors["InputWorkspaces"] =
              "Input workspaces must either be matrix workspaces or group "
              "workspaces containing matrix workspaces only";
          break;
        }
      }

      if (errors.count("InputWorkspaces"))
        break; // error found, stop adding workspaces

      m_inputWSMatrix.push_back(inputWorkspaces);
    }
  }

  int scaleFactorFromPeriod = this->getProperty("ScaleFactorFromPeriod");
  m_scaleFactorFromPeriod = static_cast<size_t>(scaleFactorFromPeriod);
  m_scaleFactorFromPeriod--; // To account for period being indexed from 1
  if (m_scaleFactorFromPeriod >= m_inputWSGroups.size())
    errors["ScaleFactorFromPeriod"] = "Period index out of range";

  // Log all errors and throw a runtime error if an error is found
  validateCommonInputs(errors);
  if (errors.size() > 0) {
    auto &warnLog = getLogger().warning();
    for (const auto &error : errors) {
      warnLog << "Invalid value for " << error.first << ": " << error.second
              << "\n";
    }
    throw std::runtime_error("Some invalid Properties found");
  }
}

/** Load and validate properties common to both group and non-group workspaces.
*/
void Stitch1DMany::validateCommonInputs(
    std::map<std::string, std::string> &errors) {

  size_t numStitchableWS =
      (m_numWSPerPeriod > 1) ? m_numWSPerPeriod : m_numWSPerGroup;
  if (numStitchableWS < 2)
    errors["InputWorkspaces"] = "At least 2 input workspaces required.";

  m_startOverlaps = this->getProperty("StartOverlaps");
  m_endOverlaps = this->getProperty("EndOverlaps");
  m_scaleRHSWorkspace = this->getProperty("ScaleRHSWorkspace");
  m_params = this->getProperty("Params");

  if (!m_startOverlaps.empty() && m_startOverlaps.size() != numStitchableWS - 1)
    errors["StartOverlaps"] = "If given, StartOverlaps must have one fewer "
                              "entries than the number of input workspaces.";

  if (m_startOverlaps.size() != m_endOverlaps.size())
    errors["EndOverlaps"] =
        "EndOverlaps must have the same number of entries as StartOverlaps.";

  m_useManualScaleFactors = this->getProperty("UseManualScaleFactors");
  m_manualScaleFactors = this->getProperty("ManualScaleFactors");

  if (m_manualScaleFactors.size() > 0) {
    if (m_manualScaleFactors.size() == 1) {
      // Single value: fill with list of the same scale factor value
      m_manualScaleFactors = std::vector<double>(numStitchableWS - 1,
                                                 m_manualScaleFactors.front());
    } else if (m_manualScaleFactors.size() != numStitchableWS - 1) {
      errors["ManualScaleFactors"] =
          "If given, ManualScaleFactors must either "
          "consist of one entry or one fewer entries "
          "than the number of input workspaces";
    }
  }
}

/** Execute the algorithm.
 */
void Stitch1DMany::exec() {

  std::string tempOutName;

  doStitch1D(m_inputWSMatrix.front(), m_startOverlaps, m_endOverlaps, m_params,
             m_scaleRHSWorkspace, m_useManualScaleFactors, m_manualScaleFactors,
             m_outputWorkspace, tempOutName, m_scaleFactors);

  // Save output
  this->setProperty("OutputWorkspace", m_outputWorkspace);
  this->setProperty("OutScaleFactors", m_scaleFactors);
}

/** Performs the Stitch1D algorithm at a specific workspace index.
 * @param toStitch :: Vector of workspaces to be stitched
 * @param startOverlaps :: Start overlaps for stitched workspaces
 * @param endOverlaps :: End overlaps for stitched workspaces
 * @param params :: Rebinning parameters
 * @param scaleRhsWS :: Scaling either with respect to left or right workspaces
 * @param useManualScaleFactors :: True to use provided values for scale factors
 * @param manualScaleFactors :: Provided values for scaling factors
 * @param outWS :: Output stitched workspace
 * @param outName :: Output stitched workspace name
 * @param outScaleFactors :: Actual values used for scale factors
 */
void Stitch1DMany::doStitch1D(const std::vector<MatrixWorkspace_sptr> &toStitch,
                              const std::vector<double> &startOverlaps,
                              const std::vector<double> &endOverlaps,
                              const std::vector<double> &params,
                              const bool scaleRhsWS,
                              const bool useManualScaleFactors,
                              const std::vector<double> &manualScaleFactors,
                              Workspace_sptr &outWS, std::string &outName,
                              std::vector<double> &outScaleFactors) {

  auto lhsWS = toStitch.front();
  outName += "_" + lhsWS->getName();

  for (size_t i = 1; i < toStitch.size(); i++) {

    auto rhsWS = toStitch[i];
    outName += "_" + rhsWS->getName();

    IAlgorithm_sptr alg = createChildAlgorithm("Stitch1D");
    alg->initialize();
    alg->setProperty("LHSWorkspace", lhsWS);
    alg->setProperty("RHSWorkspace", rhsWS);
    if (startOverlaps.size() > i - 1) {
      alg->setProperty("StartOverlap", startOverlaps[i - 1]);
      alg->setProperty("EndOverlap", endOverlaps[i - 1]);
    }
    alg->setProperty("Params", params);
    alg->setProperty("ScaleRHSWorkspace", scaleRhsWS);
    alg->setProperty("UseManualScaleFactor", useManualScaleFactors);
    if (useManualScaleFactors)
      alg->setProperty("ManualScaleFactor", manualScaleFactors[i - 1]);
    alg->execute();

    lhsWS = alg->getProperty("OutputWorkspace");
    double outScaleFactor = alg->getProperty("OutScaleFactor");
    outScaleFactors.push_back(outScaleFactor);

    if (!isChild()) {
      // Copy each input workspace's history into our output workspace's history
      for (const auto &inputWS : toStitch) {
        lhsWS->history().addHistory(inputWS->getHistory());
      }
    }
  }

  outWS = lhsWS;
}

/** Performs the Stitch1DMany algorithm at a specific period
 * @param inputWSGroups :: The set of workspace groups to be stitched
 * @param period :: The period index we are stitching at
 * @param storeInADS :: True to store in the AnalysisDataService
 * @param startOverlaps :: Start overlaps for stitched workspaces
 * @param endOverlaps :: End overlaps for stitched workspaces
 * @param params :: Rebinning parameters
 * @param scaleRhsWS :: Scaling either with respect to left or right workspaces
 * @param useManualScaleFactors :: True to use provided values for scale factors
 * @param manualScaleFactors :: Provided values for scaling factors
 * @param outName :: Output stitched workspace name
 * @param outScaleFactors :: Actual values used for scale factors
 */
void Stitch1DMany::doStitch1DMany(
    std::vector<WorkspaceGroup_sptr> inputWSGroups, const size_t period,
    const bool storeInADS, const std::vector<double> &startOverlaps,
    const std::vector<double> &endOverlaps, const std::vector<double> &params,
    const bool scaleRhsWS, const bool useManualScaleFactors,
    const std::vector<double> &manualScaleFactors, std::string &outName,
    std::vector<double> &outScaleFactors) {

  // List of workspaces to stitch
  std::vector<std::string> toProcess;

  for (const auto &groupWs : inputWSGroups) {
    const std::string &wsName = groupWs->getItem(period)->getName();
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
  alg->setProperty("UseManualScaleFactors", useManualScaleFactors);
  if (useManualScaleFactors)
    alg->setProperty("ManualScaleFactors", manualScaleFactors);
  alg->execute();

  outScaleFactors = alg->getProperty("OutScaleFactors");
}

bool Stitch1DMany::checkGroups() {
  std::vector<std::string> wsNames = getProperty("InputWorkspaces");

  try {
    if (AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            wsNames.front()))
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

  // Determine whether or not we are scaling workspaces using scale factors from
  // a specific period
  Property *manualSF = this->getProperty("ManualScaleFactors");
  bool usingScaleFromPeriod = m_useManualScaleFactors && manualSF->isDefault();

  if (!usingScaleFromPeriod) {
    for (size_t i = 0; i < m_numWSPerGroup; ++i) {

      outName = groupName;
      std::vector<double> scaleFactors;
      doStitch1DMany(m_inputWSGroups, i, true, m_startOverlaps, m_endOverlaps,
                     m_params, m_scaleRHSWorkspace, m_useManualScaleFactors,
                     m_manualScaleFactors, outName, scaleFactors);

      // Add the resulting workspace to the list to be grouped together
      toGroup.push_back(outName);

      // Add the scalefactors to the list so far
      m_scaleFactors.insert(m_scaleFactors.end(), scaleFactors.begin(),
                            scaleFactors.end());
    }
  } else {
    // Obtain scale factors for the specified period
    std::string tempOutName;
    std::vector<double> periodScaleFactors;

    doStitch1DMany(m_inputWSGroups, m_scaleFactorFromPeriod, false,
                   m_startOverlaps, m_endOverlaps, m_params,
                   m_scaleRHSWorkspace, false, m_manualScaleFactors,
                   tempOutName, periodScaleFactors);

    // Iterate over each period
    for (size_t i = 0; i < m_numWSPerGroup; i++) {

      outName = groupName;
      Workspace_sptr outStitchedWS;

      doStitch1D(m_inputWSMatrix[i], m_startOverlaps, m_endOverlaps, m_params,
                 m_scaleRHSWorkspace, m_useManualScaleFactors,
                 periodScaleFactors, outStitchedWS, outName, m_scaleFactors);

      // Add name of stitched workspaces to group list and ADS
      toGroup.push_back(outName);
      AnalysisDataService::Instance().addOrReplace(outName, outStitchedWS);
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
