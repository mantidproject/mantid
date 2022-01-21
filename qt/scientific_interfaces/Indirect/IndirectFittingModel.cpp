// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFittingModel.h"
#include "IndirectFitDataModel.h"
#include "IndirectFitOutput.h"

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
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

std::string cutLastOf(std::string const &str, std::string const &delimiter) {
  auto const cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}

/**
 * @return  True if the first function precedes the second when ordering by
 *          name.
 */
bool functionNameComparator(const IFunction_const_sptr &first, const IFunction_const_sptr &second) {
  return first->name() < second->name();
}

/**
 * Extracts the functions from a composite function into a vector.
 *
 * @param composite The composite function.
 * @return          A vector of the functions in the specified composite
 *                  function.
 */
std::vector<IFunction_const_sptr> extractFunctions(const CompositeFunction &composite) {
  std::vector<IFunction_const_sptr> functions;
  functions.reserve(composite.nFunctions());

  for (auto i = 0u; i < composite.nFunctions(); ++i)
    functions.emplace_back(composite.getFunction(i));
  return functions;
}

bool equivalentFunctions(const IFunction_const_sptr &func1, const IFunction_const_sptr &func2);

/*
 * Checks whether the specified composite functions have the same composition.
 *
 * @param composite1 Function to compare.
 * @param composite2 Function to compare.
 * @return           True if the specified functions have the same composition,
 *                   False otherwise.
 */
