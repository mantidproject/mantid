// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"

#include <algorithm>
#include <optional>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr spinAsymmetryWorkspacePrefix = "__isis_reflectometry_spin_asymmetry_";

Mantid::API::IAlgorithm_sptr createAlgorithm(std::string const &name) {
  auto algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(name);
  algorithm->initialize();
  algorithm->setLogging(false);
  return algorithm;
}

void executeBinaryAlgorithm(std::string const &algorithmName, std::string const &lhsWorkspace,
                            std::string const &rhsWorkspace, std::string const &outputWorkspace) {
  auto algorithm = createAlgorithm(algorithmName);
  algorithm->setPropertyValue("LHSWorkspace", lhsWorkspace);
  algorithm->setPropertyValue("RHSWorkspace", rhsWorkspace);
  algorithm->setPropertyValue("OutputWorkspace", outputWorkspace);
  algorithm->execute();
}

std::optional<std::pair<std::string, std::string>>
spinAsymmetryUpDownWorkspaces(std::vector<std::string> const &workspaces) {
  if (workspaces.size() == 2) {
    return std::make_pair(workspaces.front(), workspaces.back());
  }
  if (workspaces.size() == 4) {
    return std::make_pair(workspaces.front(), workspaces.back());
  }
  return std::nullopt;
}

std::string createSpinAsymmetryWorkspace(std::vector<std::string> const &workspaces, size_t index) {
  auto const upDownWorkspaces = spinAsymmetryUpDownWorkspaces(workspaces);
  if (!upDownWorkspaces) {
    return "";
  }

  auto const outputWorkspace = std::string{spinAsymmetryWorkspacePrefix} + std::to_string(index);
  auto const numeratorWorkspace = outputWorkspace + "_numerator";
  auto const denominatorWorkspace = outputWorkspace + "_denominator";

  executeBinaryAlgorithm("Minus", upDownWorkspaces->first, upDownWorkspaces->second, numeratorWorkspace);
  executeBinaryAlgorithm("Plus", upDownWorkspaces->first, upDownWorkspaces->second, denominatorWorkspace);
  executeBinaryAlgorithm("Divide", numeratorWorkspace, denominatorWorkspace, outputWorkspace);

  auto &ads = Mantid::API::AnalysisDataService::Instance();
  ads.remove(numeratorWorkspace);
  ads.remove(denominatorWorkspace);
  return outputWorkspace;
}

std::string groupingKey(PlottingWorkspaceSelection const &workspace) {
  if (!workspace.workspaceGroupName.empty()) {
    return std::string{"workspace-group:"} + workspace.workspaceGroupName;
  }

  auto key = std::string{"run:"} + workspace.groupName;
  for (auto const &runNumber : workspace.runNumbers) {
    key += ":" + runNumber;
  }
  return key;
}

std::vector<std::vector<std::string>>
workspaceGroups(std::vector<PlottingWorkspaceSelection> const &selectedWorkspaces) {
  auto keys = std::vector<std::string>{};
  auto groupedWorkspaces = std::vector<std::vector<std::string>>{};
  for (auto const &workspace : selectedWorkspaces) {
    auto const key = groupingKey(workspace);
    auto const keyIter = std::find(keys.cbegin(), keys.cend(), key);
    if (keyIter == keys.cend()) {
      keys.emplace_back(key);
      groupedWorkspaces.push_back({workspace.workspaceName});
    } else {
      groupedWorkspaces[std::distance(keys.cbegin(), keyIter)].emplace_back(workspace.workspaceName);
    }
  }
  return groupedWorkspaces;
}

std::vector<std::string> selectedWorkspaceNames(std::vector<PlottingWorkspaceSelection> const &selectedWorkspaces) {
  auto workspaceNames = std::vector<std::string>{};
  workspaceNames.reserve(selectedWorkspaces.size());
  for (auto const &workspace : selectedWorkspaces) {
    workspaceNames.emplace_back(workspace.workspaceName);
  }
  return workspaceNames;
}

std::vector<std::string> createSpinAsymmetryWorkspaces(std::vector<PlottingWorkspaceSelection> const &workspaces) {
  auto outputWorkspaces = std::vector<std::string>{};
  for (auto const &workspaceGroup : workspaceGroups(workspaces)) {
    auto outputWorkspace = createSpinAsymmetryWorkspace(workspaceGroup, outputWorkspaces.size());
    if (!outputWorkspace.empty()) {
      outputWorkspaces.emplace_back(std::move(outputWorkspace));
    }
  }
  return outputWorkspaces;
}
} // namespace

std::vector<std::string> PlottingModel::workspacesForPlotting(std::vector<PlottingWorkspaceSelection> const &workspaces,
                                                              PlotOutputOptions const &options) const {
  if (options.outputType == PlotOutputType::SpinAsymmetry) {
    return createSpinAsymmetryWorkspaces(workspaces);
  }
  return selectedWorkspaceNames(workspaces);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
