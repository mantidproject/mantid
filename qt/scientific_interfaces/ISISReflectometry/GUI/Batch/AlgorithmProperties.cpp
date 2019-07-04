// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Workspace_sptr;

namespace AlgorithmProperties {
std::string boolToString(bool value) { return value ? "1" : "0"; }

void update(std::string const &property, std::string const &value,
            AlgorithmRuntimeProps &properties) {
  if (!value.empty())
    properties[property] = value;
}

void update(std::string const &property,
            boost::optional<std::string> const &value,
            AlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

void update(std::string const &property, bool value,
            AlgorithmRuntimeProps &properties) {
  update(property, boolToString(value), properties);
}

void update(std::string const &property, int value,
            AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, size_t value,
            AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, double value,
            AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, boost::optional<double> const &value,
            AlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

void updateFromMap(AlgorithmRuntimeProps &properties,
                   std::map<std::string, std::string> const &parameterMap) {
  for (auto kvp : parameterMap) {
    update(kvp.first, kvp.second, properties);
  }
}
std::string getOutputWorkspace(IAlgorithm_sptr algorithm,
                               std::string const &property) {
  auto const workspaceName = algorithm->getPropertyValue(property);
  // The workspaces are not in the ADS by default, so add them
  if (!workspaceName.empty()) {
    Workspace_sptr workspace = algorithm->getProperty(property);
    if (workspace)
      Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName,
                                                                workspace);
  }
  return workspaceName;
}
} // namespace AlgorithmProperties
} // namespace CustomInterfaces
} // namespace MantidQt
