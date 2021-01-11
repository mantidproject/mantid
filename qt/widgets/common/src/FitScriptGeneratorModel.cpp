// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"

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

std::string removeTopFunctionIndex(std::string const &functionIndex) {
  auto resultPrefix = functionIndex;
  auto const firstDotIndex = resultPrefix.find(".");
  if (firstDotIndex != std::string::npos)
    resultPrefix.erase(0, firstDotIndex + 1);
  return resultPrefix;
}

std::string replaceTopFunctionIndexWith(std::string const &functionIndex,
                                        std::size_t const &newIndex) {
  return "f" + std::to_string(newIndex) + "." +
         removeTopFunctionIndex(functionIndex);
}

std::string getTieRHS(std::string const &tie) {
  auto const tieSplit = splitStringBy(tie, "=");
  return tieSplit.size() > 1 ? tieSplit[1] : tieSplit[0];
}

std::string getLocalTie(std::string const &tie,
                        FittingMode const &fittingMode) {
  if (!tie.empty() && fittingMode == FittingMode::Simultaneous)
    return removeTopFunctionIndex(getTieRHS(tie));
  return tie;
}

bool isFunctionIndex(std::string const &str) {
  auto const subStrings = splitStringBy(str, "f");
  if (subStrings.size() == 1)
    return isNumber(subStrings[0]);
  return false;
}

bool isSameDomain(std::size_t const &domainIndex,
                  std::string const &parameter) {
  if (!parameter.empty() && !isNumber(parameter))
    return domainIndex == getFunctionIndexAt(parameter, 0);
  return true;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

FitScriptGeneratorModel::FitScriptGeneratorModel()
    : m_presenter(), m_fitDomains(), m_globalParameters(), m_globalTies(),
      m_fittingMode(FittingMode::Sequential) {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {
  m_fitDomains.clear();
  m_globalTies.clear();
  m_globalParameters.clear();
}

void FitScriptGeneratorModel::subscribePresenter(
    IFitScriptGeneratorPresenter *presenter) {
  m_presenter = presenter;
}

void FitScriptGeneratorModel::removeWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) {
  auto const removeIter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (removeIter != m_fitDomains.cend()) {
    m_fitDomains.erase(removeIter);
    checkGlobalTies();
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
      std::make_unique<FitDomain>(workspaceName, workspaceIndex, startX, endX));
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

std::vector<std::unique_ptr<FitDomain>>::const_iterator
FitScriptGeneratorModel::findWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  auto const isMatch = [&](auto const &fitDomain) {
    return fitDomain->workspaceName() == workspaceName &&
           fitDomain->workspaceIndex() == workspaceIndex;
  };

  return std::find_if(m_fitDomains.cbegin(), m_fitDomains.cend(), isMatch);
}

bool FitScriptGeneratorModel::hasWorkspaceDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  return findWorkspaceDomain(workspaceName, workspaceIndex) !=
         m_fitDomains.cend();
}

bool FitScriptGeneratorModel::updateStartX(std::string const &workspaceName,
                                           WorkspaceIndex workspaceIndex,
                                           double startX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex]->setStartX(startX);
}

bool FitScriptGeneratorModel::updateEndX(std::string const &workspaceName,
                                         WorkspaceIndex workspaceIndex,
                                         double endX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex]->setEndX(endX);
}

void FitScriptGeneratorModel::removeFunction(std::string const &workspaceName,
                                             WorkspaceIndex workspaceIndex,
                                             std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex]->removeFunction(function);
  checkGlobalTies();
}

void FitScriptGeneratorModel::addFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex]->addFunction(createIFunction(function));
  checkGlobalTies();
}

void FitScriptGeneratorModel::setFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex]->setFunction(createIFunction(function));
  checkGlobalTies();
}

IFunction_sptr
FitScriptGeneratorModel::getFunction(std::string const &workspaceName,
                                     WorkspaceIndex workspaceIndex) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex]->getFunction();
}

std::string FitScriptGeneratorModel::getEquivalentFunctionIndexForDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &functionIndex) const {
  if (!functionIndex.empty() && m_fittingMode == FittingMode::Simultaneous) {
    auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
    return replaceTopFunctionIndexWith(functionIndex, domainIndex);
  }
  return functionIndex;
}

std::string FitScriptGeneratorModel::getEquivalentParameterTieForDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &fullParameter, std::string const &fullTie) const {
  if (fullTie.empty() || isNumber(fullTie) || !validTie(fullTie))
    return fullTie;

  if (m_fittingMode == FittingMode::Sequential)
    return fullTie;

  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return getEquivalentParameterTieForDomain(domainIndex, fullParameter,
                                            fullTie);
}

