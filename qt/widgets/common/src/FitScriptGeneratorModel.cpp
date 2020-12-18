// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("FitScriptGenerator");

IFunction_sptr createIFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
}

CompositeFunction_sptr toComposite(IFunction_sptr function) {
  return std::dynamic_pointer_cast<CompositeFunction>(function);
}

CompositeFunction_sptr createEmptyComposite() {
  return toComposite(createIFunction("name=CompositeFunction"));
}

MultiDomainFunction_sptr
createMultiDomainFunction(std::size_t const &numberOfDomains) {
  return FunctionFactory::Instance().createInitializedMultiDomainFunction(
      "name=CompositeFunction", numberOfDomains);
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

bool isSingleFunctionPrefix(std::string const &functionPrefix) {
  auto const subStrings = splitStringBy(functionPrefix, "f.");
  return subStrings.size() == 1;
}

std::size_t getPrefixIndexAt(std::string const &functionPrefix,
                             std::size_t const &index) {
  auto const subStrings = splitStringBy(functionPrefix, "f.");
  return std::stoull(subStrings[index]);
}

std::string getTopFunctionPrefix(std::string const &functionPrefix) {
  auto topPrefix = functionPrefix;
  auto const firstDotIndex = topPrefix.find(".");
  if (firstDotIndex != std::string::npos)
    topPrefix.erase(firstDotIndex, topPrefix.size() - firstDotIndex);
  return topPrefix;
}

IFunction_sptr getFunctionAtPrefix(std::string const &functionPrefix,
                                   IFunction_sptr const &function,
                                   bool isLastFunction = false) {
  if (isLastFunction)
    return function;

  auto const isLast = isSingleFunctionPrefix(functionPrefix);
  auto const topPrefix = getTopFunctionPrefix(functionPrefix);
  auto const firstIndex = getPrefixIndexAt(functionPrefix, 0);

  try {
    return getFunctionAtPrefix(topPrefix, function->getFunction(firstIndex),
                               isLast);
  } catch (std::exception const &) {
    return IFunction_sptr();
  }
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

/**
 * FitDomain struct methods.
 */

FitDomain::FitDomain(std::string const &workspaceName,
                     WorkspaceIndex workspaceIndex, double startX, double endX)
    : m_workspaceName(workspaceName), m_workspaceIndex(workspaceIndex),
      m_startX(startX), m_endX(endX) {}

/**
 * FitScriptGeneratorModel class methods.
 */

FitScriptGeneratorModel::FitScriptGeneratorModel()
    : m_fitDomains(), m_function(createMultiDomainFunction(0)) {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {}

void FitScriptGeneratorModel::removeWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) {
  auto const removeIter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (removeIter != m_fitDomains.cend()) {
    auto const removeIndex = std::distance(m_fitDomains.cbegin(), removeIter);

    removeCompositeAtDomainIndex(removeIndex);
    if (removeIter != m_fitDomains.cend())
      m_fitDomains.erase(removeIter);
  }
}

void FitScriptGeneratorModel::addWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    double startX, double endX) {
  if (hasWorkspaceDomain(workspaceName, workspaceIndex))
    throw std::invalid_argument("The '" + workspaceName + " (" +
                                std::to_string(workspaceIndex.value) +
                                ")' domain already exists.");

  addEmptyCompositeAtDomainIndex(numberOfDomains());
  m_fitDomains.emplace_back(
      FitDomain(workspaceName, workspaceIndex, startX, endX));
}

std::size_t
FitScriptGeneratorModel::findDomainIndex(std::string const &workspaceName,
                                         WorkspaceIndex workspaceIndex) const {
  auto const iter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (iter != m_fitDomains.cend())
    return std::distance(m_fitDomains.cbegin(), iter);

  throw std::invalid_argument("The domain '" + workspaceName + " (" +
                              std::to_string(workspaceIndex.value) +
                              ")' could not be found.");
}

std::vector<FitDomain>::const_iterator
FitScriptGeneratorModel::findWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  auto const isMatch = [&](FitDomain const &fitDomain) {
    return fitDomain.m_workspaceName == workspaceName &&
           fitDomain.m_workspaceIndex == workspaceIndex;
  };

  return std::find_if(m_fitDomains.cbegin(), m_fitDomains.cend(), isMatch);
}

bool FitScriptGeneratorModel::hasWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  return findWorkspaceDomain(workspaceName, workspaceIndex) !=
         m_fitDomains.end();
}

bool FitScriptGeneratorModel::isStartXValid(std::string const &workspaceName,
                                            WorkspaceIndex workspaceIndex,
                                            double startX) const {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  auto const limits = xLimits(workspaceName, workspaceIndex);
  return limits.first <= startX && startX <= limits.second &&
         startX < m_fitDomains[domainIndex].m_endX;
}

bool FitScriptGeneratorModel::isEndXValid(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          double endX) const {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  auto const limits = xLimits(workspaceName, workspaceIndex);
  return limits.first <= endX && endX <= limits.second &&
         endX > m_fitDomains[domainIndex].m_startX;
}

std::pair<double, double>
FitScriptGeneratorModel::xLimits(std::string const &workspaceName,
                                 WorkspaceIndex workspaceIndex) const {
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(workspaceName))
    return xLimits(ads.retrieveWS<MatrixWorkspace>(workspaceName),
                   workspaceIndex);

  throw std::invalid_argument("The domain '" + workspaceName + " (" +
                              std::to_string(workspaceIndex.value) +
                              ")' could not be found.");
}

