// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitDomain.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
Mantid::Kernel::Logger g_log("FitDomain");

IFunction_sptr createIFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
}

CompositeFunction_sptr toComposite(const IFunction_sptr &function) {
  return std::dynamic_pointer_cast<CompositeFunction>(function);
}

CompositeFunction_sptr createEmptyComposite() { return toComposite(createIFunction("name=CompositeFunction")); }

std::vector<std::string> getFunctionNamesInString(std::string const &functionString) {
  std::vector<std::string> functionNames;
  for (auto const &str : splitStringBy(functionString, ",();"))
    if (str.substr(0, 5) == "name=")
      functionNames.emplace_back(str.substr(5));
  return functionNames;
}

bool isValueWithinConstraint(std::string const &constraint, double value) {
  auto const limits = splitConstraintString(constraint).second;
  return std::atof(limits.first.c_str()) <= value && value <= std::atof(limits.second.c_str());
}

} // namespace

namespace MantidQt::MantidWidgets {

FitDomain::FitDomain(std::string workspaceName, WorkspaceIndex workspaceIndex, double startX, double endX)
    : m_workspaceName(std::move(workspaceName)), m_workspaceIndex(workspaceIndex), m_startX(startX), m_endX(endX),
      m_function(nullptr) {}

void FitDomain::setWorkspaceName(std::string const &workspaceName) { m_workspaceName = workspaceName; }

std::string FitDomain::domainName() const {
  return m_workspaceName + " (" + std::to_string(m_workspaceIndex.value) + ")";
}

bool FitDomain::setStartX(double startX) {
  auto const validStartX = isValidStartX(startX);
  if (validStartX)
    m_startX = startX;
  return validStartX;
}

bool FitDomain::setEndX(double endX) {
  auto const validEndX = isValidEndX(endX);
  if (validEndX)
    m_endX = endX;
  return validEndX;
}

void FitDomain::setFunction(Mantid::API::IFunction_sptr const &function) { m_function = function; }

Mantid::API::IFunction_sptr FitDomain::getFunctionCopy() const {
  if (m_function)
    return m_function->clone();
  return nullptr;
}

void FitDomain::removeFunction(std::string const &function) {
  if (m_function) {
    if (auto composite = toComposite(m_function))
      removeFunctionFromComposite(function, composite);
    else
      removeFunctionFromIFunction(function, m_function);
  }
}

void FitDomain::removeFunctionFromIFunction(std::string const &function, IFunction_sptr &iFunction) {
  auto const functionNames = getFunctionNamesInString(function);
  const std::string iFunctionName = iFunction->name();
  const auto it = std::find_if(functionNames.cbegin(), functionNames.cend(),
                               [&iFunctionName](const auto &name) { return iFunctionName == name; });
  if (it != functionNames.cend())
    iFunction = nullptr;
}

void FitDomain::removeFunctionFromComposite(std::string const &function, CompositeFunction_sptr &composite) {
  for (auto const &functionName : getFunctionNamesInString(function)) {
    if (composite->hasFunction(functionName))
      composite->removeFunction(composite->functionIndex(functionName));
  }

  if (composite->nFunctions() == 0)
    m_function = nullptr;
  else if (composite->nFunctions() == 1)
    m_function = composite->getFunction(0);
}

void FitDomain::addFunction(IFunction_sptr const &function) {
  if (m_function) {
    addFunctionToExisting(function);
  } else {
    m_function = function;
  }
}

void FitDomain::addFunctionToExisting(IFunction_sptr const &function) {
  if (toComposite(function)) {
    g_log.error("Add function failed: Nested composite functions are not supported.");
    return;
  }

  if (auto composite = toComposite(m_function)) {
    composite->addFunction(function);
  } else {
    auto newComposite = createEmptyComposite();
    newComposite->addFunction(m_function->clone());
    newComposite->addFunction(function);
    m_function = newComposite;
  }
}

void FitDomain::setParameterValue(std::string const &parameter, double newValue) {
  if (hasParameter(parameter) && isParameterValueWithinConstraints(parameter, newValue)) {
    m_function->setParameter(parameter, newValue);
    removeInvalidatedTies();
  }
}

void FitDomain::removeInvalidatedTies() {
  for (auto paramIndex = 0u; paramIndex < m_function->nParams(); ++paramIndex) {
    if (auto const tie = m_function->getTie(paramIndex)) {
      auto const parameterName = m_function->parameterName(paramIndex);
      if (!isParameterValueWithinConstraints(parameterName, tie->eval(false)))
        clearParameterTie(parameterName);
    }
  }
}

double FitDomain::getParameterValue(std::string const &parameter) const {
  if (hasParameter(parameter))
    return m_function->getParameter(parameter);
  throw std::runtime_error("The function does not contain the parameter " + parameter + ".");
}

double FitDomain::getTieValue(std::string const &tie) const {
  if (tie.empty())
    throw std::runtime_error("This tie does not have a value.");
  else if (isNumber(tie))
    return std::stod(tie);
  return getParameterValue(tie);
}

void FitDomain::setAttributeValue(std::string const &attribute, const IFunction::Attribute &newValue) {
  if (m_function && m_function->hasAttribute(attribute))
    m_function->setAttribute(attribute, newValue);
}

Mantid::API::IFunction::Attribute FitDomain::getAttributeValue(std::string const &attribute) const {
  if (m_function && m_function->hasAttribute(attribute))
    return m_function->getAttribute(attribute);
  throw std::runtime_error("The function does not contain this attribute.");
}

bool FitDomain::hasParameter(std::string const &parameter) const {
  if (m_function)
    return m_function->hasParameter(parameter);
  return false;
}

bool FitDomain::isParameterActive(std::string const &parameter) const {
  if (hasParameter(parameter))
    return m_function->getParameterStatus(m_function->parameterIndex(parameter)) == IFunction::ParameterStatus::Active;
  return false;
}

void FitDomain::setParameterFixed(std::string const &parameter, bool fix) const {
  if (!hasParameter(parameter))
    throw std::runtime_error("The parameter does not exist in this domain.");

  if (fix) {
    m_function->tie(parameter, std::to_string(getParameterValue(parameter)));
  } else {
    m_function->removeTie(m_function->parameterIndex(parameter));
  }
}

bool FitDomain::isParameterFixed(std::string const &parameter) const {
  if (hasParameter(parameter)) {
    return m_function->getParameterStatus(m_function->parameterIndex(parameter)) == IFunction::ParameterStatus::Fixed;
  }
  throw std::runtime_error("The parameter does not exist in this domain.");
}

std::string FitDomain::getParameterTie(std::string const &parameter) const {
  if (hasParameter(parameter)) {
    auto const parameterIndex = m_function->parameterIndex(parameter);
    if (auto const tie = m_function->getTie(parameterIndex)) {
      auto const tieStr = tie->asString();
      auto const equalsIndex = tieStr.find("=");
      if (equalsIndex != std::string::npos)
        return tieStr.substr(equalsIndex + 1);
    }
    return "";
  }
  throw std::runtime_error("The parameter does not exist in this domain.");
}

std::string FitDomain::getParameterConstraint(std::string const &parameter) const {
  if (hasParameter(parameter)) {
    auto const parameterIndex = m_function->parameterIndex(parameter);
    auto const constraint = m_function->getConstraint(parameterIndex);
    return constraint ? constraint->asString() : "";
  }
  throw std::runtime_error("The parameter does not exist in this domain.");
}

void FitDomain::clearParameterTie(std::string const &parameter) {
  if (hasParameter(parameter))
    m_function->removeTie(m_function->parameterIndex(parameter));
}

bool FitDomain::updateParameterTie(std::string const &parameter, std::string const &tie) {
  if (hasParameter(parameter)) {
    if (tie.empty())
      m_function->removeTie(m_function->parameterIndex(parameter));
    else
      return setParameterTie(parameter, tie);
  }
  // We want to silently ignore if the function doesn't have the parameter
  return true;
}

bool FitDomain::setParameterTie(std::string const &parameter, std::string const &tie) {
  try {
    if (isValidParameterTie(parameter, tie)) {
      m_function->tie(parameter, tie);
      setParameterValue(parameter, getTieValue(tie));
    }
  } catch (std::invalid_argument const &ex) {
    g_log.warning(ex.what());
    return false;
  } catch (std::runtime_error const &ex) {
    g_log.warning(ex.what());
    return false;
  }
  return true;
}

void FitDomain::removeParameterConstraint(std::string const &parameter) {
  if (hasParameter(parameter))
    m_function->removeConstraint(parameter);
}

void FitDomain::updateParameterConstraint(std::string const &functionIndex, std::string const &parameter,
                                          std::string const &constraint) {
  if (isValidParameterConstraint(functionIndex + parameter, constraint)) {
    if (functionIndex.empty())
      m_function->addConstraints(constraint);
    else if (auto composite = toComposite(m_function))
      updateParameterConstraint(composite, functionIndex, parameter, constraint);
  }
}

void FitDomain::updateParameterConstraint(CompositeFunction_sptr &composite, std::string const &functionIndex,
                                          std::string const &parameter, std::string const &constraint) {
  auto const index = getFunctionIndexAt(functionIndex, 0);
  if (index < composite->nFunctions()) {
    auto function = composite->getFunction(index);
    if (function->hasParameter(parameter))
      function->addConstraints(constraint);
  }
}

std::vector<std::string> FitDomain::getParametersTiedTo(std::string const &parameter) const {
  std::vector<std::string> tiedParameters;
  if (m_function) {
    for (auto paramIndex = 0u; paramIndex < m_function->nParams(); ++paramIndex)
      appendParametersTiedTo(tiedParameters, parameter, paramIndex);
  }
  return tiedParameters;
}

void FitDomain::appendParametersTiedTo(std::vector<std::string> &tiedParameters, std::string const &parameter,
                                       std::size_t const &parameterIndex) const {
  if (auto const tie = m_function->getTie(parameterIndex)) {
    for (auto rhsParameter : tie->getRHSParameters()) {
      if (parameter == rhsParameter.parameterName())
        tiedParameters.emplace_back(m_function->parameterName(parameterIndex));
    }
  }
}

bool FitDomain::isParameterValueWithinConstraints(std::string const &parameter, double value) const {
  if (!hasParameter(parameter))
    return false;

  auto isValid = true;

  auto const parameterIndex = m_function->parameterIndex(parameter);
  if (auto const constraint = m_function->getConstraint(parameterIndex))
    isValid = isValueWithinConstraint(constraint->asString(), value);

  if (!isValid)
    g_log.warning("The provided value for '" + parameter + "' is not within its constraints.");
  return isValid;
}

bool FitDomain::isValidParameterTie(std::string const &parameter, std::string const &tie) const {
  if (tie.empty())
    return true;
  else if (isNumber(tie))
    return isParameterValueWithinConstraints(parameter, std::stod(tie));
  return isParameterValueWithinConstraints(parameter, getParameterValue(tie));
}

bool FitDomain::isValidParameterConstraint(std::string const &parameter, std::string const &constraint) const {
  auto isValid = false;
  if (hasParameter(parameter)) {
    auto const parameterValue = m_function->getParameter(parameter);
    isValid = isValueWithinConstraint(constraint, parameterValue);
    if (!isValid)
      g_log.warning("The provided constraint for '" + parameter + "' does not encompass its current value.");
  }
  return isValid;
}

bool FitDomain::isValidStartX(double startX) const {
  auto const limits = xLimits();
  return limits.first <= startX && startX <= limits.second && startX < m_endX;
}

bool FitDomain::isValidEndX(double endX) const {
  auto const limits = xLimits();
  return limits.first <= endX && endX <= limits.second && endX > m_startX;
}

std::pair<double, double> FitDomain::xLimits() const {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(m_workspaceName))
    return xLimits(ads.retrieveWS<MatrixWorkspace>(m_workspaceName), m_workspaceIndex);

  throw std::invalid_argument("The domain '" + m_workspaceName + " (" + std::to_string(m_workspaceIndex.value) +
                              ")' could not be found.");
}

std::pair<double, double> FitDomain::xLimits(MatrixWorkspace_const_sptr const &workspace,
                                             WorkspaceIndex workspaceIndex) const {
  if (workspace) {
    auto const xData = workspace->x(workspaceIndex.value);
    return std::pair<double, double>(xData.front(), xData.back());
  }

  throw std::invalid_argument("The workspace '" + m_workspaceName + "' is not a matrix workspace.");
}

} // namespace MantidQt::MantidWidgets
