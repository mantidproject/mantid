// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::Kernel;
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

std::string replaceTopFunctionIndexWith(std::string const &functionIndex, std::size_t const &newIndex) {
  return "f" + std::to_string(newIndex) + "." + removeTopFunctionIndex(functionIndex);
}

std::string getTieRHS(std::string const &tie) {
  auto const tieSplit = splitStringBy(tie, "=");
  return tieSplit.size() > 1 ? tieSplit[1] : tieSplit[0];
}

std::string getLocalTie(std::string const &tie, FittingMode const &fittingMode) {
  if (!tie.empty() && fittingMode == FittingMode::SIMULTANEOUS)
    return removeTopFunctionIndex(getTieRHS(tie));
  return tie;
}

bool isFunctionIndex(std::string const &str) {
  auto const subStrings = splitStringBy(str, "f");
  if (subStrings.size() == 1)
    return isNumber(subStrings[0]);
  return false;
}

bool isSameDomain(std::size_t const &domainIndex, std::string const &fullParameter) {
  if (!fullParameter.empty() && !isNumber(fullParameter))
    return domainIndex == getFunctionIndexAt(fullParameter, 0);
  return true;
}

FitDomainIndex getDomainIndexOf(std::string const &fullParameter) {
  return FitDomainIndex(getFunctionIndexAt(fullParameter, 0));
}

std::string getParameterName(std::string const &constraint) {
  return splitParameterName(splitConstraintString(constraint).first).second;
}

std::string getFunctionIndex(std::string const &parameter) { return splitFunctionPrefix(parameter).first; }

std::string getAdjustedConstraint(std::string const &constraint) {
  auto const parameterName = getParameterName(constraint);
  auto const limits = splitConstraintString(constraint).second;
  return limits.first + "<" + parameterName + "<" + limits.second;
}

} // namespace

namespace MantidQt::MantidWidgets {

FitScriptGeneratorModel::FitScriptGeneratorModel()
    : m_presenter(), m_outputBaseName("Output_Fit"), m_fitDomains(), m_globalParameters(), m_globalTies(),
      m_fittingMode(FittingMode::SEQUENTIAL) {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {
  m_fitDomains.clear();
  m_globalTies.clear();
  m_globalParameters.clear();
}

void FitScriptGeneratorModel::subscribePresenter(IFitScriptGeneratorPresenter *presenter) { m_presenter = presenter; }

void FitScriptGeneratorModel::removeDomain(FitDomainIndex domainIndex) {
  if (domainIndex.value > m_fitDomains.size())
    return;

  auto const removeIter = m_fitDomains.begin() + domainIndex.value;
  if (removeIter != m_fitDomains.cend()) {
    m_fitDomains.erase(removeIter);
    checkGlobalTies();
  }
}

void FitScriptGeneratorModel::addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                 double startX, double endX) {
  if (hasWorkspaceDomain(workspaceName, workspaceIndex))
    throw std::invalid_argument("The '" + workspaceName + " (" + std::to_string(workspaceIndex.value) +
                                ")' domain already exists.");

  m_fitDomains.emplace_back(std::make_unique<FitDomain>(workspaceName, workspaceIndex, startX, endX));
}

bool FitScriptGeneratorModel::hasWorkspaceDomain(std::string const &workspaceName,
                                                 WorkspaceIndex workspaceIndex) const {
  return findWorkspaceDomain(workspaceName, workspaceIndex) != m_fitDomains.cend();
}

void FitScriptGeneratorModel::renameWorkspace(std::string const &workspaceName, std::string const &newName) {
  for (auto const &fitDomain : m_fitDomains) {
    if (fitDomain->workspaceName() == workspaceName)
      fitDomain->setWorkspaceName(newName);
  }
}

FitDomainIndex FitScriptGeneratorModel::findDomainIndex(std::string const &workspaceName,
                                                        WorkspaceIndex workspaceIndex) const {
  auto const iter = findWorkspaceDomain(workspaceName, workspaceIndex);
  if (iter != m_fitDomains.cend())
    return FitDomainIndex(std::distance(m_fitDomains.cbegin(), iter));

  throw std::invalid_argument("The domain '" + workspaceName + " (" + std::to_string(workspaceIndex.value) +
                              ")' could not be found.");
}

std::vector<std::unique_ptr<FitDomain>>::const_iterator
FitScriptGeneratorModel::findWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) const {
  auto const isMatch = [&](auto const &fitDomain) {
    return fitDomain->workspaceName() == workspaceName && fitDomain->workspaceIndex() == workspaceIndex;
  };

  return std::find_if(m_fitDomains.cbegin(), m_fitDomains.cend(), isMatch);
}