std::string FitScriptGeneratorModel::getEquivalentParameterTieForDomain(
    std::size_t const &domainIndex, std::string const &fullParameter,
    std::string const &fullTie) const {
  auto const parameterDomainIndex = getFunctionIndexAt(fullParameter, 0);
  auto const tieDomainIndex = getFunctionIndexAt(fullTie, 0);
  if (parameterDomainIndex == tieDomainIndex)
    return replaceTopFunctionIndexWith(fullTie, domainIndex);
  return fullTie;
}

std::string FitScriptGeneratorModel::getAdjustedFunctionIndex(
    std::string const &parameter) const {
  if (parameter.empty() || isNumber(parameter))
    return parameter;

  if (m_fittingMode == FittingMode::Sequential)
    return parameter;
  return removeTopFunctionIndex(parameter);
}

void FitScriptGeneratorModel::updateParameterValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &fullParameter, double newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  if (!hasGlobalTie(fullParameter)) {
    auto const parameter = getAdjustedFunctionIndex(fullParameter);
    m_fitDomains[domainIndex]->setParameterValue(parameter, newValue);
    updateParameterValuesWithGlobalTieTo(fullParameter);
  }
}

void FitScriptGeneratorModel::updateParameterValuesWithGlobalTieTo(
    std::string const &parameter) {
  for (auto const &globalTie : m_globalTies) {
    if (parameter == globalTie.m_tie) {
      auto const domainIndex = getFunctionIndexAt(globalTie.m_parameter, 0);
      m_fitDomains[domainIndex]->setParameterValue(
          getAdjustedFunctionIndex(globalTie.m_parameter),
          getParameterValue(parameter));
    }
  }
}

void FitScriptGeneratorModel::updateAttributeValue(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &fullAttribute, IFunction::Attribute const &newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex]->setAttributeValue(
      getAdjustedFunctionIndex(fullAttribute), newValue);
}

void FitScriptGeneratorModel::updateParameterTie(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &fullParameter, std::string const &tie) {
  if (validTie(tie)) {
    auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
    updateParameterTie(domainIndex, fullParameter, tie);
  } else {
    g_log.warning("Invalid tie '" + tie + "' provided.");
  }
}

void FitScriptGeneratorModel::updateParameterTie(
    std::size_t const &domainIndex, std::string const &fullParameter,
    std::string const &fullTie) {
  checkParameterIsNotGlobal(fullParameter);

  if (m_fittingMode == FittingMode::Sequential ||
      isSameDomain(domainIndex, fullTie))
    updateLocalParameterTie(domainIndex, fullParameter, fullTie);
  else
    updateGlobalParameterTie(domainIndex, fullParameter, fullTie);
}

void FitScriptGeneratorModel::updateLocalParameterTie(
    std::size_t const &domainIndex, std::string const &fullParameter,
    std::string const &fullTie) {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  auto const tie = getLocalTie(fullTie, m_fittingMode);

  if (parameter != tie && validParameter(fullParameter)) {
    if (m_fitDomains[domainIndex]->updateParameterTie(parameter, tie))
      clearGlobalTie(fullParameter);
    else
      g_log.warning("Invalid tie '" + fullTie + "' provided.");
  }
}

void FitScriptGeneratorModel::updateGlobalParameterTie(
    std::size_t const &domainIndex, std::string const &fullParameter,
    std::string const &fullTie) {
  if (validParameter(fullParameter)) {
    if (validGlobalTie(fullTie)) {
      clearGlobalTie(fullParameter);

      auto const parameter = getAdjustedFunctionIndex(fullParameter);
      m_fitDomains[domainIndex]->clearParameterTie(parameter);
      m_fitDomains[domainIndex]->setParameterValue(parameter,
                                                   getParameterValue(fullTie));

      m_globalTies.emplace_back(GlobalTie(fullParameter, fullTie));
    } else {
      g_log.warning("Invalid tie '" + fullTie + "' provided.");
    }
  }
}

void FitScriptGeneratorModel::removeParameterConstraint(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &fullParameter) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex]->removeParameterConstraint(
      getAdjustedFunctionIndex(fullParameter));
}

void FitScriptGeneratorModel::updateParameterConstraint(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &functionIndex, std::string const &constraint) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  auto const parameterName =
      splitConstraintString(constraint).first.toStdString();
  m_fitDomains[domainIndex]->updateParameterConstraint(
      getAdjustedFunctionIndex(functionIndex), parameterName, constraint);
}

double FitScriptGeneratorModel::getParameterValue(
    std::string const &fullParameter) const {
  auto const domainIndex = getFunctionIndexAt(fullParameter, 0);
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  if (domainIndex < numberOfDomains())
    return m_fitDomains[domainIndex]->getParameterValue(parameter);

  throw std::runtime_error("The domain index provided does not exist.");
}

bool FitScriptGeneratorModel::validParameter(
    std::string const &fullParameter) const {
  auto const domainIndex = getFunctionIndexAt(fullParameter, 0);
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  if (domainIndex < numberOfDomains())
    return m_fitDomains[domainIndex]->hasParameter(parameter);
  return false;
}

