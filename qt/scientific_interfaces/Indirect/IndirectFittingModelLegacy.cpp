// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFittingModelLegacy.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/TableRow.h"

#include <algorithm>
#include <numeric>
#include <set>

#include <boost/algorithm/string.hpp>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

std::string cutLastOf(std::string const &str, std::string const &delimiter) {
  auto const cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}

bool equivalentWorkspaces(MatrixWorkspace_const_sptr lhs,
                          MatrixWorkspace_const_sptr rhs) {
  if (!lhs || !rhs)
    return false;
  else if (lhs->getName() == "" && rhs->getName() == "")
    return lhs == rhs;
  return lhs->getName() == rhs->getName();
}

/**
 * @return  True if the first function precedes the second when ordering by
 *          name.
 */
bool functionNameComparator(IFunction_const_sptr first,
                            IFunction_const_sptr second) {
  return first->name() < second->name();
}

/**
 * Extracts the functions from a composite function into a vector.
 *
 * @param composite The composite function.
 * @return          A vector of the functions in the specified composite
 *                  function.
 */
std::vector<IFunction_const_sptr>
extractFunctions(const CompositeFunction &composite) {
  std::vector<IFunction_const_sptr> functions;
  functions.reserve(composite.nFunctions());

  for (auto i = 0u; i < composite.nFunctions(); ++i)
    functions.emplace_back(composite.getFunction(i));
  return functions;
}

bool equivalentFunctions(IFunction_const_sptr func1,
                         IFunction_const_sptr func2);

/*
 * Checks whether the specified composite functions have the same composition.
 *
 * @param composite1 Function to compare.
 * @param composite2 Function to compare.
 * @return           True if the specified functions have the same composition,
 *                   False otherwise.
 */
bool equivalentComposites(const CompositeFunction &composite1,
                          const CompositeFunction &composite2) {

  if (composite1.nFunctions() != composite2.nFunctions() ||
      composite1.nParams() != composite2.nParams()) {
    return false;
  } else {
    auto functions1 = extractFunctions(composite1);
    auto functions2 = extractFunctions(composite2);
    std::sort(functions1.begin(), functions1.end(), functionNameComparator);
    std::sort(functions2.begin(), functions2.end(), functionNameComparator);

    for (auto i = 0u; i < functions1.size(); ++i) {
      if (!equivalentFunctions(functions1[i], functions2[i]))
        return false;
    }
    return true;
  }
}

/*
 * Checks whether the specified functions have the same composition.
 *
 * @param func1 Function to compare.
 * @param func2 Function to compare.
 * @return      True if the specified functions have the same composition,
 *              False otherwise.
 */
bool equivalentFunctions(IFunction_const_sptr func1,
                         IFunction_const_sptr func2) {
  const auto composite1 =
      boost::dynamic_pointer_cast<const CompositeFunction>(func1);
  const auto composite2 =
      boost::dynamic_pointer_cast<const CompositeFunction>(func2);

  if (composite1 && composite2)
    return equivalentComposites(*composite1, *composite2);
  else if (func1 && func2 && !composite1 && !composite2)
    return func1->name() == func2->name();
  return false;
}

std::ostringstream &addInputString(IndirectFitDataLegacy *fitData,
                                   std::ostringstream &stream) {
  const auto &name = fitData->workspace()->getName();
  if (!name.empty()) {
    auto addToStream = [&](std::size_t spectrum) {
      stream << name << ",i" << spectrum << ";";
    };
    fitData->applySpectra(addToStream);
    return stream;
  } else
    throw std::runtime_error(
        "Workspace name is empty. The sample workspace may not be loaded.");
}

std::string constructInputString(
    const std::vector<std::unique_ptr<IndirectFitDataLegacy>> &fittingData) {
  std::ostringstream input;
  for (const auto &fitData : fittingData)
    addInputString(fitData.get(), input);
  return input.str();
}

void addInputDataToSimultaneousFit(IAlgorithm_sptr fitAlgorithm,
                                   MatrixWorkspace_sptr workspace,
                                   std::size_t spectrum,
                                   const std::pair<double, double> &xRange,
                                   const std::vector<double> &excludeRegions,
                                   const std::string &suffix) {
  fitAlgorithm->setProperty("InputWorkspace" + suffix, workspace);
  fitAlgorithm->setProperty("StartX" + suffix, xRange.first);
  fitAlgorithm->setProperty("EndX" + suffix, xRange.second);
  fitAlgorithm->setProperty("WorkspaceIndex" + suffix,
                            boost::numeric_cast<int>(spectrum));

  if (!excludeRegions.empty())
    fitAlgorithm->setProperty("Exclude" + suffix, excludeRegions);
}

