// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include <MantidKernel/InvisibleProperty.h>
#include <memory>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid::Algorithms {
DECLARE_ALGORITHM(Stitch1DMany)

/// Initialize the algorithm's properties.
void Stitch1DMany::init() {

  declareProperty(std::make_unique<ArrayProperty<std::string>>("InputWorkspaces", std::make_shared<ADSValidator>()),
                  "List or group of MatrixWorkspaces");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "Stitched workspace.");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("Params", std::make_shared<RebinParamsValidator>(true), Direction::Input),
      "Rebinning Parameters, see Rebin algorithm for format.");

  declareProperty(std::make_unique<ArrayProperty<double>>("StartOverlaps", Direction::Input),
                  "Start overlaps for stitched workspaces "
                  "(number of input workspaces minus one).");

  declareProperty(std::make_unique<ArrayProperty<double>>("EndOverlaps", Direction::Input),
                  "End overlaps for stitched workspaces "
                  "(number of input workspaces minus one).");

  declareProperty(
      std::make_unique<PropertyWithValue<OptionalBool>>("ScaleRHSWorkspace", OptionalBool::Unset, Direction::Input),
      "Scaling either with respect to first (first hand side, LHS) "
      "or second (right hand side, RHS) workspace. "
      "This property no longer has an effect, please use the IndexOfReference property instead.");
  // Hide this property from the GUI as it now has no effect and will eventually be removed
  setPropertySettings("ScaleRHSWorkspace", std::make_unique<InvisibleProperty>());

  declareProperty(std::make_unique<PropertyWithValue<bool>>("UseManualScaleFactors", false, Direction::Input),
                  "True to use provided values for the scale factor.");

  declareProperty(std::make_unique<ArrayProperty<double>>("ManualScaleFactors", Direction::Input),
                  "Either a single scale factor which will be applied to all "
                  "input workspaces or individual scale factors "
                  "(number of input workspaces minus one)");
  setPropertySettings("ManualScaleFactors",
                      std::make_unique<VisibleWhenProperty>("UseManualScaleFactors", IS_EQUAL_TO, "1"));

  declareProperty(std::make_unique<ArrayProperty<double>>("OutScaleFactors", Direction::Output),
                  "The actual used values for the scaling factors at each stitch step.");

  auto scaleFactorFromPeriodValidator = std::make_shared<BoundedValidator<int>>();
  scaleFactorFromPeriodValidator->setLower(1);
  declareProperty(std::make_unique<PropertyWithValue<int>>("ScaleFactorFromPeriod", 1, scaleFactorFromPeriodValidator,
                                                           Direction::Input),
                  "Provided index of period to obtain scale factor from; "
                  "periods are indexed from 1 and used only if stitching group "
                  "workspaces, UseManualScaleFactors is true and "
                  "ManualScaleFactors is set to default.");

  auto useManualScaleFactorsTrue = VisibleWhenProperty("UseManualScaleFactors", IS_EQUAL_TO, "1");
  auto manualScaleFactorsDefault = VisibleWhenProperty("ManualScaleFactors", IS_DEFAULT);
  auto scaleFactorFromPeriodVisible =
      std::make_unique<VisibleWhenProperty>(useManualScaleFactorsTrue, manualScaleFactorsDefault, AND);

  setPropertySettings("ScaleFactorFromPeriod", std::move(scaleFactorFromPeriodVisible));

  auto indexValidator = std::make_shared<BoundedValidator<int>>();
  indexValidator->setLower(-1);
  declareProperty("IndexOfReference", 0, indexValidator,
                  "Index of the workspace to be used as reference for scaling, or -1 to choose the last workspace as "
                  "the reference.");
}