std::pair<double, double>
FitScriptGeneratorModel::xLimits(MatrixWorkspace_const_sptr const &workspace,
                                 WorkspaceIndex workspaceIndex) const {
  if (workspace) {
    auto const xData = workspace->x(workspaceIndex.value);
    return std::pair<double, double>(xData.front(), xData.back());
  }

  throw std::invalid_argument("The workspace '" + workspace->getName() +
                              "' is not a matrix workspace.");
}

void FitScriptGeneratorModel::updateStartX(std::string const &workspaceName,
                                           WorkspaceIndex workspaceIndex,
                                           double startX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].m_startX = startX;
}

void FitScriptGeneratorModel::updateEndX(std::string const &workspaceName,
                                         WorkspaceIndex workspaceIndex,
                                         double endX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].m_endX = endX;
}

void FitScriptGeneratorModel::removeFunction(std::string const &workspaceName,
                                             WorkspaceIndex workspaceIndex,
                                             std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  if (auto composite = toComposite(m_function->getFunction(domainIndex))) {
    for (auto const functionName : getFunctionNamesInString(function))
      if (composite->hasFunction(functionName))
        composite->removeFunction(composite->functionIndex(functionName));
  }
}

void FitScriptGeneratorModel::addFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  addFunction(workspaceName, workspaceIndex, createIFunction(function));
}

void FitScriptGeneratorModel::addFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          IFunction_sptr const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  if (auto composite = toComposite(m_function->getFunction(domainIndex)))
    composite->addFunction(function);
}

void FitScriptGeneratorModel::setFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  clearCompositeAtDomainIndex(findDomainIndex(workspaceName, workspaceIndex));

  if (auto const iFunction = createIFunction(function)) {
    if (auto const composite = toComposite(iFunction)) {
      for (auto i = 0u; i < composite->nFunctions(); ++i)
        addFunction(workspaceName, workspaceIndex, composite->getFunction(i));
    } else {
      addFunction(workspaceName, workspaceIndex, iFunction);
    }
  }
}

CompositeFunction_sptr
FitScriptGeneratorModel::getFunction(std::string const &workspaceName,
                                     WorkspaceIndex workspaceIndex) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return toComposite(m_function->getFunction(domainIndex));
}

void FitScriptGeneratorModel::updateParameterValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &parameter, double newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  if (auto composite = toComposite(m_function->getFunction(domainIndex))) {
    auto fullParameterName = parameter;
    if (composite->nFunctions() == 1)
      fullParameterName = "f0." + fullParameterName;

    if (composite->hasParameter(fullParameterName))
      composite->setParameter(fullParameterName, newValue);
  }
}

void FitScriptGeneratorModel::updateAttributeValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &attribute, IFunction::Attribute const &newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  if (auto composite = toComposite(m_function->getFunction(domainIndex))) {
    auto fullAttributeName = attribute;
    if (composite->nFunctions() == 1)
      fullAttributeName = "f0." + fullAttributeName;

    if (composite->hasAttribute(fullAttributeName))
      composite->setAttribute(fullAttributeName, newValue);
  }
}

void FitScriptGeneratorModel::updateParameterTie(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &parameter, std::string const &tie) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  if (auto composite = toComposite(m_function->getFunction(domainIndex))) {

    if (composite->nFunctions() == 1) {
      auto function = composite->getFunction(0);
      if (function->hasParameter(parameter))
        updateParameterTie(function, parameter, tie);
    } else {
      if (composite->hasParameter(parameter))
        updateParameterTie(composite, parameter, tie);
    }
  }
}

void FitScriptGeneratorModel::updateParameterTie(IFunction_sptr const &function,
                                                 std::string const &parameter,
                                                 std::string const &tie) {
  if (tie.empty()) {
    function->removeTie(function->parameterIndex(parameter));
  } else {
    auto const tieSplit = splitStringBy(tie, "=");
    auto const tieValue = tieSplit.size() > 1 ? tieSplit[1] : tieSplit[0];

    try {
      function->tie(parameter, tieValue);
    } catch (std::invalid_argument const &ex) {
      g_log.error(ex.what());
    } catch (std::runtime_error const &ex) {
      g_log.error(ex.what());
    }
  }
}

void FitScriptGeneratorModel::removeCompositeAtDomainIndex(
    std::size_t const &domainIndex) {
  if (domainIndex >= numberOfDomains())
    throw std::runtime_error("The domain index provided does not exist");

  clearCompositeAtDomainIndex(domainIndex);
  m_function->removeFunction(domainIndex);
}

void FitScriptGeneratorModel::clearCompositeAtDomainIndex(
    std::size_t const &domainIndex) {
  if (domainIndex >= numberOfDomains())
    throw std::runtime_error("The domain index provided does not exist");

  auto composite = toComposite(m_function->getFunction(domainIndex));
  for (auto i = composite->nFunctions() - 1u; i < composite->nFunctions(); --i)
    composite->removeFunction(i);
}

void FitScriptGeneratorModel::addEmptyCompositeAtDomainIndex(
    std::size_t const &domainIndex) {
  if (domainIndex != numberOfDomains())
    throw std::runtime_error("The domain index provided is invalid.");

  m_function->addFunction(createEmptyComposite());
  m_function->setDomainIndex(domainIndex, domainIndex);
}

std::string FitScriptGeneratorModel::nextAvailableDomainPrefix() const {
  return "f" + std::to_string(numberOfDomains());
}

} // namespace MantidWidgets
} // namespace MantidQt