void addInputDataToSimultaneousFit(
    IAlgorithm_sptr fitAlgorithm,
    const std::unique_ptr<IndirectFitDataLegacy> &fitData,
    std::size_t &counter) {
  const auto workspace = fitData->workspace();
  const auto addData = [&](std::size_t spectrum) {
    const auto suffix = counter == 0 ? "" : "_" + std::to_string(counter);
    addInputDataToSimultaneousFit(
        fitAlgorithm, workspace, spectrum, fitData->getRange(spectrum),
        fitData->excludeRegionsVector(spectrum), suffix);
    counter += 1;
  };
  fitData->applySpectra(addData);
}

void addInputDataToSimultaneousFit(
    IAlgorithm_sptr fitAlgorithm,
    const std::unique_ptr<IndirectFitDataLegacy> &fitData,
    const std::pair<double, double> &range, const std::vector<double> &exclude,
    std::size_t &counter) {
  const auto workspace = fitData->workspace();
  const auto addData = [&](std::size_t spectrum) {
    const auto suffix = counter == 0 ? "" : "_" + std::to_string(counter);
    addInputDataToSimultaneousFit(fitAlgorithm, workspace, spectrum, range,
                                  exclude, suffix);
    counter += 1;
  };
  fitData->applySpectra(addData);
}

void addInputDataToSimultaneousFit(
    IAlgorithm_sptr fitAlgorithm,
    const std::vector<std::unique_ptr<IndirectFitDataLegacy>> &fittingData) {
  std::size_t counter = 0;
  for (const auto &data : fittingData)
    addInputDataToSimultaneousFit(fitAlgorithm, data, counter);
}

void addInputDataToSimultaneousFit(
    IAlgorithm_sptr fitAlgorithm,
    const std::vector<std::unique_ptr<IndirectFitDataLegacy>> &fittingData,
    const std::pair<double, double> &range,
    const std::vector<double> &exclude) {
  std::size_t counter = 0;
  for (const auto &data : fittingData)
    addInputDataToSimultaneousFit(fitAlgorithm, data, range, exclude, counter);
}

template <typename Map> Map combine(const Map &mapA, const Map &mapB) {
  Map newMap(mapA);
  newMap.insert(std::begin(mapB), std::end(mapB));
  return newMap;
}

std::unordered_map<std::string, std::string>
shortToLongParameterNames(IFunction_sptr function) {
  std::unordered_map<std::string, std::string> shortToLong;
  for (const auto &name : function->getParameterNames())
    shortToLong[name.substr(name.rfind(".") + 1)] = name;
  return shortToLong;
}

template <typename Map, typename KeyMap>
Map mapKeys(const Map &map, const KeyMap &mapping) {
  Map mapped;
  for (const auto value : map) {
    auto it = mapping.find(value.first);
    if (it != mapping.end())
      mapped[it->second] = value.second;
  }
  return mapped;
}

void removeFromADSIfExists(const std::string &name) {
  if (AnalysisDataService::Instance().doesExist(name))
    AnalysisDataService::Instance().remove(name);
}

void cleanTemporaries(const std::string &base) {
  removeFromADSIfExists(base + "_Parameters");
  removeFromADSIfExists(base + "_Workspace");
  removeFromADSIfExists(base + "_NormalisedCovarianceMatrix");
}

void cleanTemporaries(const std::string &base,
                      const std::unique_ptr<IndirectFitDataLegacy> &fitData) {
  removeFromADSIfExists(base);

  const auto clean = [&](std::size_t index, std::size_t /*unused*/) {
    cleanTemporaries(base + "_" + std::to_string(index));
  };
  fitData->applyEnumeratedSpectraLegacy(clean);
}

void cleanTemporaries(
    const std::string &algorithmName,
    const std::vector<std::unique_ptr<IndirectFitDataLegacy>> &fittingData) {
  const auto prefix = "__" + algorithmName + "_ws";
  for (auto i = 0u; i < fittingData.size(); ++i)
    cleanTemporaries(prefix + std::to_string(i + 1), fittingData[i]);
}