/// Load and validate the algorithm's properties.
std::map<std::string, std::string> Stitch1DMany::validateInputs() {
  std::map<std::string, std::string> issues;
  const std::vector<std::string> inputWorkspacesStr = this->getProperty("InputWorkspaces");
  if (inputWorkspacesStr.size() < 2)
    issues["InputWorkspaces"] = "Nothing to stitch";
  else {
    try { // input workspaces must be group or MatrixWorkspaces
      auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspacesStr);
      /*
       * Column:    one column of MatrixWorkspaces or several columns of
       * MatrixWorkspaces from a group
       * Row:       each period only for groups
       */
      m_inputWSMatrix.reserve(inputWorkspacesStr.size());
      /// When the StitchIDMany algorithm is called from the GUI, the validateInputs method is called twice.
      /// This causes an unintended side effect on the m_inputWSMatrix variable.
      /// To fix this, the clear method is called on m_inputWSMatrix to ensure it is always empty at this point.
      m_inputWSMatrix.clear();
      std::vector<MatrixWorkspace_sptr> column;
      for (const auto &ws : inputWorkspacesStr) {
        auto groupWS = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(ws);
        if (groupWS) {
          for (size_t i = 0; i < groupWS->size(); i++) {
            auto inputMatrix = std::dynamic_pointer_cast<MatrixWorkspace>(groupWS->getItem(i));
            if (inputMatrix) {
              column.emplace_back(inputMatrix);
            } else
              issues["InputWorkspaces"] = "Input workspace " + ws + " must be a MatrixWorkspace";
          }
          m_inputWSMatrix.emplace_back(column);
          column.clear();
        } else {
          auto inputMatrix = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(ws);
          column.emplace_back(inputMatrix);
        }
      }
      if (!isDefault("IndexOfReference")) {
        m_indexOfReference = static_cast<int>(this->getProperty("IndexOfReference"));
        if (m_indexOfReference > -1 && static_cast<size_t>(m_indexOfReference) >= column.size() &&
            static_cast<size_t>(m_indexOfReference) >= m_inputWSMatrix.size()) {
          issues["IndexOfReference"] = "The index of reference workspace is larger than the number of "
                                       "provided workspaces.";
        }
      }

      if (m_inputWSMatrix.empty()) { // no group workspaces
        // A column of matrix workspaces will be stitched
        RunCombinationHelper combHelper;
        combHelper.setReferenceProperties(column.front());
        for (const auto &ws : column) {
          // check if all the others are compatible with the reference
          std::string compatible = combHelper.checkCompatibility(ws, true);
          if (!compatible.empty()) {
            if (!(compatible == "spectra must have either Dx values or not; ") ||
                (ws->isHistogramData())) // Issue only for point data
              issues["RHSWorkspace"] = "Workspace " + ws->getName() + " is not compatible: " + compatible + "\n";
          }
        }
        m_inputWSMatrix.emplace_back(column);
      } else if (m_inputWSMatrix.size() != inputWorkspacesStr.size()) { // not only group workspaces
        issues["InputWorkspaces"] = "All input workspaces must be groups";
      } else { // only group workspaces
        // Each row of matrix workspaces will be stitched
        for (size_t spec = 1; spec < m_inputWSMatrix.front().size(); ++spec) {
          for (const auto &ws : m_inputWSMatrix) {
            if (ws.size() != m_inputWSMatrix.front().size()) {
              issues["InputWorkspaces"] = "Size mismatch of group workspaces";
            } else {
              RunCombinationHelper combHelper;
              combHelper.setReferenceProperties(ws[0]);
              // check if all the others are compatible with the reference
              std::string compatible = combHelper.checkCompatibility(ws[spec], true);
              if (!compatible.empty())
                issues["InputWorkspaces"] =
                    "Workspace " + ws[spec]->getName() + " is not compatible: " + compatible + "\n";
            }
          }
        }
        int scaleFactorFromPeriod = this->getProperty("ScaleFactorFromPeriod");
        // Period -1 corresponds to workspace index
        m_scaleFactorFromPeriod = static_cast<size_t>(--scaleFactorFromPeriod);
        if (m_scaleFactorFromPeriod >= m_inputWSMatrix.front().size()) {
          std::stringstream expectedRange;
          expectedRange << m_inputWSMatrix.front().size() + 1;
          issues["ScaleFactorFromPeriod"] = "Period index out of range, must be smaller than " + expectedRange.str();
        }
      }

      m_startOverlaps = this->getProperty("StartOverlaps");
      m_endOverlaps = this->getProperty("EndOverlaps");
      m_params = this->getProperty("Params");

      // Either stitch MatrixWorkspaces or workspaces of the group
      size_t numStitchableWS =
          (workspaces.size() == inputWorkspacesStr.size()) ? workspaces.size() : inputWorkspacesStr.size();
      std::stringstream expectedVal;
      expectedVal << numStitchableWS - 1;
      if (!m_startOverlaps.empty() && m_startOverlaps.size() != numStitchableWS - 1)
        issues["StartOverlaps"] = "Expected " + expectedVal.str() + " value(s)";

      if (m_startOverlaps.size() != m_endOverlaps.size())
        issues["EndOverlaps"] = "EndOverlaps must have the same number of "
                                "entries as StartOverlaps.";

      m_useManualScaleFactors = this->getProperty("UseManualScaleFactors");
      m_manualScaleFactors = this->getProperty("ManualScaleFactors");

      if (!m_manualScaleFactors.empty()) {
        if (m_manualScaleFactors.size() == 1) {
          // Single value: fill with list of the same scale factor value
          m_manualScaleFactors = std::vector<double>(numStitchableWS - 1, m_manualScaleFactors.front());
        } else if (m_manualScaleFactors.size() != numStitchableWS - 1) {
          if ((numStitchableWS - 1) == 1)
            issues["ManualScaleFactors"] = "Must be a single value";
          else
            issues["ManualScaleFactors"] =
                "Must be a single value for all input workspaces or " + expectedVal.str() + " values";
        }
      } else { // if not a group, no period scaling possible
        if (m_useManualScaleFactors && (m_inputWSMatrix.size() == 1))
          issues["ManualScaleFactors"] = "Must contain scale factors";
      }
    } catch (const std::exception &e) {
      issues["InputWorkspaces"] = std::string(e.what());
    }
  }
  return issues;
}