bool FitScriptGeneratorModel::updateStartX(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                           double startX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex.value]->setStartX(startX);
}

bool FitScriptGeneratorModel::updateEndX(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double endX) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex.value]->setEndX(endX);
}

void FitScriptGeneratorModel::removeFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                             std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex.value]->removeFunction(function);
  checkGlobalTies();
}

void FitScriptGeneratorModel::addFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex.value]->addFunction(createIFunction(function));
  checkGlobalTies();
}

void FitScriptGeneratorModel::setFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex.value]->setFunction(createIFunction(function));
  checkGlobalTies();
}

IFunction_sptr FitScriptGeneratorModel::getFunction(std::string const &workspaceName,
                                                    WorkspaceIndex workspaceIndex) const {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex.value]->getFunctionCopy();
}

std::string FitScriptGeneratorModel::getEquivalentFunctionIndexForDomain(std::string const &workspaceName,
                                                                         WorkspaceIndex workspaceIndex,
                                                                         std::string const &functionIndex) const {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return getEquivalentFunctionIndexForDomain(domainIndex, functionIndex);
}

std::string FitScriptGeneratorModel::getEquivalentFunctionIndexForDomain(FitDomainIndex domainIndex,
                                                                         std::string const &functionIndex) const {
  if (domainIndex.value >= numberOfDomains())
    throw std::invalid_argument("The domain index provided does not exist.");

  if (!functionIndex.empty() && m_fittingMode == FittingMode::SIMULTANEOUS)
    return replaceTopFunctionIndexWith(functionIndex, domainIndex.value);
  return functionIndex;
}

std::string FitScriptGeneratorModel::getEquivalentParameterTieForDomain(std::string const &workspaceName,
                                                                        WorkspaceIndex workspaceIndex,
                                                                        std::string const &fullParameter,
                                                                        std::string const &fullTie) const {
  if (fullTie.empty() || isNumber(fullTie) || !validTie(fullTie))
    return fullTie;

  if (m_fittingMode == FittingMode::SEQUENTIAL)
    return fullTie;

  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return getEquivalentParameterTieForDomain(domainIndex, fullParameter, fullTie);
}

std::string FitScriptGeneratorModel::getEquivalentParameterTieForDomain(FitDomainIndex domainIndex,
                                                                        std::string const &fullParameter,
                                                                        std::string const &fullTie) const {
  auto const parameterDomainIndex = getFunctionIndexAt(fullParameter, 0);
  auto const tieDomainIndex = getFunctionIndexAt(fullTie, 0);
  if (parameterDomainIndex == tieDomainIndex)
    return replaceTopFunctionIndexWith(fullTie, domainIndex.value);
  return fullTie;
}

std::string FitScriptGeneratorModel::getAdjustedFunctionIndex(std::string const &parameter) const {
  if (parameter.empty() || isNumber(parameter))
    return parameter;

  if (m_fittingMode == FittingMode::SEQUENTIAL)
    return parameter;
  return removeTopFunctionIndex(parameter);
}

std::string FitScriptGeneratorModel::getFullParameter(FitDomainIndex domainIndex, std::string const &parameter) const {
  if (m_fittingMode == FittingMode::SEQUENTIAL)
    return parameter;
  return "f" + std::to_string(domainIndex.value) + "." + parameter;
}

std::string FitScriptGeneratorModel::getFullTie(FitDomainIndex domainIndex, std::string const &tie) const {
  if (tie.empty() || isNumber(tie))
    return tie;
  return getFullParameter(domainIndex, tie);
}

void FitScriptGeneratorModel::updateParameterValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                   std::string const &fullParameter, double newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  if (!hasGlobalTie(fullParameter)) {
    auto const parameter = getAdjustedFunctionIndex(fullParameter);
    m_fitDomains[domainIndex.value]->setParameterValue(parameter, newValue);

    updateParameterValuesWithLocalTieTo(domainIndex, parameter, newValue);
    if (m_fittingMode == FittingMode::SIMULTANEOUS) {
      updateParameterValuesWithGlobalTieTo(fullParameter, newValue);
    }
  }
}