CompositeFunction_sptr createMultiDomainFunction(IFunction_sptr function,
                                                 std::size_t numberOfDomains) {
  auto multiDomainFunction = boost::make_shared<MultiDomainFunction>();

  for (auto i = 0u; i < numberOfDomains; ++i) {
    multiDomainFunction->addFunction(function);
    multiDomainFunction->setDomainIndex(i, i);
  }
  return multiDomainFunction;
}

IFunction_sptr extractFirstInnerFunction(IFunction_sptr function) {
  if (const auto multiDomain =
          boost::dynamic_pointer_cast<MultiDomainFunction>(function)) {
    if (multiDomain->nFunctions() > 0)
      return multiDomain->getFunction(0);
  }
  return function;
}

IFunction_sptr extractFirstInnerFunction(const std::string &function) {
  return extractFirstInnerFunction(
      FunctionFactory::Instance().createInitialized(function));
}

template <typename WorkspaceType>
boost::shared_ptr<WorkspaceType>
getWorkspaceOutput(IAlgorithm_sptr algorithm, const std::string &propertyName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceType>(
      algorithm->getProperty(propertyName));
}

WorkspaceGroup_sptr getOutputResult(IAlgorithm_sptr algorithm) {
  return getWorkspaceOutput<WorkspaceGroup>(algorithm, "OutputWorkspace");
}

ITableWorkspace_sptr getOutputParameters(IAlgorithm_sptr algorithm) {
  return getWorkspaceOutput<ITableWorkspace>(algorithm,
                                             "OutputParameterWorkspace");
}

WorkspaceGroup_sptr getOutputGroup(IAlgorithm_sptr algorithm) {
  return getWorkspaceOutput<WorkspaceGroup>(algorithm, "OutputWorkspaceGroup");
}

void addFitProperties(Mantid::API::IAlgorithm &algorithm,
                      Mantid::API::IFunction_sptr function,
                      std::string const &xAxisUnit) {
  algorithm.setProperty("Function", function);
  algorithm.setProperty("ResultXAxisUnit", xAxisUnit);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

PrivateFittingDataLegacy::PrivateFittingDataLegacy() : m_data() {}

PrivateFittingDataLegacy::PrivateFittingDataLegacy(
    PrivateFittingDataLegacy &&privateData)
    : m_data(std::move(privateData.m_data)) {}

PrivateFittingDataLegacy::PrivateFittingDataLegacy(
    std::vector<std::unique_ptr<IndirectFitDataLegacy>> &&data)
    : m_data(std::move(data)) {}

PrivateFittingDataLegacy &PrivateFittingDataLegacy::
operator=(PrivateFittingDataLegacy &&fittingData) {
  m_data = std::move(fittingData.m_data);
  return *this;
}

IndirectFittingModelLegacy::IndirectFittingModelLegacy()
    : m_previousModelSelected(false),
      m_fittingMode(FittingModeLegacy::SEQUENTIAL) {}

bool IndirectFittingModelLegacy::hasWorkspace(
    std::string const &workspaceName) const {
  auto const names = getWorkspaceNames();
  auto const iter = std::find(names.begin(), names.end(), workspaceName);
  return iter != names.end();
}

MatrixWorkspace_sptr
IndirectFittingModelLegacy::getWorkspace(std::size_t index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index]->workspace();
  return nullptr;
}

std::vector<std::string> IndirectFittingModelLegacy::getWorkspaceNames() const {
  std::vector<std::string> names;
  names.reserve(m_fittingData.size());
  for (auto i = 0u; i < m_fittingData.size(); ++i)
    names.emplace_back(m_fittingData[i]->workspace()->getName());
  return names;
}

SpectraLegacy IndirectFittingModelLegacy::getSpectra(std::size_t index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index]->spectra();
  return DiscontinuousSpectra<std::size_t>("");
}

std::pair<double, double>
IndirectFittingModelLegacy::getFittingRange(std::size_t dataIndex,
                                            std::size_t spectrum) const {
  if (dataIndex < m_fittingData.size() &&
      !m_fittingData[dataIndex]->zeroSpectra()) {
    if (FittingModeLegacy::SEQUENTIAL == m_fittingMode)
      return m_fittingData.front()->getRange(0);
    return m_fittingData[dataIndex]->getRange(spectrum);
  }
  return std::make_pair(0., 0.);
}