bool equivalentComposites(const CompositeFunction &composite1, const CompositeFunction &composite2) {

  if (composite1.nFunctions() != composite2.nFunctions() || composite1.nParams() != composite2.nParams()) {
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
bool equivalentFunctions(const IFunction_const_sptr &func1, const IFunction_const_sptr &func2) {
  const auto composite1 = std::dynamic_pointer_cast<const CompositeFunction>(func1);
  const auto composite2 = std::dynamic_pointer_cast<const CompositeFunction>(func2);

  if (composite1 && composite2)
    return equivalentComposites(*composite1, *composite2);
  else if (func1 && func2 && !composite1 && !composite2)
    return func1->name() == func2->name();
  return false;
}

void addInputDataToSimultaneousFit(const IAlgorithm_sptr &fitAlgorithm, const MatrixWorkspace_sptr &workspace,
                                   size_t spectrum, const std::pair<double, double> &xRange,
                                   const std::vector<double> &excludeRegions, const std::string &suffix) {
  fitAlgorithm->setProperty("InputWorkspace" + suffix, workspace);
  fitAlgorithm->setProperty("StartX" + suffix, xRange.first);
  fitAlgorithm->setProperty("EndX" + suffix, xRange.second);
  fitAlgorithm->setProperty("WorkspaceIndex" + suffix, static_cast<int>(spectrum));

  if (!excludeRegions.empty())
    fitAlgorithm->setProperty("Exclude" + suffix, excludeRegions);
}

void addInputDataToSimultaneousFit(const IAlgorithm_sptr &fitAlgorithm, const IIndirectFitDataModel *fittingData) {
  for (auto index = FitDomainIndex{0}; index < FitDomainIndex{fittingData->getNumberOfDomains()}; index++) {
    std::string suffix = index == FitDomainIndex{0} ? "" : "_" + std::to_string(index.value);
    addInputDataToSimultaneousFit(fitAlgorithm, fittingData->getWorkspace(index), fittingData->getSpectrum(index),
                                  fittingData->getFittingRange(index), fittingData->getExcludeRegionVector(index),
                                  suffix);
  }
}

template <typename Map> Map combine(const Map &mapA, const Map &mapB) {
  Map newMap(mapA);
  newMap.insert(std::begin(mapB), std::end(mapB));
  return newMap;
}

std::unordered_map<std::string, std::string> shortToLongParameterNames(const IFunction_sptr &function) {
  std::unordered_map<std::string, std::string> shortToLong;
  for (const auto &name : function->getParameterNames()) {
    auto shortName = name.substr(name.rfind(".") + 1);
    if (shortToLong.find(shortName) != shortToLong.end()) {
      shortToLong[shortName] += "," + name;
    } else {
      shortToLong[shortName] = name;
    }
  }
  return shortToLong;
}

template <typename Map, typename KeyMap> Map mapKeys(const Map &map, const KeyMap &mapping) {
  Map mapped;
  for (const auto &value : map) {
    auto it = mapping.find(value.first);
    if (it != mapping.end()) {
      std::stringstream paramNames(it->second);
      std::string paramName;
      while (std::getline(paramNames, paramName, ',')) {
        mapped[paramName] = value.second;
      }
    }
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

std::ostringstream &addInputString(const std::string &workspaceName, size_t workspaceIndex,
                                   std::ostringstream &stream) {
  if (!workspaceName.empty()) {
    stream << workspaceName << ",i" << workspaceIndex << ";";
    return stream;
  } else
    throw std::runtime_error("Workspace name is empty. The sample workspace may not be loaded.");
}

std::string constructInputString(const IIndirectFitDataModel *fittingData) {
  std::ostringstream input;
  for (auto index = FitDomainIndex{0}; index < fittingData->getNumberOfDomains(); index++) {
    addInputString(fittingData->getWorkspace(index)->getName(), fittingData->getSpectrum(index), input);
  }
  return input.str();
}

IFunction_sptr extractFirstInnerFunction(IFunction_sptr function) {
  if (const auto multiDomain = std::dynamic_pointer_cast<MultiDomainFunction>(function)) {
    if (multiDomain->nFunctions() > 0)
      return multiDomain->getFunction(0);
  }
  return function;
}

IFunction_sptr extractFirstInnerFunction(const std::string &function) {
  return extractFirstInnerFunction(FunctionFactory::Instance().createInitialized(function));
}

template <typename WorkspaceType>
std::shared_ptr<WorkspaceType> getWorkspaceOutput(const IAlgorithm_sptr &algorithm, const std::string &propertyName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceType>(algorithm->getProperty(propertyName));
}

WorkspaceGroup_sptr getOutputResult(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<WorkspaceGroup>(algorithm, "OutputWorkspace");
}

ITableWorkspace_sptr getOutputParameters(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<ITableWorkspace>(algorithm, "OutputParameterWorkspace");
}

WorkspaceGroup_sptr getOutputGroup(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<WorkspaceGroup>(algorithm, "OutputWorkspaceGroup");
}

void addFitProperties(Mantid::API::IAlgorithm &algorithm, const Mantid::API::IFunction_sptr &function,
                      std::string const &xAxisUnit) {
  algorithm.setProperty("Function", function);
  algorithm.setProperty("ResultXAxisUnit", xAxisUnit);
}

IFunction_sptr firstFunctionWithParameter(IFunction_sptr function, const std::string &category,
                                          const std::string &parameterName);

IFunction_sptr firstFunctionWithParameter(const CompositeFunction_sptr &composite, const std::string &category,
                                          const std::string &parameterName) {
  for (auto i = 0u; i < composite->nFunctions(); ++i) {
    const auto value = firstFunctionWithParameter(composite->getFunction(i), category, parameterName);
    if (value)
      return value;
  }
  return nullptr;
}

IFunction_sptr firstFunctionWithParameter(IFunction_sptr function, const std::string &category,
                                          const std::string &parameterName) {
  if (function->category() == category && function->hasParameter(parameterName))
    return function;

  const auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    return firstFunctionWithParameter(composite, category, parameterName);
  return nullptr;
}

void setFunctionParameters(const IFunction_sptr &function, const std::string &category,
                           const std::string &parameterName, double value);

void setFunctionParameters(const CompositeFunction_sptr &composite, const std::string &category,
                           const std::string &parameterName, double value) {
  for (auto i = 0u; i < composite->nFunctions(); ++i)
    setFunctionParameters(composite->getFunction(i), category, parameterName, value);
}

void setFunctionParameters(const IFunction_sptr &function, const std::string &category,
                           const std::string &parameterName, double value) {
  if (function->category() == category && function->hasParameter(parameterName))
    function->setParameter(parameterName, value);

  auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    setFunctionParameters(composite, category, parameterName, value);
}

void setFunctionParameters(const MultiDomainFunction_sptr &function, const std::string &category,
                           const std::string &parameterName, double value) {
  for (size_t i = 0u; i < function->nFunctions(); ++i)
    setFunctionParameters(function->getFunction(i), category, parameterName, value);
}

void setFirstBackground(IFunction_sptr function, double value) {
  firstFunctionWithParameter(std::move(function), "Background", "A0")->setParameter("A0", value);
}

} // namespace

namespace MantidQt::CustomInterfaces::IDA {

std::unordered_map<FittingMode, std::string> fitModeToName = std::unordered_map<FittingMode, std::string>(
    {{FittingMode::SEQUENTIAL, "Seq"}, {FittingMode::SIMULTANEOUS, "Sim"}});

IndirectFittingModel::IndirectFittingModel()
    : m_fitDataModel(std::make_unique<IndirectFitDataModel>()), m_previousModelSelected(false),
      m_fittingMode(FittingMode::SEQUENTIAL), m_fitOutput(std::make_unique<IndirectFitOutput>()) {}

// Functions that interact with IndirectFitDataModel

void IndirectFittingModel::addDefaultParameters() {
  m_defaultParameters.emplace_back(createDefaultParameters(WorkspaceID{0}));
}

void IndirectFittingModel::removeDefaultParameters() {
  if (m_fitDataModel->getNumberOfWorkspaces() < m_defaultParameters.size()) {
    m_defaultParameters.remove(WorkspaceID{0});
  }
}

void IndirectFittingModel::clearWorkspaces() {
  m_fitOutput->clear();
  m_fitDataModel->clear();
}

MatrixWorkspace_sptr IndirectFittingModel::getWorkspace(WorkspaceID workspaceID) const {
  return m_fitDataModel->getWorkspace(workspaceID);
}

WorkspaceID IndirectFittingModel::getNumberOfWorkspaces() const { return m_fitDataModel->getNumberOfWorkspaces(); }

bool IndirectFittingModel::isMultiFit() const { return m_fitDataModel->getNumberOfWorkspaces().value > 1; }

// Other Functions
void IndirectFittingModel::setFitTypeString(const std::string &fitType) { m_fitString = fitType; }

std::string IndirectFittingModel::createOutputName(const std::string &fitMode, const std::string &workspaceName,
                                                   const std::string &spectra) const {
  std::string inputWorkspace = isMultiFit() ? "Multi" : workspaceName;
  std::string inputSpectra = isMultiFit() ? "" : spectra;
  return inputWorkspace + "_" + m_fitType + "_" + fitMode + "_" + m_fitString + "_" + inputSpectra + "_Results";
}

std::string IndirectFittingModel::sequentialFitOutputName() const {
  return createOutputName(SEQ_STRING, m_fitDataModel->getWorkspaceNames()[0],
                          m_fitDataModel->getSpectra(WorkspaceID{0}).getString());
}

std::string IndirectFittingModel::simultaneousFitOutputName() const {
  return createOutputName(SIM_STRING, m_fitDataModel->getWorkspaceNames()[0],
                          m_fitDataModel->getSpectra(WorkspaceID{0}).getString());
}

bool IndirectFittingModel::isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  auto domainIndex = m_fitDataModel->getDomainIndex(workspaceID, spectrum);
  return m_fitOutput->isSpectrumFit(domainIndex);
}

boost::optional<std::string> IndirectFittingModel::isInvalidFunction() const {
  if (!m_activeFunction)
    return std::string("No fit function has been defined");

  const auto composite = std::dynamic_pointer_cast<CompositeFunction>(m_activeFunction);
  if (composite && (composite->nFunctions() == 0 || composite->nParams() == 0))
    return std::string("No fitting functions have been defined.");
  return boost::none;
}

std::vector<std::string> IndirectFittingModel::getFitParameterNames() const {
  if (!m_fitOutput->isEmpty())
    return m_fitOutput->getResultParameterNames();
  return std::vector<std::string>();
}

IIndirectFitOutput *IndirectFittingModel::getFitOutput() const { return m_fitOutput.get(); }

Mantid::API::MultiDomainFunction_sptr IndirectFittingModel::getFitFunction() const { return m_activeFunction; }

void IndirectFittingModel::removeFittingData() { m_fitOutput->clear(); }

void IndirectFittingModel::setFittingMode(FittingMode mode) { m_fittingMode = mode; }

void IndirectFittingModel::setFitFunction(MultiDomainFunction_sptr function) {
  m_activeFunction = std::move(function);
  m_previousModelSelected = isPreviousModelSelected();
}

void IndirectFittingModel::setFWHM(double fwhm, WorkspaceID WorkspaceID) {
  setDefaultParameterValue("FWHM", fwhm, WorkspaceID);
  setFunctionParameters(getFitFunction(), "Peak", "FWHM", fwhm);
}

void IndirectFittingModel::setBackground(double background, WorkspaceID WorkspaceID) {
  setDefaultParameterValue("A0", background, WorkspaceID);
  setFirstBackground(getFitFunction(), background);
}

void IndirectFittingModel::setDefaultParameterValue(const std::string &name, double value, WorkspaceID workspaceID) {
  if (m_defaultParameters.size() > workspaceID)
    m_defaultParameters[workspaceID][name] = ParameterValue(value);
}

void IndirectFittingModel::addOutput(IAlgorithm_sptr fitAlgorithm) {
  auto group = getOutputGroup(fitAlgorithm);
  auto parameters = getOutputParameters(fitAlgorithm);
  auto result = getOutputResult(fitAlgorithm);
  m_fitFunction = extractFirstInnerFunction(fitAlgorithm->getPropertyValue("Function"));
  m_fitOutput->addOutput(group, parameters, result);
  m_previousModelSelected = isPreviousModelSelected();
}

void IndirectFittingModel::addSingleFitOutput(const IAlgorithm_sptr &fitAlgorithm, WorkspaceID workspaceID,
                                              WorkspaceIndex spectrum) {
  auto group = getOutputGroup(fitAlgorithm);
  auto parameters = getOutputParameters(fitAlgorithm);
  auto result = getOutputResult(fitAlgorithm);
  m_fitFunction = FunctionFactory::Instance().createInitialized(fitAlgorithm->getPropertyValue("Function"));
  auto fitDomainIndex = m_fitDataModel->getDomainIndex(workspaceID, spectrum);
  m_fitOutput->addSingleOutput(group, parameters, result, fitDomainIndex);
  m_previousModelSelected = isPreviousModelSelected();
}

FittingMode IndirectFittingModel::getFittingMode() const { return m_fittingMode; }

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getParameterValues(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  if (m_fitDataModel->getNumberOfWorkspaces() > workspaceID) {
    const auto parameters = getFitParameters(workspaceID, spectrum);
    if (m_previousModelSelected)
      return parameters;
    else if (parameters.empty())
      return getDefaultParameters(workspaceID);
    return combine(getDefaultParameters(workspaceID), parameters);
  }
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, ParameterValue> IndirectFittingModel::getFitParameters(WorkspaceID workspaceID,
                                                                                       WorkspaceIndex spectrum) const {
  auto fitDomainIndex = m_fitDataModel->getDomainIndex(workspaceID, spectrum);
  if (!m_fitOutput->isEmpty())
    return m_fitOutput->getParameters(fitDomainIndex);
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getDefaultParameters(WorkspaceID workspaceID) const {
  if (workspaceID < m_defaultParameters.size())
    return mapKeys(m_defaultParameters[workspaceID], mapDefaultParameterNames());
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, std::string> IndirectFittingModel::mapDefaultParameterNames() const {
  if (m_activeFunction)
    return shortToLongParameterNames(getFitFunction());
  return std::unordered_map<std::string, std::string>();
}

std::unordered_map<std::string, ParameterValue> IndirectFittingModel::createDefaultParameters(WorkspaceID) const {
  return std::unordered_map<std::string, ParameterValue>();
}

std::string IndirectFittingModel::getResultXAxisUnit() const { return "MomentumTransfer"; }

std::string IndirectFittingModel::getResultLogName() const { return "axis-1"; }

boost::optional<ResultLocationNew> IndirectFittingModel::getResultLocation(WorkspaceID workspaceID,
                                                                           WorkspaceIndex spectrum) const {
  auto fitDomainIndex = m_fitDataModel->getDomainIndex(workspaceID, spectrum);
  if (!m_fitOutput->isEmpty() && m_fitDataModel->getNumberOfWorkspaces() > workspaceID)
    return m_fitOutput->getResultLocation(fitDomainIndex);
  return boost::none;
}

WorkspaceGroup_sptr IndirectFittingModel::getResultWorkspace() const { return m_fitOutput->getLastResultWorkspace(); }

WorkspaceGroup_sptr IndirectFittingModel::getResultGroup() const { return m_fitOutput->getLastResultGroup(); }

bool IndirectFittingModel::isPreviousModelSelected() const {
  return m_fitFunction && equivalentFunctions(extractFirstInnerFunction(getFitFunction()), m_fitFunction);
}

MultiDomainFunction_sptr IndirectFittingModel::getMultiDomainFunction() const { return m_activeFunction; }

IAlgorithm_sptr IndirectFittingModel::getFittingAlgorithm(FittingMode mode) const {
  if (mode == FittingMode::SEQUENTIAL) {
    if (m_activeFunction->getNumberDomains() == 0) {
      throw std::runtime_error("Function is undefined");
    }
    return createSequentialFit(getFitFunction());
  } else
    return createSimultaneousFit(getFitFunction());
}

IAlgorithm_sptr IndirectFittingModel::getSingleFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  const auto ws = m_fitDataModel->getWorkspace(workspaceID);
  const auto range = m_fitDataModel->getFittingRange(workspaceID, spectrum);
  const auto exclude = m_fitDataModel->getExcludeRegionVector(workspaceID, spectrum);
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, getSingleFunction(workspaceID, spectrum), getResultXAxisUnit());
  addInputDataToSimultaneousFit(fitAlgorithm, ws, spectrum.value, range, exclude, std::string(""));
  fitAlgorithm->setProperty("OutputWorkspace",
                            singleFitOutputName(m_fitDataModel->getWorkspaceNames()[workspaceID.value], spectrum));
  return fitAlgorithm;
}

Mantid::API::IFunction_sptr IndirectFittingModel::getSingleFunction(WorkspaceID workspaceID,
                                                                    WorkspaceIndex spectrum) const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == m_fitDataModel->getNumberOfDomains());
  if (function->getNumberDomains() == 0) {
    throw std::runtime_error("Cannot set up a fit: is the function defined?");
  }
  return function->getFunction(m_fitDataModel->getDomainIndex(workspaceID, spectrum).value);
}

Mantid::API::IAlgorithm_sptr IndirectFittingModel::sequentialFitAlgorithm() const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == m_fitDataModel->getNumberOfDomains());
  return AlgorithmManager::Instance().create("QENSFitSequential");
}

