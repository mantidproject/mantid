// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"

namespace Mantid::API::AlgorithmProperties {

using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Workspace_sptr;

std::string boolToString(bool value) { return value ? "1" : "0"; }

void update(std::string const &property, std::string const &value, IAlgorithmRuntimeProps &properties) {
  if (!value.empty())
    properties.setPropertyValue(property, value);
}

// TODO code that uses this should move to std::optional
[[deprecated("use version with std::optional instead")]] void
update(std::string const &property, boost::optional<std::string> const &value, IAlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

void update(std::string const &property, std::optional<std::string> const &value, IAlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.value(), properties);
}

void update(std::string const &property, bool value, IAlgorithmRuntimeProps &properties) {
  properties.setProperty(property, value);
}

void update(std::string const &property, int value, IAlgorithmRuntimeProps &properties) {
  properties.setProperty(property, value);
}

void update(std::string const &property, size_t value, IAlgorithmRuntimeProps &properties) {
  properties.setProperty(property, value);
}

void update(std::string const &property, double value, IAlgorithmRuntimeProps &properties) {
  properties.setProperty(property, value);
}

// TODO code that uses this should move to std::optional
[[deprecated("use version with std::optional instead")]] void
update(std::string const &property, boost::optional<double> const &value, IAlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

void update(std::string const &property, std::optional<double> const &value, IAlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.value(), properties);
}

void update(std::string const &property, Workspace_sptr const &workspace, IAlgorithmRuntimeProps &properties) {
  properties.setProperty(property, workspace);
}

void update(std::string const &property, MatrixWorkspace_sptr const &workspace, IAlgorithmRuntimeProps &properties) {
  properties.setProperty(property, workspace);
}

void update(std::string const &property, IFunction_sptr const &function, IAlgorithmRuntimeProps &properties) {
  properties.setProperty(property, function);
}

void updateFromMap(IAlgorithmRuntimeProps &properties, std::map<std::string, std::string> const &parameterMap) {
  for (auto kvp : parameterMap) {
    update(kvp.first, kvp.second, properties);
  }
}
std::string getOutputWorkspace(const IAlgorithm_sptr &algorithm, std::string const &property) {
  auto const workspaceName = algorithm->getPropertyValue(property);
  return workspaceName;
}

} // namespace Mantid::API::AlgorithmProperties