std::string
IndirectFittingModelLegacy::getExcludeRegion(std::size_t dataIndex,
                                             std::size_t spectrum) const {
  if (dataIndex < m_fittingData.size() &&
      !m_fittingData[dataIndex]->zeroSpectra()) {
    if (FittingModeLegacy::SEQUENTIAL == m_fittingMode)
      return m_fittingData.back()->getExcludeRegion(0);
    return m_fittingData[dataIndex]->getExcludeRegion(spectrum);
  }
  return "";
}

std::string
IndirectFittingModelLegacy::createDisplayName(const std::string &formatString,
                                              const std::string &rangeDelimiter,
                                              std::size_t dataIndex) const {
  if (m_fittingData.size() > dataIndex)
    return m_fittingData[dataIndex]->displayName(formatString, rangeDelimiter);
  else
    throw std::runtime_error("Cannot create a display name for a workspace: "
                             "the workspace index provided is too large.");
}

std::string
IndirectFittingModelLegacy::createOutputName(const std::string &formatString,
                                             const std::string &rangeDelimiter,
                                             std::size_t dataIndex) const {
  return createDisplayName(formatString, rangeDelimiter, dataIndex) +
         "_Results";
}

bool IndirectFittingModelLegacy::isMultiFit() const {
  return numberOfWorkspaces() > 1;
}

bool IndirectFittingModelLegacy::isPreviouslyFit(std::size_t dataIndex,
                                                 std::size_t spectrum) const {
  if (!m_previousModelSelected || !m_fitOutput ||
      m_fittingData.size() <= dataIndex)
    return false;
  const auto fitData = m_fittingData[dataIndex].get();
  return m_fitOutput->isSpectrumFit(fitData, spectrum);
}

bool IndirectFittingModelLegacy::hasZeroSpectra(std::size_t dataIndex) const {
  if (m_fittingData.size() > dataIndex)
    return m_fittingData[dataIndex]->zeroSpectra();
  return true;
}

boost::optional<std::string>
IndirectFittingModelLegacy::isInvalidFunction() const {
  if (!m_activeFunction)
    return std::string("No fit function has been defined");

  const auto composite =
      boost::dynamic_pointer_cast<CompositeFunction>(m_activeFunction);
  if (composite && (composite->nFunctions() == 0 || composite->nParams() == 0))
    return std::string("No fitting functions have been defined.");
  return boost::none;
}

std::size_t IndirectFittingModelLegacy::numberOfWorkspaces() const {
  return m_fittingData.size();
}

std::size_t
IndirectFittingModelLegacy::getNumberOfSpectra(std::size_t index) const {
  if (index < m_fittingData.size())
    return m_fittingData[index]->numberOfSpectra();
  else
    throw std::runtime_error(
        "Cannot find the number of spectra for a workspace: the workspace "
        "index provided is too large.");
}

std::vector<std::string>
IndirectFittingModelLegacy::getFitParameterNames() const {
  if (m_fitOutput)
    return m_fitOutput->getResultParameterNames();
  return std::vector<std::string>();
}

Mantid::API::IFunction_sptr
IndirectFittingModelLegacy::getFittingFunction() const {
  return m_activeFunction;
}

void IndirectFittingModelLegacy::setFittingData(
    PrivateFittingDataLegacy &&fittingData) {
  m_fittingData = std::move(fittingData.m_data);
}

void IndirectFittingModelLegacy::setSpectra(const std::string &spectra,
                                            std::size_t dataIndex) {
  setSpectra(DiscontinuousSpectra<std::size_t>(spectra), dataIndex);
}

void IndirectFittingModelLegacy::setSpectra(SpectraLegacy &&spectra,
                                            std::size_t dataIndex) {
  m_fittingData[dataIndex]->setSpectra(std::forward<SpectraLegacy>(spectra));
}

void IndirectFittingModelLegacy::setSpectra(const SpectraLegacy &spectra,
                                            std::size_t dataIndex) {
  m_fittingData[dataIndex]->setSpectra(spectra);
}