void FitScriptGeneratorModel::updateParameterValuesWithLocalTieTo(FitDomainIndex domainIndex,
                                                                  std::string const &parameter, double newValue) {
  for (auto const &tiedParameterName : m_fitDomains[domainIndex.value]->getParametersTiedTo(parameter)) {
    m_fitDomains[domainIndex.value]->setParameterValue(tiedParameterName, newValue);
  }
}

void FitScriptGeneratorModel::updateParameterValuesWithGlobalTieTo(std::string const &fullParameter, double newValue) {
  // Deep copy so that global ties can be removed whilst in this for loop. This
  // will happen if a global tie has been invalidated.
  auto const globalTies = m_globalTies;
  for (auto const &globalTie : globalTies)
    if (fullParameter == globalTie.m_tie)
      updateParameterValueInGlobalTie(globalTie, newValue);
}

void FitScriptGeneratorModel::updateParameterValueInGlobalTie(GlobalTie const &globalTie, double newValue) {
  if (validGlobalTie(globalTie.m_parameter, globalTie.m_tie)) {
    auto const domainIndex = getFunctionIndexAt(globalTie.m_parameter, 0);
    m_fitDomains[domainIndex]->setParameterValue(getAdjustedFunctionIndex(globalTie.m_parameter), newValue);
  } else {
    clearGlobalTie(globalTie.m_parameter);
  }
}

void FitScriptGeneratorModel::updateAttributeValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                   std::string const &fullAttribute,
                                                   IFunction::Attribute const &newValue) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex.value]->setAttributeValue(getAdjustedFunctionIndex(fullAttribute), newValue);
}

void FitScriptGeneratorModel::updateParameterTie(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                 std::string const &fullParameter, std::string const &tie) {
  if (validTie(tie)) {
    auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
    updateParameterTie(domainIndex, fullParameter, tie);
  } else {
    g_log.warning("Invalid tie '" + tie + "' provided.");
  }
}

void FitScriptGeneratorModel::updateParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                                                 std::string const &fullTie) {
  checkParameterIsNotGlobal(fullParameter);

  if (m_fittingMode == FittingMode::SEQUENTIAL || isSameDomain(domainIndex.value, fullTie))
    updateLocalParameterTie(domainIndex, fullParameter, fullTie);
  else
    updateGlobalParameterTie(domainIndex, fullParameter, fullTie);
}

void FitScriptGeneratorModel::updateLocalParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                                                      std::string const &fullTie) {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  auto const tie = getLocalTie(fullTie, m_fittingMode);

  if (parameter != tie && validParameter(domainIndex, fullParameter)) {
    if (m_fitDomains[domainIndex.value]->updateParameterTie(parameter, tie))
      clearGlobalTie(fullParameter);
    else
      g_log.warning("Invalid tie '" + fullTie + "' provided.");
  }
}

void FitScriptGeneratorModel::updateGlobalParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                                                       std::string const &fullTie) {
  if (validParameter(domainIndex, fullParameter)) {
    if (validGlobalTie(fullParameter, fullTie)) {
      clearGlobalTie(fullParameter);

      auto const parameter = getAdjustedFunctionIndex(fullParameter);
      auto const tieValue = getParameterValue(getDomainIndexOf(fullTie), fullTie);

      m_fitDomains[domainIndex.value]->clearParameterTie(parameter);
      m_fitDomains[domainIndex.value]->setParameterValue(parameter, tieValue);

      m_globalTies.emplace_back(GlobalTie(fullParameter, fullTie));
    } else {
      g_log.warning("Invalid tie '" + fullTie + "' provided.");
    }
  }
}

void FitScriptGeneratorModel::removeParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                        std::string const &fullParameter) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex.value]->removeParameterConstraint(getAdjustedFunctionIndex(fullParameter));
}

void FitScriptGeneratorModel::updateParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                                        std::string const &functionIndex,
                                                        std::string const &constraint) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  m_fitDomains[domainIndex.value]->updateParameterConstraint(getAdjustedFunctionIndex(functionIndex),
                                                             getParameterName(constraint), constraint);
}

bool FitScriptGeneratorModel::validParameter(std::string const &fullParameter) const {
  return validParameter(getDomainIndexOf(fullParameter), fullParameter);
}

