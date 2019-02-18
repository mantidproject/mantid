// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchJobAlgorithm.h"
#include "MantidAPI/IAlgorithm.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Workspace_sptr;
using AlgorithmRuntimeProps = std::map<std::string, std::string>;

BatchJobAlgorithm::BatchJobAlgorithm(
    Mantid::API::IAlgorithm_sptr algorithm,
    MantidQt::API::ConfiguredAlgorithm::AlgorithmRuntimeProps properties,
    // cppcheck-suppress passedByValue
    std::vector<std::string> outputWorkspaceProperties, Item *item)
    : ConfiguredAlgorithm(algorithm, properties), m_item(item),
      m_outputWorkspaceProperties(outputWorkspaceProperties) {}

Item *BatchJobAlgorithm::item() { return m_item; }

std::vector<std::string> BatchJobAlgorithm::outputWorkspaceNames() const {
  auto workspaceNames = std::vector<std::string>();
  for (auto &property : m_outputWorkspaceProperties) {
    workspaceNames.emplace_back(m_algorithm->getPropertyValue(property));
  }
  return workspaceNames;
}

std::map<std::string, Workspace_sptr>
BatchJobAlgorithm::outputWorkspaceNameToWorkspace() const {
  auto propertyToName = std::map<std::string, Workspace_sptr>();
  for (auto &property : m_outputWorkspaceProperties) {
    auto workspaceName = m_algorithm->getPropertyValue(property);
    if (!workspaceName.empty())
      propertyToName[workspaceName] = m_algorithm->getProperty(property);
  }
  return propertyToName;
}
} // namespace CustomInterfaces
} // namespace MantidQt