void IndirectFittingModelLegacy::setStartX(double startX, std::size_t dataIndex,
                                           std::size_t spectrum) {
  if (FittingModeLegacy::SEQUENTIAL == m_fittingMode)
    m_fittingData.front()->setStartX(startX, 0);
  else
    m_fittingData[dataIndex]->setStartX(startX, spectrum);
}

void IndirectFittingModelLegacy::setEndX(double endX, std::size_t dataIndex,
                                         std::size_t spectrum) {
  if (FittingModeLegacy::SEQUENTIAL == m_fittingMode)
    m_fittingData.front()->setEndX(endX, 0);
  else
    m_fittingData[dataIndex]->setEndX(endX, spectrum);
}

void IndirectFittingModelLegacy::setExcludeRegion(const std::string &exclude,
                                                  std::size_t dataIndex,
                                                  std::size_t spectrum) {
  if (FittingModeLegacy::SEQUENTIAL == m_fittingMode)
    m_fittingData.front()->setExcludeRegionString(exclude, 0);
  else
    m_fittingData[dataIndex]->setExcludeRegionString(exclude, spectrum);
}

void IndirectFittingModelLegacy::addWorkspace(
    const std::string &workspaceName) {
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
  addWorkspace(ws, std::make_pair(0u, ws->getNumberHistograms() - 1));
}

void IndirectFittingModelLegacy::addWorkspace(const std::string &workspaceName,
                                              const std::string &spectra) {
  if (spectra.empty())
    throw std::runtime_error(
        "Fitting Data must consist of one or more spectra.");
  if (workspaceName.empty() || !doesExistInADS(workspaceName))
    throw std::runtime_error("A valid sample file needs to be selected.");

  addWorkspace(workspaceName, DiscontinuousSpectra<std::size_t>(spectra));
}

void IndirectFittingModelLegacy::addWorkspace(const std::string &workspaceName,
                                              const SpectraLegacy &spectra) {
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
  addWorkspace(ws, spectra);
}

void IndirectFittingModelLegacy::addWorkspace(MatrixWorkspace_sptr workspace,
                                              const SpectraLegacy &spectra) {
  if (!m_fittingData.empty() &&
      equivalentWorkspaces(workspace, m_fittingData.back()->workspace()))
    m_fittingData.back()->combine(IndirectFitDataLegacy(workspace, spectra));
  else
    addNewWorkspace(workspace, spectra);
}

void IndirectFittingModelLegacy::addNewWorkspace(MatrixWorkspace_sptr workspace,
                                                 const SpectraLegacy &spectra) {
  m_fittingData.emplace_back(new IndirectFitDataLegacy(workspace, spectra));
  m_defaultParameters.emplace_back(
      createDefaultParameters(m_fittingData.size() - 1));
}

void IndirectFittingModelLegacy::removeWorkspaceFromFittingData(
    std::size_t const &index) {
  if (m_fittingData.size() > index)
    removeFittingData(index);
  else
    throw std::runtime_error("Cannot remove a workspace from the fitting data: "
                             "the workspace index provided is too large.");
}

void IndirectFittingModelLegacy::removeWorkspace(std::size_t index) {
  removeWorkspaceFromFittingData(index);

  if (index > 0 && m_fittingData.size() > index) {
    const auto previousWS = m_fittingData[index - 1]->workspace();
    const auto subsequentWS = m_fittingData[index]->workspace();

    if (equivalentWorkspaces(previousWS, subsequentWS)) {
      m_fittingData[index - 1]->combine(*m_fittingData[index]);
      m_fittingData.erase(m_fittingData.begin() + index);
    }
  }
}

void IndirectFittingModelLegacy::removeFittingData(std::size_t index) {
  if (m_fitOutput)
    m_fitOutput->removeOutput(m_fittingData[index].get());
  m_fittingData.erase(m_fittingData.begin() + index);
  if (m_defaultParameters.size() > index)
    m_defaultParameters.erase(m_defaultParameters.begin() + index);
}

PrivateFittingDataLegacy IndirectFittingModelLegacy::clearWorkspaces() {
  m_fitOutput.reset();
  return std::move(m_fittingData);
}

void IndirectFittingModelLegacy::setFittingMode(FittingModeLegacy mode) {
  m_fittingMode = mode;
}

