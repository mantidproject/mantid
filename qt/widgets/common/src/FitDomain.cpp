// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitDomain.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <stdexcept>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("FitDomain");

IFunction_sptr createIFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
}

CompositeFunction_sptr toComposite(IFunction_sptr function) {
  return std::dynamic_pointer_cast<CompositeFunction>(function);
}

CompositeFunction_sptr createEmptyComposite() {
  return toComposite(createIFunction("name=CompositeFunction"));
}

std::vector<std::string> splitStringBy(std::string const &str,
                                       std::string const &delimiter) {
  std::vector<std::string> subStrings;
  boost::split(subStrings, str, boost::is_any_of(delimiter));
  subStrings.erase(std::remove_if(subStrings.begin(), subStrings.end(),
                                  [](std::string const &subString) {
                                    return subString.empty();
                                  }),
                   subStrings.cend());
  return subStrings;
}

std::vector<std::string>
getFunctionNamesInString(std::string const &functionString) {
  std::vector<std::string> functionNames;
  for (auto const &str : splitStringBy(functionString, ",();"))
    if (str.substr(0, 5) == "name=")
      functionNames.emplace_back(str.substr(5));
  return functionNames;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

FitDomain::FitDomain(std::string const &workspaceName,
                     WorkspaceIndex workspaceIndex, double startX, double endX)
    : m_workspaceName(workspaceName), m_workspaceIndex(workspaceIndex),
      m_startX(startX), m_endX(endX), m_function(nullptr) {}

bool FitDomain::setStartX(double startX) {
  auto const validStartX = isValidStartX(startX);
  if (validStartX)
    m_startX = startX;
  return validStartX;
}

bool FitDomain::setEndX(double endX) {
  auto const validEndX = isValidStartX(endX);
  if (validEndX)
    m_endX = endX;
  return validEndX;
}

void FitDomain::setFunction(Mantid::API::IFunction_sptr const &function) {
  m_function = function;
}

Mantid::API::IFunction_sptr FitDomain::getFunction() const {
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

void FitDomain::removeFunctionFromIFunction(std::string const &function,
                                            IFunction_sptr &iFunction) {
  for (auto const &functionName : getFunctionNamesInString(function)) {
    if (iFunction->name() == functionName) {
      iFunction = nullptr;
      break;
    }
  }
}

void FitDomain::removeFunctionFromComposite(std::string const &function,
                                            CompositeFunction_sptr &composite) {
  for (auto const &functionName : getFunctionNamesInString(function))
    if (composite->hasFunction(functionName))
      composite->removeFunction(composite->functionIndex(functionName));

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
  if (auto composite = toComposite(m_function)) {
    composite->addFunction(function);
  } else {
    auto newComposite = createEmptyComposite();
    newComposite->addFunction(m_function->clone());
    newComposite->addFunction(function);
    m_function = newComposite;
  }
}

void FitDomain::setParameterValue(std::string const &parameter,
                                  double newValue) {
  if (hasParameter(parameter))
    m_function->setParameter(parameter, newValue);
}

void FitDomain::setAttributeValue(std::string const &attribute,
                                  IFunction::Attribute newValue) {
  if (m_function && m_function->hasAttribute(attribute))
    m_function->setAttribute(attribute, newValue);
}

bool FitDomain::hasParameter(std::string const &parameter) const {
  if (m_function)
    return m_function->hasParameter(parameter);
  return false;
}

double FitDomain::getParameterValue(std::string const &parameter) const {
  if (hasParameter(parameter))
    return m_function->getParameter(parameter);
  throw std::runtime_error("The function does not contain this parameter.");
}

bool FitDomain::updateParameterTie(std::string const &parameter,
                                   std::string const &tie) {
  if (hasParameter(parameter)) {
    if (tie.empty())
      m_function->removeTie(m_function->parameterIndex(parameter));
    else
      return setParameterTie(parameter, tie);
  }
  // We want to silently ignore if the function doesn't have the parameter
  return true;
}

bool FitDomain::setParameterTie(std::string const &parameter,
                                std::string const &tie) {
  try {
    m_function->tie(parameter, tie);
    return true;
  } catch (std::invalid_argument const &ex) {
    g_log.warning(ex.what());
    return false;
  } catch (std::runtime_error const &ex) {
    g_log.warning(ex.what());
    return false;
  }
}

void FitDomain::clearParameterTie(std::string const &parameter) {
  if (hasParameter(parameter))
    m_function->removeTie(m_function->parameterIndex(parameter));
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
    return xLimits(ads.retrieveWS<MatrixWorkspace>(m_workspaceName),
                   m_workspaceIndex);

  throw std::invalid_argument("The domain '" + m_workspaceName + " (" +
                              std::to_string(m_workspaceIndex.value) +
                              ")' could not be found.");
}

std::pair<double, double>
FitDomain::xLimits(MatrixWorkspace_const_sptr const &workspace,
                   WorkspaceIndex workspaceIndex) const {
  if (workspace) {
    auto const xData = workspace->x(workspaceIndex.value);
    return std::pair<double, double>(xData.front(), xData.back());
  }

  throw std::invalid_argument("The workspace '" + m_workspaceName +
                              "' is not a matrix workspace.");
}

} // namespace MantidWidgets
} // namespace MantidQt
