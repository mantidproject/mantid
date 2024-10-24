// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/QENSFitUtilities.h"
#include <unordered_map>
#include <utility>

namespace Mantid::API {

void renameWorkspacesWith(const WorkspaceGroup_sptr &groupWorkspace,
                          std::function<std::string(std::size_t)> const &getName,
                          std::function<void(Workspace_sptr, const std::string &)> const &renamer) {
  std::unordered_map<std::string, std::size_t> nameCount;
  for (auto i = 0u; i < groupWorkspace->size(); ++i) {
    const auto name = getName(i);
    auto count = nameCount.find(name);

    if (count == nameCount.end()) {
      renamer(groupWorkspace->getItem(i), name);
      nameCount[name] = 1;
    } else
      renamer(groupWorkspace->getItem(i), name + "(" + std::to_string(++count->second) + ")");
  }
}

void renameWorkspace(const IAlgorithm_sptr &renamer, const Workspace_sptr &workspace, const std::string &newName) {
  renamer->setProperty("InputWorkspace", workspace);
  renamer->setProperty("OutputWorkspace", newName);
  renamer->executeAsChildAlg();
}

bool containsMultipleData(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  const auto &first = workspaces.front();
  return std::any_of(workspaces.cbegin(), workspaces.cend(),
                     [&first](const auto &workspace) { return workspace != first; });
}

void renameWorkspacesInQENSFit(Algorithm *qensFit, IAlgorithm_sptr renameAlgorithm,
                               const WorkspaceGroup_sptr &outputGroup, std::string const &outputBaseName,
                               std::string const &, std::function<std::string(std::size_t)> const &getNameSuffix) {
  Progress renamerProg(qensFit, 0.98, 1.0, outputGroup->size() + 1);
  renamerProg.report("Renaming group workspaces...");

  auto getName = [&](std::size_t i) {
    std::string name = outputBaseName + "_" + getNameSuffix(i);
    return name;
  };

  auto renamer = [&](const Workspace_sptr &workspace, const std::string &name) {
    renameWorkspace(renameAlgorithm, workspace, name);
    renamerProg.report("Renamed workspace in group.");
  };
  renameWorkspacesWith(outputGroup, getName, renamer);
}

} // namespace Mantid::API