Mantid::API::IAlgorithm_sptr IndirectFittingModel::simultaneousFitAlgorithm() const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == m_fitDataModel->getNumberOfDomains());
  return AlgorithmManager::Instance().create("QENSFitSimultaneous");
}

IAlgorithm_sptr IndirectFittingModel::createSequentialFit(const IFunction_sptr function) const {
  const auto input = constructInputString(m_fitDataModel.get());
  return createSequentialFit(function, input);
}

IAlgorithm_sptr IndirectFittingModel::createSequentialFit(const IFunction_sptr function,
                                                          const std::string &input) const {
  auto fitAlgorithm = sequentialFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());
  fitAlgorithm->setProperty("Input", input);
  fitAlgorithm->setProperty("OutputWorkspace", sequentialFitOutputName());
  fitAlgorithm->setProperty("LogName", getResultLogName());
  std::stringstream startX;
  std::stringstream endX;
  for (size_t i = 0; i < m_fitDataModel->getNumberOfDomains(); i++) {
    const auto range = m_fitDataModel->getFittingRange(FitDomainIndex(i));
    startX << range.first << ",";
    endX << range.second << ",";
  }
  fitAlgorithm->setProperty("StartX", startX.str());
  fitAlgorithm->setProperty("EndX", endX.str());

  std::vector<std::string> excludeRegions;
  excludeRegions.reserve(m_fitDataModel->getNumberOfDomains());
  for (size_t i = 0; i < m_fitDataModel->getNumberOfDomains(); i++) {
    if (!m_fitDataModel->getExcludeRegionVector(FitDomainIndex{i}).empty()) {
      excludeRegions.emplace_back(m_fitDataModel->getExcludeRegion(FitDomainIndex{i}));
    } else {
      excludeRegions.emplace_back("");
    }
  }
  fitAlgorithm->setProperty("ExcludeMultiple", excludeRegions);

  return fitAlgorithm;
}