void IndirectFittingModelLegacy::setFitFunction(IFunction_sptr function) {
  m_activeFunction = function;
  m_previousModelSelected = isPreviousModelSelected();
}

void IndirectFittingModelLegacy::setDefaultParameterValue(
    const std::string &name, double value, std::size_t dataIndex) {
  if (m_defaultParameters.size() > dataIndex)
    m_defaultParameters[dataIndex][name] = ParameterValueLegacy(value);
}

void IndirectFittingModelLegacy::addOutput(IAlgorithm_sptr fitAlgorithm) {
  addOutput(fitAlgorithm, m_fittingData.begin(), m_fittingData.end());
}

void IndirectFittingModelLegacy::addOutput(IAlgorithm_sptr fitAlgorithm,
                                           const FitDataIteratorLegacy &fitDataBegin,
                                           const FitDataIteratorLegacy &fitDataEnd) {
  auto group = getOutputGroup(fitAlgorithm);
  auto parameters = getOutputParameters(fitAlgorithm);
  auto result = getOutputResult(fitAlgorithm);
  m_fitFunction =
      extractFirstInnerFunction(fitAlgorithm->getPropertyValue("Function"));
  addOutput(group, parameters, result, fitDataBegin, fitDataEnd);
}

void IndirectFittingModelLegacy::addSingleFitOutput(
    IAlgorithm_sptr fitAlgorithm, std::size_t index) {
  auto group = getOutputGroup(fitAlgorithm);
  auto parameters = getOutputParameters(fitAlgorithm);
  auto result = getOutputResult(fitAlgorithm);
  int spectrum = fitAlgorithm->getProperty("WorkspaceIndex");
  m_fitFunction = FunctionFactory::Instance().createInitialized(
      fitAlgorithm->getPropertyValue("Function"));
  addOutput(group, parameters, result, m_fittingData[index].get(),
            boost::numeric_cast<std::size_t>(spectrum));
}

void IndirectFittingModelLegacy::addOutput(WorkspaceGroup_sptr resultGroup,
                                           ITableWorkspace_sptr parameterTable,
                                           WorkspaceGroup_sptr resultWorkspace,
                                           const FitDataIteratorLegacy &fitDataBegin,
                                           const FitDataIteratorLegacy &fitDataEnd) {
  if (m_previousModelSelected && m_fitOutput)
    addOutput(m_fitOutput.get(), resultGroup, parameterTable, resultWorkspace,
              fitDataBegin, fitDataEnd);
  else
    m_fitOutput = std::make_unique<IndirectFitOutputLegacy>(
        createFitOutput(resultGroup, parameterTable, resultWorkspace,
                        fitDataBegin, fitDataEnd));
  m_previousModelSelected = isPreviousModelSelected();
}

void IndirectFittingModelLegacy::addOutput(WorkspaceGroup_sptr resultGroup,
                                           ITableWorkspace_sptr parameterTable,
                                           WorkspaceGroup_sptr resultWorkspace,
                                           IndirectFitDataLegacy *fitData,
                                           std::size_t spectrum) {
  if (m_previousModelSelected && m_fitOutput)
    addOutput(m_fitOutput.get(), resultGroup, parameterTable, resultWorkspace,
              fitData, spectrum);
  else
    m_fitOutput = std::make_unique<IndirectFitOutputLegacy>(createFitOutput(
        resultGroup, parameterTable, resultWorkspace, fitData, spectrum));
  m_previousModelSelected = isPreviousModelSelected();
}

IndirectFitOutputLegacy IndirectFittingModelLegacy::createFitOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    WorkspaceGroup_sptr resultWorkspace, const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd) const {
  return IndirectFitOutputLegacy(resultGroup, parameterTable, resultWorkspace,
                                 fitDataBegin, fitDataEnd);
}

IndirectFitOutputLegacy IndirectFittingModelLegacy::createFitOutput(
    Mantid::API::WorkspaceGroup_sptr resultGroup,
    Mantid::API::ITableWorkspace_sptr parameterTable,
    Mantid::API::WorkspaceGroup_sptr resultWorkspace,
    IndirectFitDataLegacy *fitData, std::size_t spectrum) const {
  return IndirectFitOutputLegacy(resultGroup, parameterTable, resultWorkspace,
                                 fitData, spectrum);
}

