#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/BoundedValidator.h"

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
      new ArrayProperty<std::string>("InputWorkspaces", Direction::Input),
      "Input Workspaces. List of histogram workspaces to stitch together.");

  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output stitched workspace.");

  declareProperty(new ArrayProperty<double>(
                      "Params", boost::make_shared<RebinParamsValidator>(true),
                      Direction::Input),
                  "Rebinning Parameters. See Rebin for format.");

  declareProperty(new ArrayProperty<double>("StartOverlaps", Direction::Input),
                  "Start overlaps for stitched workspaces.");

  declareProperty(new ArrayProperty<double>("EndOverlaps", Direction::Input),
                  "End overlaps for stitched workspaces.");

  declareProperty(
      new PropertyWithValue<bool>("ScaleRHSWorkspace", true, Direction::Input),
      "Scaling either with respect to workspace 1 or workspace 2");

  declareProperty(new PropertyWithValue<bool>("UseManualScaleFactor", false,
                                              Direction::Input),
                  "True to use a provided value for the scale factor.");

  auto manualScaleFactorValidator =
      boost::make_shared<BoundedValidator<double>>();
  manualScaleFactorValidator->setLower(0);
  manualScaleFactorValidator->setExclusive(true);
  declareProperty(new PropertyWithValue<double>("ManualScaleFactor", 1.0,
                                                manualScaleFactorValidator,
                                                Direction::Input),
                  "Provided value for the scale factor.");

  declareProperty(
      new ArrayProperty<double>("OutScaleFactors", Direction::Output),
      "The actual used values for the scaling factores at each stitch step.");
}

/** Load and validate the algorithm's properties.
 */
std::map<std::string, std::string> Stitch1DMany::validateInputs() {
  std::map<std::string, std::string> errors;

  m_inputWorkspaces.clear();

  const std::vector<std::string> inputWorkspacesStr =
      this->getProperty("InputWorkspaces");
  if (inputWorkspacesStr.size() < 2)
    errors["InputWorkspaces"] = "At least 2 input workspaces required.";

  for (auto ws = inputWorkspacesStr.begin(); ws != inputWorkspacesStr.end();
       ++ws) {
    if (AnalysisDataService::Instance().doesExist(*ws)) {
      m_inputWorkspaces.push_back(
          AnalysisDataService::Instance().retrieveWS<Workspace>(*ws));
    } else {
      errors["InputWorkspaces"] = *ws + " is not a valid workspace.";
      break;
    }
  }

  // Check that all the workspaces are of the same type
  if (m_inputWorkspaces.size() > 0) {
    const std::string id = m_inputWorkspaces[0]->id();
    for (auto it = m_inputWorkspaces.begin(); it != m_inputWorkspaces.end();
         ++it) {
      if ((*it)->id() != id) {
        errors["InputWorkspaces"] = "All workspaces must be the same type.";
        break;
      }
    }

    // If our inputs are all group workspaces, check they're the same size
    WorkspaceGroup_sptr firstGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(m_inputWorkspaces[0]);
    if (firstGroup) {
      size_t groupSize = firstGroup->size();
      for (auto it = m_inputWorkspaces.begin(); it != m_inputWorkspaces.end();
           ++it) {
        WorkspaceGroup_sptr group =
            boost::dynamic_pointer_cast<WorkspaceGroup>(*it);
        if (group->size() != groupSize) {
          errors["InputWorkspaces"] =
              "All group workspaces must be the same size.";
          break;
        }
      }
    }
  } else {
    errors["InputWorkspaces"] = "Input workspaces must be given";
  }

  m_numWorkspaces = m_inputWorkspaces.size();

  m_startOverlaps = this->getProperty("StartOverlaps");
  m_endOverlaps = this->getProperty("EndOverlaps");

  if (m_startOverlaps.size() > 0 &&
      m_startOverlaps.size() != m_numWorkspaces - 1)
    errors["StartOverlaps"] = "If given, StartOverlaps must have one fewer "
                              "entries than the number of input workspaces.";

  if (m_startOverlaps.size() != m_endOverlaps.size())
    errors["EndOverlaps"] =
        "EndOverlaps must have the same number of entries as StartOverlaps.";

  m_scaleRHSWorkspace = this->getProperty("ScaleRHSWorkspace");
  m_useManualScaleFactor = this->getProperty("UseManualScaleFactor");
  m_manualScaleFactor = this->getProperty("ManualScaleFactor");
  m_params = this->getProperty("Params");

  if (m_params.size() < 1)
    errors["Params"] = "At least one parameter must be given.";

  if (!m_scaleRHSWorkspace) {
    // Flip these around for processing
    std::reverse(m_inputWorkspaces.begin(), m_inputWorkspaces.end());
    std::reverse(m_startOverlaps.begin(), m_startOverlaps.end());
    std::reverse(m_endOverlaps.begin(), m_endOverlaps.end());
  }

  m_scaleFactors.clear();
  m_outputWorkspace.reset();

  return errors;
}