bool FitScriptGeneratorModel::validParameter(FitDomainIndex domainIndex, std::string const &fullParameter) const {
  if (domainIndex.value < numberOfDomains()) {
    auto const parameter = getAdjustedFunctionIndex(fullParameter);
    return m_fitDomains[domainIndex.value]->hasParameter(parameter);
  }
  return false;
}

bool FitScriptGeneratorModel::validTie(std::string const &tie) const {
  if (tie.empty() || isNumber(tie))
    return true;

  auto const subStrings = splitStringBy(tie, ".");
  switch (m_fittingMode) {
  case FittingMode::SEQUENTIAL:
    return 1 <= subStrings.size() && subStrings.size() <= 2;
  case FittingMode::SIMULTANEOUS:
    return 2 <= subStrings.size() && subStrings.size() <= 3 && isFunctionIndex(subStrings[0]);
  default:
    throw std::invalid_argument("Fitting mode must be SEQUENTIAL or SIMULTANEOUS.");
  }
}

bool FitScriptGeneratorModel::validGlobalTie(std::string const &fullParameter, std::string const &fullTie) const {
  if (!fullTie.empty() && !isNumber(fullTie) && validParameter(fullTie)) {
    auto const domainIndex = getDomainIndexOf(fullParameter);
    auto const tieDomainIndex = getDomainIndexOf(fullTie);
    return isParameterValueWithinConstraints(domainIndex, fullParameter, getParameterValue(tieDomainIndex, fullTie));
  }
  return false;
}

bool FitScriptGeneratorModel::isParameterValueWithinConstraints(FitDomainIndex domainIndex,
                                                                std::string const &fullParameter, double value) const {
  if (domainIndex.value < numberOfDomains()) {
    auto const parameter = getAdjustedFunctionIndex(fullParameter);
    return m_fitDomains[domainIndex.value]->isParameterValueWithinConstraints(parameter, value);
  }
  return false;
}

void FitScriptGeneratorModel::clearGlobalTie(std::string const &fullParameter) {
  auto const removeIter = findGlobalTie(fullParameter);
  if (removeIter != m_globalTies.cend()) {
    m_globalTies.erase(removeIter);
    m_presenter->setGlobalTies(m_globalTies);
  }
}

void FitScriptGeneratorModel::checkGlobalTies() {
  auto const isTieInvalid = [&](GlobalTie &globalTie) {
    tryToAdjustParameterInGlobalTieIfInvalidated(globalTie);
    tryToAdjustTieInGlobalTieIfInvalidated(globalTie);

    return !validParameter(globalTie.m_parameter) || !validGlobalTie(globalTie.m_parameter, globalTie.m_tie);
  };

  auto const iter = std::remove_if(m_globalTies.begin(), m_globalTies.end(), isTieInvalid);

  if (iter != m_globalTies.cend())
    m_globalTies.erase(iter, m_globalTies.cend());

  m_presenter->setGlobalTies(m_globalTies);
}

void FitScriptGeneratorModel::tryToAdjustParameterInGlobalTieIfInvalidated(GlobalTie &globalTie) {
  if (!validParameter(globalTie.m_parameter)) {
    auto const promotedParam = globalTie.toCompositeParameter(globalTie.m_parameter);
    auto const demotedParam = globalTie.toNonCompositeParameter(globalTie.m_parameter);

    if (validParameter(promotedParam)) {
      globalTie.m_parameter = promotedParam;
    } else if (validParameter(demotedParam)) {
      globalTie.m_parameter = demotedParam;
    }
  }
}

void FitScriptGeneratorModel::tryToAdjustTieInGlobalTieIfInvalidated(GlobalTie &globalTie) {
  if (!validGlobalTie(globalTie.m_parameter, globalTie.m_tie)) {
    auto const promotedTie = globalTie.toCompositeParameter(globalTie.m_tie);
    auto const demotedTie = globalTie.toNonCompositeParameter(globalTie.m_tie);

    if (validGlobalTie(globalTie.m_parameter, promotedTie)) {
      globalTie.m_tie = promotedTie;
    } else if (validGlobalTie(globalTie.m_parameter, demotedTie)) {
      globalTie.m_tie = demotedTie;
    }
  }
}

bool FitScriptGeneratorModel::hasGlobalTie(std::string const &fullParameter) const {
  return findGlobalTie(fullParameter) != m_globalTies.cend();
}

