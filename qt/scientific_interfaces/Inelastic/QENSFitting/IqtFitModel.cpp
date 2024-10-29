// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IqtFitModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"

#include <utility>

using namespace Mantid::API;

namespace {
IFunction_sptr getFirstInCategory(IFunction_sptr function, const std::string &category);

IFunction_sptr getFirstInCategory(const CompositeFunction_sptr &composite, const std::string &category) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    auto function = getFirstInCategory(composite->getFunction(i), category);
    if (function)
      return function;
  }
  return nullptr;
}

IFunction_sptr getFirstInCategory(IFunction_sptr function, const std::string &category) {
  if (!function)
    return nullptr;
  if (function->category() == category)
    return function;
  auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return getFirstInCategory(composite, category);
  return nullptr;
}

std::vector<std::string> getParameters(const IFunction_sptr &function, const std::string &shortParameterName) {
  auto const parameterNames = function->getParameterNames();

  std::vector<std::string> parameters;
  std::copy_if(parameterNames.cbegin(), parameterNames.cend(), std::back_inserter(parameters),
               [&shortParameterName](std::string const &longName) { return longName.ends_with(shortParameterName); });
  return parameters;
}

bool constrainIntensities(const IFunction_sptr &function) {
  const auto intensityParameters = getParameters(function, "Height");
  const auto backgroundParameters = getParameters(function, "A0");

  if (intensityParameters.size() + backgroundParameters.size() < 2 || intensityParameters.empty())
    return false;

  std::string tieString =
      std::accumulate(backgroundParameters.cbegin(), backgroundParameters.cend(), std::string("1"),
                      [](const auto &head, const auto &parameter) { return head + "-" + parameter; });

  for (auto i = 1u; i < intensityParameters.size(); ++i)
    tieString += "-" + intensityParameters[i];

  function->tie(intensityParameters[0], tieString);
  return true;
}

double computeTauApproximation(const MatrixWorkspace_sptr &workspace) {
  const auto &x = workspace->x(0);
  const auto &y = workspace->y(0);

  if (x.size() > 4)
    return -x[4] / log(y[4]);
  return 0.0;
}

double computeHeightApproximation(IFunction_sptr function) {
  const auto background = getFirstInCategory(std::move(function), "Background");
  const double height = 1.0;
  if (background && background->hasParameter("A0"))
    return height - background->getParameter("A0");
  return height;
}
} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

IqtFitModel::IqtFitModel() : FittingModel(), m_constrainIntensities(false) { m_fitType = IQT_STRING; }

IAlgorithm_sptr IqtFitModel::sequentialFitAlgorithm() const {
  auto algorithm = AlgorithmManager::Instance().create("IqtFitSequential");
  algorithm->setProperty("IgnoreInvalidData", true);
  return algorithm;
}

IAlgorithm_sptr IqtFitModel::simultaneousFitAlgorithm() const {
  auto algorithm = AlgorithmManager::Instance().create("IqtFitSimultaneous");
  algorithm->setProperty("IgnoreInvalidData", true);
  return algorithm;
}

void IqtFitModel::setFitFunction(Mantid::API::MultiDomainFunction_sptr function) {
  FittingModel::setFitFunction(function);
  if (m_constrainIntensities)
    constrainIntensities(function);
}

std::unordered_map<std::string, ParameterValue> IqtFitModel::createDefaultParameters(WorkspaceID workspaceID) const {
  std::unordered_map<std::string, ParameterValue> parameters;
  parameters["Height"] = ParameterValue(computeHeightApproximation(getFitFunction()));

  const auto inputWs = getWorkspace(workspaceID);
  const auto tau = inputWs ? computeTauApproximation(inputWs) : 0.0;

  parameters["Lifetime"] = ParameterValue(tau);
  parameters["Stretching"] = ParameterValue(1.0);
  parameters["A0"] = ParameterValue(0.0);
  return parameters;
}

} // namespace MantidQt::CustomInterfaces::Inelastic
