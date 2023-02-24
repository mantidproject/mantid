// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"

<<<<<<< HEAD
=======
namespace {

template <typename VALUE_TYPE>
void updateVectorImpl(std::string const &property, std::vector<VALUE_TYPE> const &values,
                      Mantid::API::IAlgorithmRuntimeProps &properties) {
  if (values.size() < 1)
    return;

  auto value = Mantid::Kernel::Strings::simpleJoin(values.cbegin(), values.cend(), ", ");
  if (!value.empty())
    properties.setPropertyValue(property, value);
}

} // namespace

>>>>>>> 87cbcf85a16 (Move AlgorithmProperties to Mantid API)
namespace Mantid::API::AlgorithmProperties {

using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Workspace_sptr;

std::string boolToString(bool value) { return value ? "1" : "0"; }

void update(std::string const &property, std::string const &value, IAlgorithmRuntimeProps &properties) {
  if (!value.empty())
    properties.setPropertyValue(property, value);
}

void update(std::string const &property, boost::optional<std::string> const &value,
            IAlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
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

void update(std::string const &property, boost::optional<double> const &value, IAlgorithmRuntimeProps &properties) {
  if (value)
    update(property, value.get(), properties);
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
