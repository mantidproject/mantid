// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidKernel/Logger.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iterator>
#include <stdexcept>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
Mantid::Kernel::Logger g_log("FitScriptGeneratorModel");

IFunction_sptr createIFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
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

std::size_t getPrefixIndexAt(std::string const &functionPrefix,
                             std::size_t const &index) {
  auto const subStrings = splitStringBy(functionPrefix, "f.");
  return std::stoull(subStrings[index]);
}

std::string removeTopFunctionIndex(std::string const &functionPrefix) {
  auto resultPrefix = functionPrefix;
  auto const firstDotIndex = resultPrefix.find(".");
  if (firstDotIndex != std::string::npos)
    resultPrefix.erase(0, firstDotIndex + 1);
  return resultPrefix;
}

std::string getTieRHS(std::string const &tie) {
  auto const tieSplit = splitStringBy(tie, "=");
  return tieSplit.size() > 1 ? tieSplit[1] : tieSplit[0];
}

std::string getTieExpression(std::string const &tie,
                             FittingMode const &fittingMode) {
  if (!tie.empty() && fittingMode == FittingMode::Simultaneous)
    return removeTopFunctionIndex(getTieRHS(tie));
  return tie;
}

std::string getParameterToTie(std::string const &parameter,
                              FittingMode const &fittingMode) {
  if (!parameter.empty() && fittingMode == FittingMode::Simultaneous)
    return removeTopFunctionIndex(parameter);
  return parameter;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

FitScriptGeneratorModel::FitScriptGeneratorModel()
    : m_fitDomains(), m_fittingMode(FittingMode::Sequential) {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {}

void FitScriptGeneratorModel::removeWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) {
  auto const removeIter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (removeIter != m_fitDomains.cend()) {
    auto const removeIndex = std::distance(m_fitDomains.cbegin(), removeIter);

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
    return fitDomain.workspaceName() == workspaceName &&
           fitDomain.workspaceIndex() == workspaceIndex;
  };

  return std::find_if(m_fitDomains.cbegin(), m_fitDomains.cend(), isMatch);
}

bool FitScriptGeneratorModel::hasWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  return findWorkspaceDomain(workspaceName, workspaceIndex) !=
         m_fitDomains.end();
}

bool FitScriptGeneratorModel::updateStartX(std::string const &workspaceName,
                                           WorkspaceIndex workspaceIndex,
                                           double startX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex].setStartX(startX);
}

bool FitScriptGeneratorModel::updateEndX(std::string const &workspaceName,
                                         WorkspaceIndex workspaceIndex,
                                         double endX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex].setEndX(endX);
}

void FitScriptGeneratorModel::removeFunction(std::string const &workspaceName,
                                             WorkspaceIndex workspaceIndex,
                                             std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].removeFunction(function);
}

void FitScriptGeneratorModel::addFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].addFunction(createIFunction(function));
}

void FitScriptGeneratorModel::setFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].setFunction(createIFunction(function));
}

IFunction_sptr
FitScriptGeneratorModel::getFunction(std::string const &workspaceName,
                                     WorkspaceIndex workspaceIndex) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex].getFunction();
}

void FitScriptGeneratorModel::updateParameterValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &parameter, double newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].setParameterValue(removeTopFunctionIndex(parameter),
                                              newValue);
}

void FitScriptGeneratorModel::updateAttributeValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &attribute, IFunction::Attribute const &newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].setAttributeValue(removeTopFunctionIndex(attribute),
                                              newValue);
}

void FitScriptGeneratorModel::updateParameterTie(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &parameter, std::string const &tie) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  auto const parameterToTie = getParameterToTie(parameter, m_fittingMode);
  auto const tieExpression = getTieExpression(tie, m_fittingMode);

  auto const tieSuccess = m_fitDomains[domainIndex].updateParameterTie(
      parameterToTie, tieExpression);

  if (!tieSuccess)
    g_log.warning("Failed to tie '" + parameter + "' to '" + tie + "'.");
}

void FitScriptGeneratorModel::setFittingMode(FittingMode const &fittingMode) {
  if (fittingMode == FittingMode::SimultaneousAndSequential)
    throw std::invalid_argument(
        "The fitting mode must be Sequential or Simultaneous.");
  m_fittingMode = fittingMode;
}

} // namespace MantidWidgets
} // namespace MantidQt