std::vector<GlobalTie>::const_iterator FitScriptGeneratorModel::findGlobalTie(std::string const &fullParameter) const {
  return std::find_if(m_globalTies.cbegin(), m_globalTies.cend(),
                      [&fullParameter](GlobalTie const &globalTie) { return globalTie.m_parameter == fullParameter; });
}

void FitScriptGeneratorModel::setGlobalParameters(std::vector<std::string> const &parameters) {
  m_globalParameters.clear();
  for (auto const &fullParameter : parameters) {
    auto const globalParameter = removeTopFunctionIndex(fullParameter);
    checkParameterIsInAllDomains(globalParameter);
    checkGlobalParameterhasNoTies(globalParameter);

    m_globalParameters.emplace_back(GlobalParameter(globalParameter));
  }
}

void FitScriptGeneratorModel::setOutputBaseName(std::string const &outputBaseName) {
  m_outputBaseName = outputBaseName;
}

void FitScriptGeneratorModel::setFittingMode(FittingMode fittingMode) {
  if (fittingMode == FittingMode::SEQUENTIAL_AND_SIMULTANEOUS)
    throw std::invalid_argument("Fitting mode must be SEQUENTIAL or SIMULTANEOUS.");

  m_fittingMode = fittingMode;
  m_globalTies.clear();
  m_presenter->setGlobalTies(m_globalTies);
  m_globalParameters.clear();
  m_presenter->setGlobalParameters(m_globalParameters);
}

bool FitScriptGeneratorModel::isSimultaneousMode() const { return m_fittingMode == FittingMode::SIMULTANEOUS; }

std::string FitScriptGeneratorModel::getDomainName(FitDomainIndex domainIndex) const {
  if (domainIndex.value < numberOfDomains())
    return m_fitDomains[domainIndex.value]->domainName();

  throw std::runtime_error("The domain index provided does not exist.");
}

bool FitScriptGeneratorModel::hasParameter(FitDomainIndex domainIndex, std::string const &fullParameter) const {
  return getParameterProperty(&FitDomain::hasParameter, domainIndex, fullParameter);
}

void FitScriptGeneratorModel::setParameterValue(FitDomainIndex domainIndex, std::string const &fullParameter,
                                                double value) {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  if (domainIndex.value >= numberOfDomains())
    throw std::runtime_error("The domain index provided does not exist.");

  m_fitDomains[domainIndex.value]->setParameterValue(parameter, value);
}

void FitScriptGeneratorModel::setParameterFixed(FitDomainIndex domainIndex, std::string const &fullParameter,
                                                bool fix) {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  if (domainIndex.value >= numberOfDomains())
    throw std::runtime_error("The domain index provided does not exist.");

  m_fitDomains[domainIndex.value]->setParameterFixed(parameter, fix);
}

void FitScriptGeneratorModel::setParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                                              std::string const &tie) {
  if (domainIndex.value >= numberOfDomains())
    throw std::runtime_error("The domain index provided does not exist.");

  auto const fullTie = getFullTie(domainIndex, tie);
  if (validTie(fullTie))
    updateParameterTie(domainIndex, fullParameter, fullTie);
}

void FitScriptGeneratorModel::setParameterConstraint(FitDomainIndex domainIndex, std::string const &fullParameter,
                                                     std::string const &constraint) {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  if (domainIndex.value >= numberOfDomains())
    throw std::runtime_error("The domain index provided does not exist.");

  if (!constraint.empty()) {
    m_fitDomains[domainIndex.value]->updateParameterConstraint(
        getFunctionIndex(parameter), getParameterName(constraint), getAdjustedConstraint(constraint));
  } else {
    m_fitDomains[domainIndex.value]->removeParameterConstraint(parameter);
  }
}

double FitScriptGeneratorModel::getParameterValue(FitDomainIndex domainIndex, std::string const &fullParameter) const {
  return getParameterProperty(&FitDomain::getParameterValue, domainIndex, fullParameter);
}

bool FitScriptGeneratorModel::isParameterFixed(FitDomainIndex domainIndex, std::string const &fullParameter) const {
  return getParameterProperty(&FitDomain::isParameterFixed, domainIndex, fullParameter);
}

std::string FitScriptGeneratorModel::getParameterTie(FitDomainIndex domainIndex,
                                                     std::string const &fullParameter) const {
  return getParameterProperty(&FitDomain::getParameterTie, domainIndex, fullParameter);
}