void IndirectFittingModelLegacy::addOutput(
    IndirectFitOutputLegacy *fitOutput, WorkspaceGroup_sptr resultGroup,
    ITableWorkspace_sptr parameterTable, WorkspaceGroup_sptr resultWorkspace,
    const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd) const {
  fitOutput->addOutput(resultGroup, parameterTable, resultWorkspace,
                       fitDataBegin, fitDataEnd);
}

void IndirectFittingModelLegacy::addOutput(
    IndirectFitOutputLegacy *fitOutput,
    Mantid::API::WorkspaceGroup_sptr resultGroup,
    Mantid::API::ITableWorkspace_sptr parameterTable,
    Mantid::API::WorkspaceGroup_sptr resultWorkspace,
    IndirectFitDataLegacy *fitData, std::size_t spectrum) const {
  fitOutput->addOutput(resultGroup, parameterTable, resultWorkspace, fitData,
                       spectrum);
}

FittingModeLegacy IndirectFittingModelLegacy::getFittingMode() const {
  return m_fittingMode;
}

std::unordered_map<std::string, ParameterValueLegacy>
IndirectFittingModelLegacy::getParameterValues(std::size_t index,
                                               std::size_t spectrum) const {
  if (m_fittingData.size() > index) {
    const auto parameters = getFitParameters(index, spectrum);
    if (m_previousModelSelected)
      return parameters;
    else if (parameters.empty())
      return getDefaultParameters(index);
    return combine(getDefaultParameters(index), parameters);
  }
  return std::unordered_map<std::string, ParameterValueLegacy>();
}

std::unordered_map<std::string, ParameterValueLegacy>
IndirectFittingModelLegacy::getFitParameters(std::size_t index,
                                             std::size_t spectrum) const {
  if (m_fitOutput)
    return m_fitOutput->getParameters(m_fittingData[index].get(), spectrum);
  return std::unordered_map<std::string, ParameterValueLegacy>();
}

std::unordered_map<std::string, ParameterValueLegacy>
IndirectFittingModelLegacy::getDefaultParameters(std::size_t index) const {
  if (index < m_defaultParameters.size())
    return mapKeys(m_defaultParameters[index], mapDefaultParameterNames());
  return std::unordered_map<std::string, ParameterValueLegacy>();
}

std::unordered_map<std::string, std::string>
IndirectFittingModelLegacy::mapDefaultParameterNames() const {
  if (m_activeFunction)
    return shortToLongParameterNames(getFittingFunction());
  return std::unordered_map<std::string, std::string>();
}

std::unordered_map<std::string, ParameterValueLegacy>
IndirectFittingModelLegacy::createDefaultParameters(
    std::size_t /*unused*/) const {
  return std::unordered_map<std::string, ParameterValueLegacy>();
}

std::string IndirectFittingModelLegacy::getResultXAxisUnit() const {
  return "MomentumTransfer";
}

std::string IndirectFittingModelLegacy::getResultLogName() const {
  return "axis-1";
}

boost::optional<ResultLocation>
IndirectFittingModelLegacy::getResultLocation(std::size_t index,
                                              std::size_t spectrum) const {
  if (m_previousModelSelected && m_fitOutput && m_fittingData.size() > index)
    return m_fitOutput->getResultLocation(m_fittingData[index].get(), spectrum);
  return boost::none;
}

WorkspaceGroup_sptr IndirectFittingModelLegacy::getResultWorkspace() const {
  return m_fitOutput->getLastResultWorkspace();
}

WorkspaceGroup_sptr IndirectFittingModelLegacy::getResultGroup() const {
  return m_fitOutput->getLastResultGroup();
}

bool IndirectFittingModelLegacy::isPreviousModelSelected() const {
  return m_fitFunction &&
         equivalentFunctions(getFittingFunction(), m_fitFunction);
}

CompositeFunction_sptr
IndirectFittingModelLegacy::getMultiDomainFunction() const {
  return createMultiDomainFunction(getFittingFunction(), numberOfWorkspaces());
}

IAlgorithm_sptr IndirectFittingModelLegacy::getFittingAlgorithm() const {
  return getFittingAlgorithm(m_fittingMode);
}

IAlgorithm_sptr
IndirectFittingModelLegacy::getFittingAlgorithm(FittingModeLegacy mode) const {
  if (mode == FittingModeLegacy::SEQUENTIAL)
    return createSequentialFit(getFittingFunction());
  else
    return createSimultaneousFit(getMultiDomainFunction());
}

