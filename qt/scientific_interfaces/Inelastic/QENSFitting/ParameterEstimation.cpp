// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ParameterEstimation.h"
#include "MantidAPI/IFunction.h"

#include <math.h>

namespace {

std::string nameForParameterEstimator(Mantid::API::IFunction_sptr const &function,
                                      std::optional<Mantid::API::CompositeFunction_sptr> composite,
                                      std::optional<std::size_t> functionIndex) {
  auto functionName = function->name();
  if (composite && functionIndex) {
    // functionIndex returns the index of the first function with the given name. If the index is different,
    // we know that this is not the first function with this name in the composite function
    if ((*composite)->functionIndex(functionName) != *functionIndex) {
      functionName += "N";
    }
  }
  return functionName;
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionParameterEstimation::ParameterEstimateSetter
parameterEstimateSetter(FunctionParameterEstimation::ParameterEstimator estimator) {
  return [estimator](Mantid::API::IFunction_sptr const &function, const DataForParameterEstimation &estimationData) {
    auto const y = estimationData.y;
    auto const x = estimationData.x;
    if (x.size() != 2 || y.size() != 2) {
      return;
    }
    auto const parameterValues = estimator(x, y);

    for (auto it = parameterValues.cbegin(); it != parameterValues.cend(); ++it) {
      if (!std::isinf(it->second))
        function->setParameter(it->first, it->second);
    }
  };
}

FunctionParameterEstimation::FunctionParameterEstimation(std::unordered_map<std::string, ParameterEstimator> estimators)
    : m_funcMap() {
  for (auto it = estimators.cbegin(); it != estimators.cend(); ++it) {
    addParameterEstimationFunction(it->first, parameterEstimateSetter(it->second));
  }
}

// Add function name and estimation function to the stored function map.
void FunctionParameterEstimation::addParameterEstimationFunction(std::string const &functionName,
                                                                 ParameterEstimateSetter function) {
  m_funcMap.insert(std::make_pair(functionName, std::move(function)));
}

void FunctionParameterEstimation::estimateFunctionParameters(
    Mantid::API::IFunction_sptr const &function, const DataForParameterEstimationCollection &estimationData) {
  if (!function) {
    return;
  }

  // Estimate function parameters - parameters are updated in-place.
  if (auto composite = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(function)) {
    if (estimationData.size() != composite->nFunctions()) {
      return;
    }

    for (auto i = 0u; i < composite->nFunctions(); ++i) {
      auto childFunction = composite->getFunction(i);
      estimateFunctionParameters(childFunction, estimationData[i]);
    }
  }
}

void FunctionParameterEstimation::estimateFunctionParameters(
    Mantid::API::IFunction_sptr const &function, const DataForParameterEstimation &estimationData,
    std::optional<Mantid::API::CompositeFunction_sptr> parentComposite, std::optional<std::size_t> functionIndex) {
  if (!function) {
    return;
  }

  if (auto composite = std::dynamic_pointer_cast<Mantid::API::CompositeFunction>(function)) {
    for (auto i = 0u; i < composite->nFunctions(); ++i) {
      auto childFunction = composite->getFunction(i);
      estimateFunctionParameters(childFunction, estimationData, composite, i);
    }
  } else {
    auto const parameterEstimatorName = nameForParameterEstimator(function, parentComposite, functionIndex);
    if (m_funcMap.find(parameterEstimatorName) != m_funcMap.end()) {
      m_funcMap[parameterEstimatorName](function, estimationData);
    }
  }
}

} // namespace MantidQt::CustomInterfaces::Inelastic
