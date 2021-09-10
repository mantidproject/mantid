// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IDAFunctionParameterEstimation.h"
#include "MantidAPI/IFunction.h"

#include <math.h>
namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

// Add function name and estimation function to the stored function map.
void IDAFunctionParameterEstimation::addParameterEstimationFunction(std::string name, EstimationFunction function) {
  m_funcMap.insert(std::make_pair(std::move(name), std::move(function)));
}
// Estimate the function parameters for the input function
// If the input function exists in the stored map it will update the function
// parameters in-place.
void IDAFunctionParameterEstimation::estimateFunctionParameters(::Mantid::API::IFunction_sptr &function,
                                                                const DataForParameterEstimation &estimationData) {
  if (function) {
    std::string functionName = function->name();
    if (m_funcMap.find(functionName) != m_funcMap.end()) {
      m_funcMap[functionName](function, estimationData);
    }
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