bool FitScriptGeneratorModel::validTie(std::string const &tie) const {
  if (tie.empty() || isNumber(tie))
    return true;

  auto const subStrings = splitStringBy(tie, ".");
  switch (m_fittingMode) {
  case FittingMode::Sequential:
    return 1 <= subStrings.size() && subStrings.size() <= 2;
  case FittingMode::Simultaneous:
    return 2 <= subStrings.size() && subStrings.size() <= 3 &&
           isFunctionIndex(subStrings[0]);
  default:
    throw std::invalid_argument(
        "Fitting mode must be Sequential or Simultaneous.");
  }
}

bool FitScriptGeneratorModel::validGlobalTie(std::string const &fullTie) const {
  if (!fullTie.empty() && !isNumber(fullTie))
    return validParameter(fullTie);
  return false;
}

void FitScriptGeneratorModel::clearGlobalTie(std::string const &fullParameter) {
  auto const removeIter = findGlobalTie(fullParameter);
  if (removeIter != m_globalTies.cend())
    m_globalTies.erase(removeIter);
}

void FitScriptGeneratorModel::checkGlobalTies() {
  auto const isTieInvalid = [&](GlobalTie const &globalTie) {
    return !validParameter(globalTie.m_parameter) ||
           !validGlobalTie(globalTie.m_tie);
  };

  auto const iter =
      std::remove_if(m_globalTies.begin(), m_globalTies.end(), isTieInvalid);

  if (iter != m_globalTies.cend()) {
    m_globalTies.erase(iter, m_globalTies.cend());
    m_presenter->setGlobalTies(m_globalTies);
  }
}

bool FitScriptGeneratorModel::hasGlobalTie(
    std::string const &fullParameter) const {
  return findGlobalTie(fullParameter) != m_globalTies.cend();
}

std::vector<GlobalTie>::const_iterator
FitScriptGeneratorModel::findGlobalTie(std::string const &fullParameter) const {
  return std::find_if(m_globalTies.cbegin(), m_globalTies.cend(),
                      [&fullParameter](GlobalTie const &globalTie) {
                        return globalTie.m_parameter == fullParameter;
                      });
}

void FitScriptGeneratorModel::setGlobalParameters(
    std::vector<std::string> const &parameters) {
  m_globalParameters.clear();
  for (auto const &fullParameter : parameters) {
    auto const globalParameter = removeTopFunctionIndex(fullParameter);
    checkParameterIsInAllDomains(globalParameter);
    checkGlobalParameterhasNoTies(globalParameter);

    m_globalParameters.emplace_back(GlobalParameter(globalParameter));
  }
}

void FitScriptGeneratorModel::setFittingMode(FittingMode const &fittingMode) {
  if (fittingMode == FittingMode::SimultaneousAndSequential)
    throw std::invalid_argument(
        "Fitting mode must be Sequential or Simultaneous.");
  m_fittingMode = fittingMode;
  m_globalTies.clear();
  m_presenter->setGlobalTies(m_globalTies);
  m_globalParameters.clear();
  m_presenter->setGlobalParameters(m_globalParameters);
}

void FitScriptGeneratorModel::checkParameterIsInAllDomains(
    std::string const &globalParameter) const {
  auto const hasParameter = [&globalParameter](auto const &fitDomain) {
    return fitDomain->hasParameter(globalParameter);
  };

  if (!std::all_of(m_fitDomains.cbegin(), m_fitDomains.cend(), hasParameter))
    throw std::invalid_argument(
        globalParameter +
        " cannot be global because it doesn't exist for ALL domains.");
}

void FitScriptGeneratorModel::checkGlobalParameterhasNoTies(
    std::string const &globalParameter) const {
  auto const isNotActive = [&globalParameter](auto const &fitDomain) {
    return !fitDomain->isParameterActive(globalParameter);
  };

  auto const hasGlobalTie = [&globalParameter](GlobalTie const &globalTie) {
    return globalParameter == removeTopFunctionIndex(globalTie.m_parameter);
  };

  if (std::any_of(m_fitDomains.cbegin(), m_fitDomains.cend(), isNotActive) ||
      std::any_of(m_globalTies.cbegin(), m_globalTies.cend(), hasGlobalTie))
    throw std::invalid_argument(globalParameter +
                                " cannot be global because it already has a "
                                "tie in at least one of the domains.");
}

void FitScriptGeneratorModel::checkParameterIsNotGlobal(
    std::string const &fullParameter) const {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  auto const isGlobal = [&parameter](GlobalParameter const &globalParameter) {
    return parameter == globalParameter.m_parameter;
  };

  if (std::any_of(m_globalParameters.cbegin(), m_globalParameters.cend(),
                  isGlobal)) {
    throw std::invalid_argument(
        fullParameter + " cannot be tied because it is a global parameter.");
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