/** Execute the algorithm.
 */
void Stitch1DMany::exec() {
  // Check we're not dealing with group workspaces
  if (!boost::dynamic_pointer_cast<WorkspaceGroup>(m_inputWorkspaces[0])) {
    MatrixWorkspace_sptr lhsWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(m_inputWorkspaces[0]);

    for (size_t i = 1; i < m_numWorkspaces; ++i) {
      MatrixWorkspace_sptr rhsWS =
          boost::dynamic_pointer_cast<MatrixWorkspace>(m_inputWorkspaces[i]);

      IAlgorithm_sptr stitchAlg = createChildAlgorithm("Stitch1D");
      stitchAlg->initialize();

      stitchAlg->setProperty("LHSWorkspace", lhsWS);
      stitchAlg->setProperty("RHSWorkspace", rhsWS);
      if (m_startOverlaps.size() > i - 1) {
        stitchAlg->setProperty("StartOverlap", m_startOverlaps[i - 1]);
        stitchAlg->setProperty("EndOverlap", m_endOverlaps[i - 1]);
      }
      stitchAlg->setProperty("Params", m_params);
      stitchAlg->setProperty("ScaleRHSWorkspace", m_scaleRHSWorkspace);
      stitchAlg->setProperty("UseManualScaleFactor", m_useManualScaleFactor);
      if (m_useManualScaleFactor)
        stitchAlg->setProperty("ManualScaleFactor", m_manualScaleFactor);

      stitchAlg->execute();

      lhsWS = stitchAlg->getProperty("OutputWorkspace");
      m_scaleFactors.push_back(stitchAlg->getProperty("OutScaleFactor"));
    }

    if (!isChild()) {
      // Copy each input workspace's history into our output workspace's history
      for (auto inWS = m_inputWorkspaces.begin();
           inWS != m_inputWorkspaces.end(); ++inWS)
        lhsWS->history().addHistory((*inWS)->getHistory());
    }
    // We're a child algorithm, but we're recording history anyway
    else if (isRecordingHistoryForChild() && m_parentHistory) {
      m_parentHistory->addChildHistory(m_history);
    }

    m_outputWorkspace = lhsWS;
  }
  // We're dealing with group workspaces
  else {
    std::vector<WorkspaceGroup_sptr> groupWorkspaces;
    for (auto it = m_inputWorkspaces.begin(); it != m_inputWorkspaces.end();
         ++it)
      groupWorkspaces.push_back(
          boost::dynamic_pointer_cast<WorkspaceGroup>(*it));

    // List of workspaces to be grouped
    std::vector<std::string> toGroup;

    size_t numWSPerGroup = groupWorkspaces[0]->size();

    for (size_t i = 0; i < numWSPerGroup; ++i) {
      // List of workspaces to stitch
      std::vector<std::string> toProcess;
      // The name of the resulting workspace
      std::string outName;

      for (size_t j = 0; j < groupWorkspaces.size(); ++j) {
        const std::string wsName = groupWorkspaces[j]->getItem(i)->name();
        toProcess.push_back(wsName);
        outName += wsName;
      }

      IAlgorithm_sptr stitchAlg =
          AlgorithmManager::Instance().create("Stitch1DMany");
      stitchAlg->initialize();
      stitchAlg->setProperty("InputWorkspaces", toProcess);
      stitchAlg->setProperty("OutputWorkspace", outName);
      stitchAlg->setProperty("StartOverlaps", m_startOverlaps);
      stitchAlg->setProperty("EndOverlaps", m_endOverlaps);
      stitchAlg->setProperty("Params", m_params);
      stitchAlg->setProperty("ScaleRHSWorkspace", m_scaleRHSWorkspace);
      stitchAlg->setProperty("UseManualScaleFactor", m_useManualScaleFactor);
      if (m_useManualScaleFactor)
        stitchAlg->setProperty("ManualScaleFactor", m_manualScaleFactor);
      stitchAlg->execute();

      // Add the resulting workspace to the list to be grouped together
      toGroup.push_back(outName);

      // Add the scalefactors to the list so far
      const std::vector<double> scaleFactors =
          stitchAlg->getProperty("OutScaleFactors");
      m_scaleFactors.insert(m_scaleFactors.end(), scaleFactors.begin(),
                            scaleFactors.end());
    }

    const std::string groupName = this->getProperty("OutputWorkspace");

    IAlgorithm_sptr groupAlg =
        AlgorithmManager::Instance().create("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setProperty("InputWorkspaces", toGroup);
    groupAlg->setProperty("OutputWorkspace", groupName);
    groupAlg->execute();

    m_outputWorkspace =
        AnalysisDataService::Instance().retrieveWS<Workspace>(groupName);
  }

  // Save output
  this->setProperty("OutputWorkspace", m_outputWorkspace);
  this->setProperty("OutScaleFactors", m_scaleFactors);
}

} // namespace Algorithms
} // namespace Mantid