std::string FitScriptGeneratorModel::getParameterConstraint(FitDomainIndex domainIndex,
                                                            std::string const &fullParameter) const {
  return getParameterProperty(&FitDomain::getParameterConstraint, domainIndex, fullParameter);
}

void FitScriptGeneratorModel::checkParameterIsInAllDomains(std::string const &globalParameter) const {
  auto const domainHasParameter = [&globalParameter](auto const &fitDomain) {
    return fitDomain->hasParameter(globalParameter);
  };

  if (!std::all_of(m_fitDomains.cbegin(), m_fitDomains.cend(), domainHasParameter))
    throw std::invalid_argument(globalParameter + " cannot be global because it doesn't exist for ALL domains.");
}

void FitScriptGeneratorModel::checkGlobalParameterhasNoTies(std::string const &globalParameter) const {
  auto const isNotActive = [&globalParameter](auto const &fitDomain) {
    return !fitDomain->isParameterActive(globalParameter);
  };

  auto const parameterHasGlobalTie = [&globalParameter](GlobalTie const &globalTie) {
    return globalParameter == removeTopFunctionIndex(globalTie.m_parameter);
  };

  if (std::any_of(m_fitDomains.cbegin(), m_fitDomains.cend(), isNotActive) ||
      std::any_of(m_globalTies.cbegin(), m_globalTies.cend(), parameterHasGlobalTie))
    throw std::invalid_argument(globalParameter + " cannot be global because it already has a "
                                                  "tie in at least one of the domains.");
}

void FitScriptGeneratorModel::checkParameterIsNotGlobal(std::string const &fullParameter) const {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  auto const isGlobal = [&parameter](GlobalParameter const &globalParameter) {
    return parameter == globalParameter.m_parameter;
  };

  if (std::any_of(m_globalParameters.cbegin(), m_globalParameters.cend(), isGlobal)) {
    throw std::invalid_argument(fullParameter + " cannot be tied because it is a global parameter.");
  }
}

bool FitScriptGeneratorModel::checkFunctionExistsInAllDomains() const {
  auto const hasFunction = [](auto const &fitDomain) { return fitDomain->getFunctionCopy() != nullptr; };
  return std::all_of(m_fitDomains.cbegin(), m_fitDomains.cend(), hasFunction);
}

bool FitScriptGeneratorModel::checkFunctionIsSameForAllDomains() const {
  if (numberOfDomains() == 0u || m_fitDomains[0u]->getFunctionCopy() == nullptr) {
    return false;
  }

  auto const functionString = m_fitDomains[0u]->getFunctionCopy()->asString();
  auto const hasSameFunction = [&functionString](auto const &fitDomain) {
    return fitDomain->getFunctionCopy()->asString() == functionString;
  };
  return std::all_of(m_fitDomains.cbegin(), m_fitDomains.cend(), hasSameFunction);
}

std::tuple<bool, std::string> FitScriptGeneratorModel::isValid() const {
  std::string message;
  if (numberOfDomains() == 0u)
    message = "Domain data must be loaded before generating a python script.";
  else if (!checkFunctionExistsInAllDomains())
    message = "A function must exist in ALL domains to generate a python script.";

  if (m_outputBaseName.empty())
    message = "The Output Base Name must not be empty, please provide an Output Base Name.";

  bool valid(message.empty());
  if (valid)
    message = generatePermissibleWarnings();

  return {valid, message};
}

std::string FitScriptGeneratorModel::generatePermissibleWarnings() const {
  if (m_fittingMode == FittingMode::SEQUENTIAL && !checkFunctionIsSameForAllDomains()) {
    return "Note that each domain should have the same fit function, including ties and constraints, for a sequential "
           "fit. This is not the case for the fit functions you have provided. \n\nThe sequential fit script will be "
           "generated using the fit function in the first domain.";
  }
  return "";
}

