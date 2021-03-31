// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
namespace AlgorithmProperties {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Workspace_sptr;

std::string boolToString(bool value) { return value ? "1" : "0"; }

void update(std::string const &property, std::string const &value, AlgorithmRuntimeProps &properties) {
  if (!value.empty())
    properties[property] = value;
}

void update(std::string const &property, boost::optional<std::string> const &value, AlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

void update(std::string const &property, bool value, AlgorithmRuntimeProps &properties) {
  update(property, boolToString(value), properties);
}

void update(std::string const &property, int value, AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, size_t value, AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, double value, AlgorithmRuntimeProps &properties) {
  update(property, std::to_string(value), properties);
}

void update(std::string const &property, boost::optional<double> const &value, AlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
}

void updateFromMap(AlgorithmRuntimeProps &properties, std::map<std::string, std::string> const &parameterMap) {
  for (auto kvp : parameterMap) {
    update(kvp.first, kvp.second, properties);
  }
}
std::string getOutputWorkspace(const IAlgorithm_sptr &algorithm, std::string const &property) {
  auto const workspaceName = algorithm->getPropertyValue(property);
  return workspaceName;
}
} // namespace AlgorithmProperties
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