/// Execute the algorithm.
void Stitch1DMany::exec() {
  if (static_cast<OptionalBool>(this->getProperty("ScaleRHSWorkspace")) != OptionalBool::Unset) {
    g_log.warning("The ScaleRHSWorkspace property no longer has any effect. Please see documentation on the "
                  "IndexOfReference parameter and use that instead.");
  }

  if (m_inputWSMatrix.size() > 1) {   // groups
    std::vector<std::string> toGroup; // List of workspaces to be grouped
    std::string groupName = this->getProperty("OutputWorkspace");
    std::string outName;

    // Determine whether or not we are scaling workspaces using scale
    // factors from a specific period
    const bool usingScaleFromPeriod = m_useManualScaleFactors && isDefault("ManualScaleFactors");
    if (!usingScaleFromPeriod) {
      for (size_t i = 0; i < m_inputWSMatrix.front().size(); ++i) {

        outName = groupName;
        std::vector<double> scaleFactors;
        doStitch1DMany(i, m_useManualScaleFactors, outName, scaleFactors, m_indexOfReference);
        // Add the resulting workspace to the list to be grouped together
        toGroup.emplace_back(outName);

        // Add the scalefactors to the list so far
        m_scaleFactors.insert(m_scaleFactors.end(), scaleFactors.begin(), scaleFactors.end());
      }
    } else {
      // Obtain scale factors for the specified period
      std::string tempOutName;
      std::vector<double> periodScaleFactors;
      constexpr bool storeInADS = false;

      doStitch1DMany(m_scaleFactorFromPeriod, false, tempOutName, periodScaleFactors, m_indexOfReference, storeInADS);

      // Iterate over each period
      for (size_t i = 0; i < m_inputWSMatrix.front().size(); ++i) {
        std::vector<MatrixWorkspace_sptr> inMatrix;
        inMatrix.reserve(m_inputWSMatrix.size());

        std::transform(m_inputWSMatrix.begin(), m_inputWSMatrix.end(), std::back_inserter(inMatrix),
                       [i](const auto &ws) { return ws[i]; });

        Workspace_sptr outStitchedWS;
        doStitch1D(inMatrix, periodScaleFactors, outStitchedWS);
        // Add name of stitched workspaces to group list and ADS
        outName = createChildWorkspaceName(groupName, i);
        toGroup.emplace_back(outName);
        AnalysisDataService::Instance().addOrReplace(outName, outStitchedWS);
      }
    }

    auto groupAlg = createChildAlgorithm("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setAlwaysStoreInADS(true);
    groupAlg->setProperty("InputWorkspaces", toGroup);
    groupAlg->setProperty("OutputWorkspace", groupName);
    groupAlg->execute();

    m_outputWorkspace = AnalysisDataService::Instance().retrieveWS<Workspace>(groupName);
  } else {
    doStitch1D(m_inputWSMatrix.front(), m_manualScaleFactors, m_outputWorkspace);
  }
  // Save output
  this->setProperty("OutputWorkspace", m_outputWorkspace);
  this->setProperty("OutScaleFactors", m_scaleFactors);
}

/** Performs the Stitch1D algorithm at a specific workspace index.
 * @param toStitch :: Vector of workspaces to be stitched
 * @param manualScaleFactors :: Provided values for scaling factors
 * @param outWS :: Output stitched workspace
 */
void Stitch1DMany::doStitch1D(std::vector<MatrixWorkspace_sptr> &toStitch,
                              const std::vector<double> &manualScaleFactors, Workspace_sptr &outWS) {

  auto &lhsWS = toStitch.front();
  // Support Python list syntax for selecting the last element in the list
  auto indexOfReference = m_indexOfReference == -1 ? toStitch.size() - 1 : m_indexOfReference;

  for (size_t i = 1; i < toStitch.size(); i++) {
    auto rhsWS = toStitch[i];
    // Scale the LHS ws until we have scaled to the reference ws. After that we scale the RHS ws to keep the scaling.
    auto scaleRHSWorkspace = i > indexOfReference;

    auto alg = createChildAlgorithm("Stitch1D");
    alg->initialize();
    alg->setProperty("LHSWorkspace", lhsWS);
    alg->setProperty("RHSWorkspace", rhsWS);
    if (m_startOverlaps.size() > i - 1) {
      alg->setProperty("StartOverlap", m_startOverlaps[i - 1]);
      alg->setProperty("EndOverlap", m_endOverlaps[i - 1]);
    }
    alg->setProperty("Params", m_params);
    alg->setProperty("ScaleRHSWorkspace", scaleRHSWorkspace);
    alg->setProperty("UseManualScaleFactor", m_useManualScaleFactors);
    if (m_useManualScaleFactors)
      alg->setProperty("ManualScaleFactor", manualScaleFactors[i - 1]);
    alg->execute();

    lhsWS = alg->getProperty("OutputWorkspace");
    double outScaleFactor = alg->getProperty("OutScaleFactor");
    m_scaleFactors.emplace_back(outScaleFactor);

    if (!isChild()) {
      // Copy each input workspace's history into our output workspace's
      // history
      for (const auto &inputWS : toStitch) {
        lhsWS->history().addHistory(inputWS->getHistory());
      }
    }
  }

  outWS = lhsWS;
}

/** Performs the Stitch1DMany algorithm at a specific period
 * @param period :: The period index we are stitching at
 * @param useManualScaleFactors :: True to use provided values for scale
 * factors
 * @param outName :: Output stitched workspace name
 * @param outScaleFactors :: Actual values used for scale factors
 * @param indexOfReference :: Index of reference for scaling
 * @param storeInADS :: Whether to store in ADS or not
 */
void Stitch1DMany::doStitch1DMany(const size_t period, const bool useManualScaleFactors, std::string &outName,
                                  std::vector<double> &outScaleFactors, const int indexOfReference,
                                  const bool storeInADS) {

  // List of workspaces to stitch
  std::vector<std::string> toProcess;

  for (const auto &ws : m_inputWSMatrix) {
    const std::string &wsName = ws[period]->getName();
    toProcess.emplace_back(wsName);
  }

  outName = createChildWorkspaceName(outName, period);

  auto alg = createChildAlgorithm("Stitch1DMany");
  alg->initialize();
  alg->setAlwaysStoreInADS(storeInADS);
  alg->setProperty("InputWorkspaces", toProcess);
  if (!outName.empty())
    alg->setProperty("OutputWorkspace", outName);
  alg->setProperty("StartOverlaps", m_startOverlaps);
  alg->setProperty("EndOverlaps", m_endOverlaps);
  alg->setProperty("Params", m_params);
  alg->setProperty("UseManualScaleFactors", useManualScaleFactors);
  if (useManualScaleFactors)
    alg->setProperty("ManualScaleFactors", m_manualScaleFactors);
  alg->setProperty("IndexOfReference", indexOfReference);
  alg->execute();

  outScaleFactors = alg->getProperty("OutScaleFactors");
}

/** Creates a correctly formatted name for a stitched child workspace in a workspace group
 * @param groupName :: The name of the workspace group
 * @param periodIndex :: The period index for the child workspace
 */
std::string Stitch1DMany::createChildWorkspaceName(const std::string &groupName, const size_t periodIndex) {
  return groupName + "_" + std::to_string(periodIndex + 1);
}

} // namespace Mantid::Algorithms
