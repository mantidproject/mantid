// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/IFunction.h"
#include "ParameterEstimation.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class MANTIDQT_INDIRECT_DLL IDAFunctionParameterEstimation {
  using EstimationFunction =
      std::function<void(::Mantid::API::IFunction_sptr &function, const DataForParameterEstimation &estimationData)>;

public:
  void addParameterEstimationFunction(std::string functionName, EstimationFunction function);
  void estimateFunctionParameters(::Mantid::API::IFunction_sptr &function,
                                  const DataForParameterEstimation &estimationData);

private:
  std::map<std::string, EstimationFunction> m_funcMap;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
