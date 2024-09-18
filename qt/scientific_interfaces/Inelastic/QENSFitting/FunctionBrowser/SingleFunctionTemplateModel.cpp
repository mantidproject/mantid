// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "SingleFunctionTemplateModel.h"
#include "../ParameterEstimation.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"

#include <unordered_map>

namespace MantidQt::CustomInterfaces::Inelastic {

namespace {
// Sort the function list, keep existing order just rotate the None entry into
// the front
void sortFunctionList(std::vector<std::string> &list) {
  auto ix = std::find(list.begin(), list.end(), "None");
  if (ix == list.end() || ix == list.begin()) {
    return;
  } else {
    for (auto itr = ix; itr > list.begin(); --itr) {
      std::rotate(itr - 1, itr, itr + 1);
    }
  }
}
} // namespace

using namespace MantidWidgets;
using namespace Mantid::API;

SingleFunctionTemplateModel::SingleFunctionTemplateModel() = default;

SingleFunctionTemplateModel::SingleFunctionTemplateModel(
    std::unique_ptr<FunctionParameterEstimation> parameterEstimation)
    : m_parameterEstimation(std::move(parameterEstimation)) {}

void SingleFunctionTemplateModel::updateAvailableFunctions(
    const std::map<std::string, std::string> &functionInitialisationStrings) {
  m_fitTypeToFunctionStore.clear();
  m_globalParameterStore.clear();
  m_fitTypeList.clear();
  for (auto const &functionInfo : functionInitialisationStrings) {
    IFunction_sptr function;
    try {
      function = FunctionFactory::Instance().createInitialized(functionInfo.second);
    } catch (std::invalid_argument &) {
      // Error in function string, adding an empty function to map
    }
    m_fitTypeToFunctionStore.insert(functionInfo.first, function);
    m_globalParameterStore.insert(functionInfo.first, std::vector<std::string>());
  }
  // Sort the FunctionList as None should always appear first
  m_fitTypeList = qListToStdVector(m_fitTypeToFunctionStore.keys());
  sortFunctionList(m_fitTypeList);
  m_fitType = m_fitTypeList.front();
}

std::vector<std::string> SingleFunctionTemplateModel::getFunctionList() { return m_fitTypeList; }

int SingleFunctionTemplateModel::getEnumIndex() {
  auto const findIter = std::find(m_fitTypeList.cbegin(), m_fitTypeList.cend(), m_fitType);
  return findIter != m_fitTypeList.cend() ? static_cast<int>(std::distance(m_fitTypeList.cbegin(), findIter)) : -1;
}

void SingleFunctionTemplateModel::setFunction(IFunction_sptr fun) {
  if (!fun)
    return;
  if (fun->nFunctions() == 0) {
    const auto name = fun->name();
    auto fitType = findFitTypeForFunctionName(name);
    if (fitType) {
      setFitType(fitType.value());
    } else {
      throw std::runtime_error("Cannot set function " + name);
    }
  } else {
    throw std::runtime_error("Function has wrong structure.");
  }
}

void SingleFunctionTemplateModel::setFitType(const std::string &type) {
  if (m_function) {
    m_globalParameterStore[m_fitType] = getGlobalParameters();
  }
  m_fitType = type;
  if (type == "None") {
    FunctionModel::setFunction(IFunction_sptr());
    return;
  }
  setGlobalParameters(m_globalParameterStore[type]);
  FunctionModel::setFunction(m_fitTypeToFunctionStore[type]->clone());
  estimateFunctionParameters();
}

std::string SingleFunctionTemplateModel::getFitType() { return m_fitType; }

EstimationDataSelector SingleFunctionTemplateModel::getEstimationDataSelector() const {
  return [](const std::vector<double> &x, const std::vector<double> &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    // Find data thats within range
    double xmin = range.first;
    double xmax = range.second;
    if (xmax - xmin < 1e-5) {
      return DataForParameterEstimation{};
    }

    const auto startItr =
        std::find_if(x.cbegin(), x.cend(), [xmin](const double &val) -> bool { return val >= (xmin - 1e-5); });
    const auto endItr = std::find_if(x.cbegin(), x.cend(), [xmax](const double &val) -> bool { return val > xmax; });

    if (std::distance(startItr, endItr - 1) < 2)
      return DataForParameterEstimation{};

    size_t first = std::distance(x.cbegin(), startItr);
    size_t end = std::distance(x.cbegin(), endItr);
    size_t m = first + (end - first) / 2;

    return DataForParameterEstimation{{x[first], x[m]}, {y[first], y[m]}};
  };
}

void SingleFunctionTemplateModel::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_estimationData = std::move(data);
}

void SingleFunctionTemplateModel::estimateFunctionParameters() {
  m_parameterEstimation->estimateFunctionParameters(getFullFunction(), m_estimationData);
}

void SingleFunctionTemplateModel::setGlobal(std::string const &parameterName, bool isGlobal) {
  auto globalParameters = getGlobalParameters();
  auto const findIter = std::find(globalParameters.cbegin(), globalParameters.cend(), parameterName);
  if (isGlobal && findIter == globalParameters.cend()) {
    globalParameters.emplace_back(parameterName);
  } else if (!isGlobal && findIter != globalParameters.cend()) {
    globalParameters.erase(findIter);
  }
  setGlobalParameters(globalParameters);
}

std::optional<std::string> SingleFunctionTemplateModel::findFitTypeForFunctionName(const std::string &name) const {
  auto result = std::find_if(m_fitTypeToFunctionStore.constBegin(), m_fitTypeToFunctionStore.constEnd(),
                             [&name](const auto &function) -> bool {
                               if (function)
                                 return function->name() == name;
                               else
                                 return false;
                             });
  if (result != std::end(m_fitTypeToFunctionStore)) {
    return std::optional<std::string>{result.key()};
  }
  return std::nullopt;
}

} // namespace MantidQt::CustomInterfaces::Inelastic
