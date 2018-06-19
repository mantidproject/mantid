#include "IqtFitModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"

using namespace Mantid::API;

namespace {
IFunction_sptr getFirstInCategory(IFunction_sptr function,
                                  const std::string &category);

IFunction_sptr getFirstInCategory(CompositeFunction_sptr composite,
                                  const std::string &category) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    auto function = getFirstInCategory(composite->getFunction(i), category);
    if (function)
      return function;
  }
  return nullptr;
}

IFunction_sptr getFirstInCategory(IFunction_sptr function,
                                  const std::string &category) {
  if (!function)
    return nullptr;
  if (function->category() == category)
    return function;
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return getFirstInCategory(composite, category);
  return nullptr;
}

boost::optional<std::size_t> getFirstParameter(IFunction_sptr function,
                                               const std::string &shortName) {
  for (auto i = 0u; i < function->nParams(); ++i) {
    if (boost::algorithm::ends_with(function->parameterName(i), shortName))
      return i;
  }
  return boost::none;
}

std::vector<std::string> getParameters(IFunction_sptr function,
                                       const std::string &shortParameterName) {
  std::vector<std::string> parameters;

  for (const auto &longName : function->getParameterNames()) {
    if (boost::algorithm::ends_with(longName, shortParameterName))
      parameters.push_back(longName);
  }
  return parameters;
}

bool containsNOrMore(IFunction_sptr function,
                     const std::vector<std::string> &values, std::size_t n) {
  const auto names = function->getParameterNames();
  if (names.empty())
    return values.empty() || n == 0;

  std::size_t count = 0;
  std::size_t index = 0;
  do {
    for (const auto &value : values)
      count += boost::algorithm::ends_with(names[index], value) ? 1 : 0;
  } while (count < n && ++index < names.size());
  return count >= n;
}

bool constrainIntensities(IFunction_sptr function) {
  const auto intensityParameters = getParameters(function, "Height");
  const auto backgroundParameters = getParameters(function, "A0");

  if (intensityParameters.size() + backgroundParameters.size() < 2 ||
      intensityParameters.empty())
    return false;

  std::string tieString("1");

  for (const auto &parameter : backgroundParameters)
    tieString += "-" + parameter;

  for (auto i = 1u; i < intensityParameters.size(); ++i)
    tieString += "-" + intensityParameters[i];

  function->tie(intensityParameters[0], tieString);
  return true;
}

bool unconstrainIntensities(IFunction_sptr function) {
  const auto index = getFirstParameter(function, "Height");
  if (index)
    return function->removeTie(*index);
  return false;
}

bool hasConstrainableIntensities(IFunction_sptr function) {
  return containsNOrMore(function, {"Height", "A0"}, 2);
}

double computeTauApproximation(MatrixWorkspace_sptr workspace) {
  const auto x = workspace->x(0);
  const auto y = workspace->y(0);

  if (x.size() > 4)
    return -x[4] / log(y[4]);
  return 0.0;
}

double computeHeightApproximation(IFunction_sptr function) {
  const auto background = getFirstInCategory(function, "Background");
  const double height = 1.0;
  if (background && background->hasParameter("A0"))
    return height - background->getParameter("A0");
  return height;
}

std::string getSuffix(MatrixWorkspace_sptr workspace) {
  const auto position = workspace->getName().rfind("_");
  return workspace->getName().substr(position + 1);
}

std::string getFitString(MatrixWorkspace_sptr workspace) {
  auto suffix = getSuffix(workspace);
  boost::algorithm::to_lower(suffix);
  if (suffix == "iqt")
    return "Fit";
  return "_IqtFit";
}
}; // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IqtFitModel::IqtFitModel()
    : IndirectFittingModel(), m_constrainIntensities(false) {}

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

std::string IqtFitModel::sequentialFitOutputName() const {
  if (isMultiFit())
    return "MultiIqtFit_" + m_fitType;
  auto fitString = getFitString(getWorkspace(0));
  return createOutputName("%1%" + fitString + "_" + m_fitType + "_s%2%", "_to_",
                          0);
}

std::string IqtFitModel::simultaneousFitOutputName() const {
  if (isMultiFit())
    return "MultiSimultaneousIqtFit_" + m_fitType;
  auto fitString = getFitString(getWorkspace(0));
  return createOutputName("%1%" + fitString + "_mult" + m_fitType + "_s%2%",
                          "_to_", 0);
}

std::string IqtFitModel::singleFitOutputName(std::size_t index,
                                             std::size_t spectrum) const {
  auto fitString = getFitString(getWorkspace(0));
  return createSingleFitOutputName(
      "%1%" + fitString + "_" + m_fitType + "_s%2%", index, spectrum);
}

void IqtFitModel::setFitTypeString(const std::string &fitType) {
  m_fitType = fitType;
}

void IqtFitModel::setFitFunction(Mantid::API::IFunction_sptr function) {
  IndirectFittingModel::setFitFunction(function);
  if (m_constrainIntensities)
    constrainIntensities(function);
}

bool IqtFitModel::canConstrainIntensities() const {
  return hasConstrainableIntensities(getFittingFunction());
}

bool IqtFitModel::setConstrainIntensities(bool constrain) {
  if (constrain != m_constrainIntensities) {
    m_constrainIntensities = constrain;
    if (constrain)
      return constrainIntensities(getFittingFunction());
    return unconstrainIntensities(getFittingFunction());
  }
  return true;
}

std::unordered_map<std::string, ParameterValue>
IqtFitModel::createDefaultParameters(std::size_t index) const {
  std::unordered_map<std::string, ParameterValue> parameters;
  parameters["Height"] = computeHeightApproximation(getFittingFunction());

  const auto inputWs = getWorkspace(index);
  const auto tau = inputWs ? computeTauApproximation(inputWs) : 0.0;

  parameters["Lifetime"] = ParameterValue(tau);
  parameters["Stretching"] = ParameterValue(1.0);
  parameters["A0"] = ParameterValue(0.0);
  return parameters;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