std::string FitScriptGeneratorModel::generatePythonFitScript(
    std::tuple<std::string, std::string, std::string, std::string, std::string, bool> const &fitOptions,
    std::string const &filepath) {
  auto generateScript = AlgorithmManager::Instance().create("GeneratePythonFitScript");
  generateScript->initialize();
  generateScript->setProperty("InputWorkspaces", getInputWorkspaces());
  generateScript->setProperty("WorkspaceIndices", getWorkspaceIndices());
  generateScript->setProperty("StartXs", getStartXs());
  generateScript->setProperty("EndXs", getEndXs());

  generateScript->setProperty("FittingType", getFittingType());
  generateScript->setProperty("Function", getFunction());

  auto const [maxIterations, minimizer, costFunction, evaluationType, outputBaseName, plotOutput] = fitOptions;
  generateScript->setProperty("MaxIterations", maxIterations);
  generateScript->setProperty("Minimizer", minimizer);
  generateScript->setProperty("CostFunction", costFunction);
  generateScript->setProperty("EvaluationType", evaluationType);
  generateScript->setProperty("OutputBaseName", outputBaseName);
  generateScript->setProperty("PlotOutput", plotOutput);

  generateScript->setProperty("Filepath", filepath);
  generateScript->execute();

  return generateScript->getPropertyValue("ScriptText");
}

std::vector<std::string> FitScriptGeneratorModel::getInputWorkspaces() const {
  auto const getWSName = [](std::unique_ptr<FitDomain> const &domain) { return domain->workspaceName(); };
  return transformDomains<std::string>(getWSName);
}

std::vector<std::size_t> FitScriptGeneratorModel::getWorkspaceIndices() const {
  auto const getWSIndex = [](std::unique_ptr<FitDomain> const &domain) { return domain->workspaceIndex().value; };
  return transformDomains<std::size_t>(getWSIndex);
}

std::vector<double> FitScriptGeneratorModel::getStartXs() const {
  auto const getStartX = [](std::unique_ptr<FitDomain> const &domain) { return domain->startX(); };
  return transformDomains<double>(getStartX);
}

std::vector<double> FitScriptGeneratorModel::getEndXs() const {
  auto const getEndX = [](std::unique_ptr<FitDomain> const &domain) { return domain->endX(); };
  return transformDomains<double>(getEndX);
}

template <typename T, typename Function>
std::vector<T> FitScriptGeneratorModel::transformDomains(Function const &func) const {
  std::vector<T> domainData;
  domainData.reserve(numberOfDomains());
  std::transform(m_fitDomains.cbegin(), m_fitDomains.cend(), std::back_inserter(domainData), func);
  return domainData;
}

std::string FitScriptGeneratorModel::getFittingType() const {
  switch (m_fittingMode) {
  case FittingMode::SEQUENTIAL:
    return "Sequential";
  case FittingMode::SIMULTANEOUS:
    return "Simultaneous";
  default:
    throw std::invalid_argument("Fitting mode must be SEQUENTIAL or SIMULTANEOUS.");
  }
}

IFunction_sptr FitScriptGeneratorModel::getFunction() const {
  switch (m_fittingMode) {
  case FittingMode::SEQUENTIAL:
    return m_fitDomains[0u]->getFunctionCopy();
  case FittingMode::SIMULTANEOUS:
    return getMultiDomainFunction();
  default:
    throw std::invalid_argument("Fitting mode must be SEQUENTIAL or SIMULTANEOUS.");
  }
}

IFunction_sptr FitScriptGeneratorModel::getMultiDomainFunction() const {
  auto multiDomainFunction = std::make_shared<MultiDomainFunction>();

  for (auto i = 0u; i < numberOfDomains(); ++i) {
    multiDomainFunction->addFunction(m_fitDomains[i]->getFunctionCopy());
    multiDomainFunction->setDomainIndex(i, i);
  }

  addGlobalParameterTies(multiDomainFunction);
  addGlobalTies(multiDomainFunction);
  return multiDomainFunction;
}

void FitScriptGeneratorModel::addGlobalParameterTies(MultiDomainFunction_sptr &function) const {
  for (auto const &globalParameter : m_globalParameters)
    function->addTies(constructGlobalParameterTie(globalParameter));
}

std::string FitScriptGeneratorModel::constructGlobalParameterTie(GlobalParameter const &globalParameter) const {
  std::string tie = "f0." + globalParameter.m_parameter;
  for (auto i = 1u; i < numberOfDomains(); ++i)
    tie += "=f" + std::to_string(i) + "." + globalParameter.m_parameter;
  return tie;
}

void FitScriptGeneratorModel::addGlobalTies(MultiDomainFunction_sptr &function) const {
  for (auto const &globalTie : m_globalTies)
    function->addTies(globalTie.asString());
}

} // namespace MantidQt::MantidWidgets
