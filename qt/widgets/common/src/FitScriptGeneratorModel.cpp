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

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>

using namespace Mantid::API;

namespace {

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

std::string getFunctionNameFromString(std::string const &functionString) {
  return splitStringBy(splitStringBy(functionString, ",")[0], "=")[1];
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

/**
 * FitDomain struct methods.
 */

FitDomain::FitDomain(std::string const &prefix,
                     std::string const &workspaceName,
                     WorkspaceIndex workspaceIndex, double startX, double endX)
    : m_multiDomainFunctionPrefix(prefix), m_workspaceName(workspaceName),
      m_workspaceIndex(workspaceIndex), m_startX(startX), m_endX(endX) {}

/**
 * FitScriptGeneratorModel class methods.
 */

FitScriptGeneratorModel::FitScriptGeneratorModel()
    : m_fitDomains(), m_function(nullptr) {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {}

void FitScriptGeneratorModel::removeWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) {
  auto const removeIter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (removeIter != m_fitDomains.cend()) {
    auto const removePrefix = removeIter->m_multiDomainFunctionPrefix;
    auto const removeIndex = getPrefixIndexAt(removePrefix, 0);

    removeWorkspaceDomain(removeIndex, removeIter);
    removeCompositeAtPrefix(removePrefix);
  }
}

void FitScriptGeneratorModel::removeWorkspaceDomain(
    std::size_t const &removeIndex,
    std::vector<FitDomain>::const_iterator const &removeIter) {
  if (removeIter != m_fitDomains.cend())
    m_fitDomains.erase(removeIter);

  auto const decrementPrefixes = [&](FitDomain &fitDomain) {
    auto thisIndex = getPrefixIndexAt(fitDomain.m_multiDomainFunctionPrefix, 0);
    if (removeIndex < thisIndex) {
      --thisIndex;
      fitDomain.m_multiDomainFunctionPrefix = "f" + std::to_string(thisIndex);
    }
    return fitDomain;
  };

  std::transform(m_fitDomains.begin(), m_fitDomains.end(), m_fitDomains.begin(),
                 decrementPrefixes);
}

void FitScriptGeneratorModel::addWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    double startX, double endX) {
  if (hasWorkspaceDomain(workspaceName, workspaceIndex))
    throw std::invalid_argument("The '" + workspaceName + " (" +
                                std::to_string(workspaceIndex.value) +
                                ")' domain already exists.");

  addWorkspaceDomain(nextAvailableCompositePrefix(), workspaceName,
                     workspaceIndex, startX, endX);
}

void FitScriptGeneratorModel::addWorkspaceDomain(
    std::string const &functionPrefix, std::string const &workspaceName,
    WorkspaceIndex workspaceIndex, double startX, double endX) {
  addEmptyCompositeAtPrefix(functionPrefix);
  m_fitDomains.emplace_back(
      FitDomain(functionPrefix, workspaceName, workspaceIndex, startX, endX));
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

  auto composite = toComposite(m_function->getFunction(domainIndex));
  auto const functionName = getFunctionNameFromString(function);
  if (composite && composite->hasFunction(functionName))
    composite->removeFunction(composite->functionIndex(functionName));
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
  clearCompositeAtIndex(findDomainIndex(workspaceName, workspaceIndex));

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

void FitScriptGeneratorModel::removeCompositeAtPrefix(
    std::string const &functionPrefix) {
  removeCompositeAtIndex(getPrefixIndexAt(functionPrefix, 0));
}

void FitScriptGeneratorModel::addEmptyCompositeAtPrefix(
    std::string const &functionPrefix) {
  if (!m_function) {
    m_function = createMultiDomainFunction(1);
  } else {
    addEmptyCompositeAtPrefix(getTopFunctionPrefix(functionPrefix),
                              getPrefixIndexAt(functionPrefix, 0));
  }
}

void FitScriptGeneratorModel::addEmptyCompositeAtPrefix(
    std::string const &functionPrefix, std::size_t const &functionIndex) {
  if (functionIndex != numberOfDomains())
    throw std::runtime_error("The composite index provided is invalid.");

  if (hasCompositeAtPrefix(functionPrefix))
    throw std::runtime_error("The composite prefix provided already exists.");

  m_function->addFunction(createEmptyComposite());
  m_function->setDomainIndex(functionIndex, functionIndex);
}

void FitScriptGeneratorModel::removeCompositeAtIndex(
    std::size_t const &functionIndex) {
  if (functionIndex > numberOfDomains())
    throw std::runtime_error("The composite index provided does not exist");

  clearCompositeAtIndex(functionIndex);
  m_function->removeFunction(functionIndex);
}

void FitScriptGeneratorModel::clearCompositeAtIndex(
    std::size_t const &functionIndex) {
  if (functionIndex > numberOfDomains())
    throw std::runtime_error("The composite index provided does not exist");

  auto composite = toComposite(m_function->getFunction(functionIndex));
  for (auto i = composite->nFunctions() - 1u; i < composite->nFunctions(); --i)
    composite->removeFunction(i);
}

bool FitScriptGeneratorModel::hasCompositeAtPrefix(
    std::string const &functionPrefix) const {
  return static_cast<bool>(
      toComposite(getFunctionAtPrefix(functionPrefix, m_function)));
}

std::string FitScriptGeneratorModel::nextAvailableCompositePrefix() const {
  return "f" + std::to_string(numberOfDomains());
}

} // namespace MantidWidgets
} // namespace MantidQt
