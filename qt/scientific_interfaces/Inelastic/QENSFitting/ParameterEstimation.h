// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/cow_ptr.h"
#include "ParameterEstimation.h"

#include <optional>
#include <span>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

struct DataForParameterEstimation {
  std::vector<double> x;
  std::vector<double> y;
};

using DataForParameterEstimationCollection = std::vector<DataForParameterEstimation>;
/// The x and y ranges are taken as spans so that the size-checked histogram data types can be
/// selected from directly, without first copying them out into a std::vector.
using EstimationDataSelector = std::function<DataForParameterEstimation(
    std::span<double const> x, std::span<double const> y, const std::pair<double, double> range)>;

class MANTIDQT_INELASTIC_DLL FunctionParameterEstimation {

public:
  using ParameterEstimateSetter = std::function<void(Mantid::API::IFunction_sptr const &function,
                                                     const DataForParameterEstimation &estimationData)>;
  using ParameterEstimator =
      std::function<std::unordered_map<std::string, double>(Mantid::MantidVec const &, Mantid::MantidVec const &)>;

  FunctionParameterEstimation(std::unordered_map<std::string, ParameterEstimator> estimators);
  void addParameterEstimationFunction(std::string const &functionName, ParameterEstimateSetter function);
  void estimateFunctionParameters(Mantid::API::IFunction_sptr const &function,
                                  const DataForParameterEstimationCollection &estimationData);

private:
  void estimateFunctionParameters(Mantid::API::IFunction_sptr const &function,
                                  const DataForParameterEstimation &estimationData,
                                  std::optional<Mantid::API::CompositeFunction_sptr> parentComposite = std::nullopt,
                                  std::optional<std::size_t> functionIndex = std::nullopt);

  std::map<std::string, ParameterEstimateSetter> m_funcMap;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
