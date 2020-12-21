// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"

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

std::size_t getFunctionIndexAt(std::string const &parameter,
                               std::size_t const &index) {
  auto subStrings = splitStringBy(parameter, "f.");
  subStrings.pop_back();
  if (index < subStrings.size())
    return std::stoull(subStrings[index]);

  throw std::invalid_argument("Incorrect function index provided.");
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

std::string getLocalTie(std::string const &tie,
                        FittingMode const &fittingMode) {
  if (!tie.empty() && fittingMode == FittingMode::Simultaneous)
    return removeTopFunctionIndex(getTieRHS(tie));
  return tie;
}

std::string getLocalParameter(std::string const &parameter,
                              FittingMode const &fittingMode) {
  if (parameter.empty())
    return parameter;

  switch (fittingMode) {
  case FittingMode::Sequential:
    return parameter;
  case FittingMode::Simultaneous:
    return removeTopFunctionIndex(parameter);
  default:
    throw std::invalid_argument(
        "Fitting mode must be Sequential or Simultaneous.");
  }
}

bool isNumber(std::string const &str) {
  return !str.empty() &&
         str.find_first_not_of("0123456789.-") == std::string::npos;
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

bool validTie(std::string const &tie, FittingMode const &fittingMode) {
  if (tie.empty() || isNumber(tie))
    return true;

  auto const subStrings = splitStringBy(tie, ".");
  switch (fittingMode) {
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

} // namespace

namespace MantidQt {
namespace MantidWidgets {

FitScriptGeneratorModel::FitScriptGeneratorModel()
    : m_presenter(), m_fitDomains(), m_fittingMode(FittingMode::Sequential) {}

FitScriptGeneratorModel::~FitScriptGeneratorModel() {}

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
         m_fitDomains.cend();
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
  checkGlobalTies();
}

void FitScriptGeneratorModel::addFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].addFunction(createIFunction(function));
  checkGlobalTies();
}

void FitScriptGeneratorModel::setFunction(std::string const &workspaceName,
                                          WorkspaceIndex workspaceIndex,
                                          std::string const &function) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  m_fitDomains[domainIndex].setFunction(createIFunction(function));
  checkGlobalTies();
}

IFunction_sptr
FitScriptGeneratorModel::getFunction(std::string const &workspaceName,
                                     WorkspaceIndex workspaceIndex) {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
  return m_fitDomains[domainIndex].getFunction();
}

std::string FitScriptGeneratorModel::getEquivalentParameterForDomain(
    std::string const &workspaceName, WorkspaceIndex workspaceIndex,
    std::string const &fullParameter) const {
  auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);

  switch (m_fittingMode) {
  case FittingMode::Sequential:
    return fullParameter;
  case FittingMode::Simultaneous:
    return "f" + std::to_string(domainIndex) + "." +
           removeTopFunctionIndex(fullParameter);
  default:
    throw std::invalid_argument(
        "Fitting mode must be Sequential or Simultaneous.");
  }
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
  if (validTie(tie, m_fittingMode)) {
    auto const domainIndex = findDomainIndex(workspaceName, workspaceIndex);
    updateParameterTie(domainIndex, parameter, tie);
  } else {
    g_log.warning("Invalid tie '" + tie + "' provided.");
  }
}

void FitScriptGeneratorModel::updateParameterTie(
    std::size_t const &domainIndex, std::string const &fullParameter,
    std::string const &fullTie) {
  if (m_fittingMode == FittingMode::Sequential ||
      isSameDomain(domainIndex, fullTie))
    updateLocalParameterTie(domainIndex, fullParameter, fullTie);
  else
    updateGlobalParameterTie(domainIndex, fullParameter, fullTie);
}

void FitScriptGeneratorModel::updateLocalParameterTie(
    std::size_t const &domainIndex, std::string const &fullParameter,
    std::string const &fullTie) {
  auto const parameter = getLocalParameter(fullParameter, m_fittingMode);
  auto const tie = getLocalTie(fullTie, m_fittingMode);

  if (parameter != tie && validParameter(fullParameter)) {
    if (m_fitDomains[domainIndex].updateParameterTie(parameter, tie))
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

      m_fitDomains[domainIndex].clearParameterTie(fullParameter);
      m_globalTies.emplace_back(GlobalTie(fullParameter, fullTie));
    } else {
      g_log.warning("Invalid tie '" + fullTie + "' provided.");
    }
  }
}

bool FitScriptGeneratorModel::validParameter(
    std::string const &fullParameter) const {
  auto const domainIndex = getFunctionIndexAt(fullParameter, 0);
  auto const parameter = getLocalParameter(fullParameter, m_fittingMode);
  if (domainIndex < numberOfDomains())
    return m_fitDomains[domainIndex].hasParameter(parameter);
  return false;
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
  auto const valid = [&](GlobalTie const &globalTie) {
    return !validParameter(globalTie.m_parameter) ||
           !validGlobalTie(globalTie.m_tie);
  };

  auto const iter =
      std::remove_if(m_globalTies.begin(), m_globalTies.end(), valid);

  if (iter != m_globalTies.cend()) {
    m_globalTies.erase(iter, m_globalTies.cend());
    m_presenter->setGlobalTies(m_globalTies);
  }
}

std::vector<GlobalTie>::const_iterator
FitScriptGeneratorModel::findGlobalTie(std::string const &fullParameter) const {
  return std::find_if(m_globalTies.cbegin(), m_globalTies.cend(),
                      [&fullParameter](GlobalTie const &globalTie) {
                        return globalTie.m_parameter == fullParameter;
                      });
}

void FitScriptGeneratorModel::setFittingMode(FittingMode const &fittingMode) {
  if (fittingMode == FittingMode::SimultaneousAndSequential)
    throw std::invalid_argument(
        "Fitting mode must be Sequential or Simultaneous.");
  m_fittingMode = fittingMode;
}

} // namespace MantidWidgets
} // namespace MantidQt
