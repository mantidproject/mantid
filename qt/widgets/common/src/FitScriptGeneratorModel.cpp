// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
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

bool isSinglePrefix(std::string const &prefix) {
  auto const subStrings = splitStringBy(prefix, "f.");
  return subStrings.size() == 1;
}

std::size_t getPrefixIndexAt(std::string const &prefix,
                             std::size_t const &index) {
  auto const subStrings = splitStringBy(prefix, "f.");
  return std::stoull(subStrings[index]);
}

std::string getTopPrefix(std::string const &prefix) {
  auto topPrefix = prefix;
  auto const firstDotIndex = topPrefix.find(".");
  if (firstDotIndex != std::string::npos)
    topPrefix.erase(firstDotIndex, topPrefix.size() - firstDotIndex);
  return topPrefix;
}

IFunction_sptr getFunctionAtPrefix(std::string const &prefix,
                                   IFunction_sptr const &function,
                                   bool isLastFunction = false) {
  if (isLastFunction)
    return function;

  auto const isLast = isSinglePrefix(prefix);
  auto const topPrefix = getTopPrefix(prefix);
  auto const firstIndex = getPrefixIndexAt(prefix, 0);

  try {
    return getFunctionAtPrefix(topPrefix, function->getFunction(firstIndex),
                               isLast);
  } catch (std::exception const &) {
    return IFunction_sptr();
  }
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

bool FitDomain::isSameDomain(std::string const &workspaceName,
                             WorkspaceIndex workspaceIndex) const {
  return m_workspaceName == workspaceName && m_workspaceIndex == workspaceIndex;
}

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

  addWorkspaceDomain(nextAvailablePrefix(), workspaceName, workspaceIndex,
                     startX, endX);
}

void FitScriptGeneratorModel::addWorkspaceDomain(
    std::string const &prefix, std::string const &workspaceName,
    WorkspaceIndex workspaceIndex, double startX, double endX) {
  addEmptyCompositeAtPrefix(prefix);
  m_fitDomains.emplace_back(
      FitDomain(prefix, workspaceName, workspaceIndex, startX, endX));
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
    return fitDomain.isSameDomain(workspaceName, workspaceIndex);
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

void FitScriptGeneratorModel::removeCompositeAtPrefix(
    std::string const &prefix) {
  removeCompositeAtIndex(getPrefixIndexAt(prefix, 0));
}

void FitScriptGeneratorModel::addEmptyCompositeAtPrefix(
    std::string const &prefix) {
  if (!m_function) {
    m_function = createMultiDomainFunction(1);
  } else {
    addEmptyCompositeAtPrefix(getTopPrefix(prefix),
                              getPrefixIndexAt(prefix, 0));
  }
}

void FitScriptGeneratorModel::addEmptyCompositeAtPrefix(
    std::string const &compositePrefix, std::size_t const &compositeIndex) {
  if (compositeIndex != numberOfDomains())
    throw std::runtime_error("The composite index provided is invalid.");

  if (hasCompositeAtPrefix(compositePrefix))
    throw std::runtime_error("The composite prefix provided already exists.");

  m_function->addFunction(createEmptyComposite());
  m_function->setDomainIndex(compositeIndex, compositeIndex);
}

void FitScriptGeneratorModel::removeCompositeAtIndex(
    std::size_t const &compositeIndex) {
  if (compositeIndex > numberOfDomains())
    throw std::runtime_error("The composite prefix provided does not exist");

  m_function->removeFunction(compositeIndex);
}

bool FitScriptGeneratorModel::hasCompositeAtPrefix(
    std::string const &compositePrefix) const {
  return static_cast<bool>(
      toComposite(getFunctionAtPrefix(compositePrefix, m_function)));
}

std::string FitScriptGeneratorModel::nextAvailablePrefix() const {
  return "f" + std::to_string(numberOfDomains());
}

} // namespace MantidWidgets
} // namespace MantidQt