IAlgorithm_sptr IndirectFittingModel::createSimultaneousFit(const MultiDomainFunction_sptr &function) const {
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());
  addInputDataToSimultaneousFit(fitAlgorithm, m_fitDataModel.get());
  fitAlgorithm->setProperty("OutputWorkspace", simultaneousFitOutputName());
  return fitAlgorithm;
}

std::string IndirectFittingModel::singleFitOutputName(std::string workspaceName, WorkspaceIndex spectrum) const {
  std::string inputWorkspace = isMultiFit() ? "Multi" : workspaceName;
  std::string spectra = std::to_string(spectrum.value);
  return inputWorkspace + "_" + m_fitType + "_" + m_fitString + "_" + spectra + "_Results";
}

std::string IndirectFittingModel::getOutputBasename() const { return cutLastOf(sequentialFitOutputName(), "_Results"); }

void IndirectFittingModel::cleanFailedRun(const IAlgorithm_sptr &fittingAlgorithm) {
  const auto prefix = "__" + fittingAlgorithm->name() + "_ws";
  for (WorkspaceID datasetIndex = 0; datasetIndex < m_fitDataModel->getNumberOfWorkspaces(); ++datasetIndex) {
    const auto base = prefix + std::to_string(datasetIndex.value + 1);
    removeFromADSIfExists(base);
    for (size_t index = 0; index < m_fitDataModel->getNumberOfSpectra(datasetIndex); index++) {
      cleanTemporaries(base + "_" + std::to_string(index));
    }
  }
}

void IndirectFittingModel::cleanFailedSingleRun(const IAlgorithm_sptr &fittingAlgorithm, WorkspaceID workspaceID) {
  const auto base = "__" + fittingAlgorithm->name() + "_ws" + std::to_string(workspaceID.value + 1);
  removeFromADSIfExists(base);
  cleanTemporaries(base + "_0");
}

IIndirectFitDataModel *IndirectFittingModel::getFitDataModel() { return m_fitDataModel.get(); }

} // namespace MantidQt::CustomInterfaces::IDA