IAlgorithm_sptr
IndirectFittingModelLegacy::getSingleFit(std::size_t dataIndex,
                                         std::size_t spectrum) const {
  const auto &fitData = m_fittingData[dataIndex];
  const auto ws = fitData->workspace();
  const auto range = fitData->getRange(spectrum);
  const auto exclude = fitData->excludeRegionsVector(spectrum);

  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, getFittingFunction(), getResultXAxisUnit());
  addInputDataToSimultaneousFit(fitAlgorithm, ws, spectrum, range, exclude, "");
  fitAlgorithm->setProperty("OutputWorkspace",
                            singleFitOutputName(dataIndex, spectrum));
  return fitAlgorithm;
}

Mantid::API::IAlgorithm_sptr
IndirectFittingModelLegacy::sequentialFitAlgorithm() const {
  return AlgorithmManager::Instance().create("QENSFitSequential");
}

Mantid::API::IAlgorithm_sptr
IndirectFittingModelLegacy::simultaneousFitAlgorithm() const {
  return AlgorithmManager::Instance().create("QENSFitSimultaneous");
}

IAlgorithm_sptr
IndirectFittingModelLegacy::createSequentialFit(IFunction_sptr function) const {
  const auto input = constructInputString(m_fittingData);
  return createSequentialFit(function, input, m_fittingData.front().get());
}

IAlgorithm_sptr IndirectFittingModelLegacy::createSequentialFit(
    IFunction_sptr function, const std::string &input,
    IndirectFitDataLegacy *initialFitData) const {
  auto fitAlgorithm = sequentialFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());
  fitAlgorithm->setProperty("Input", input);
  fitAlgorithm->setProperty("OutputWorkspace", sequentialFitOutputName());
  fitAlgorithm->setProperty("PassWSIndexToFunction", true);
  fitAlgorithm->setProperty("LogName", getResultLogName());

  const auto range = initialFitData->getRange(0);
  fitAlgorithm->setProperty("StartX", range.first);
  fitAlgorithm->setProperty("EndX", range.second);

  auto excludeRegion = initialFitData->excludeRegionsVector(0);
  if (!excludeRegion.empty())
    fitAlgorithm->setProperty("Exclude", excludeRegion);

  return fitAlgorithm;
}

IAlgorithm_sptr IndirectFittingModelLegacy::createSimultaneousFit(
    IFunction_sptr function) const {
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());
  addInputDataToSimultaneousFit(fitAlgorithm, m_fittingData);
  fitAlgorithm->setProperty("OutputWorkspace", simultaneousFitOutputName());
  return fitAlgorithm;
}

IAlgorithm_sptr IndirectFittingModelLegacy::createSimultaneousFitWithEqualRange(
    IFunction_sptr function) const {
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());

  auto exclude = vectorFromStringLegacy<double>(getExcludeRegion(0, 0));
  addInputDataToSimultaneousFit(fitAlgorithm, m_fittingData,
                                getFittingRange(0, 0), exclude);
  fitAlgorithm->setProperty("OutputWorkspace", simultaneousFitOutputName());
  return fitAlgorithm;
}

std::string IndirectFittingModelLegacy::createSingleFitOutputName(
    const std::string &formatString, std::size_t index,
    std::size_t spectrum) const {
  if (m_fittingData.size() > index)
    return m_fittingData[index]->displayName(formatString, spectrum);
  else
    throw std::runtime_error("Cannot create a display name for a workspace: "
                             "the workspace index provided is too large.");
}

std::string IndirectFittingModelLegacy::getOutputBasename() const {
  return cutLastOf(sequentialFitOutputName(), "_Results");
}

void IndirectFittingModelLegacy::cleanFailedRun(
    IAlgorithm_sptr fittingAlgorithm) {
  cleanTemporaries(fittingAlgorithm->name(), m_fittingData);
}

void IndirectFittingModelLegacy::cleanFailedSingleRun(
    IAlgorithm_sptr fittingAlgorithm, std::size_t index) {
  const auto base =
      "__" + fittingAlgorithm->name() + "_ws" + std::to_string(index + 1);
  removeFromADSIfExists(base);
  cleanTemporaries(base + "_0");
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
