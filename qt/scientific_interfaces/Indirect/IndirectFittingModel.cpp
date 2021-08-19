// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFittingModel.h"
#include "IndirectFitDataTableModel.h"
#include "IndirectFitOutputModel.h"

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

std::string getFitDataName(const std::string &baseWorkspaceName, const FunctionModelSpectra &workspaceIndexes) {
  return baseWorkspaceName + " (" + workspaceIndexes.getString() + ")";
}

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

void addInputDataToSimultaneousFit(const IAlgorithm_sptr &fitAlgorithm, const IIndirectFitDataTableModel *fittingData) {
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

std::string constructInputString(const IIndirectFitDataTableModel *fittingData) {
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
  return getWorkspaceOutput<WorkspaceGroup>(std::move(algorithm), "OutputWorkspace");
}

ITableWorkspace_sptr getOutputParameters(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<ITableWorkspace>(std::move(algorithm), "OutputParameterWorkspace");
}

WorkspaceGroup_sptr getOutputGroup(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<WorkspaceGroup>(std::move(algorithm), "OutputWorkspaceGroup");
}

void addFitProperties(Mantid::API::IAlgorithm &algorithm, const Mantid::API::IFunction_sptr &function,
                      std::string const &xAxisUnit) {
  algorithm.setProperty("Function", function);
  algorithm.setProperty("ResultXAxisUnit", xAxisUnit);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

std::unordered_map<FittingMode, std::string> fitModeToName = std::unordered_map<FittingMode, std::string>(
    {{FittingMode::SEQUENTIAL, "Seq"}, {FittingMode::SIMULTANEOUS, "Sim"}});

IndirectFittingModel::IndirectFittingModel()
    : m_fitDataModel(std::make_unique<IndirectFitDataTableModel>()), m_previousModelSelected(false),
      m_fittingMode(FittingMode::SEQUENTIAL), m_fitOutput(std::make_unique<IndirectFitOutputModel>()) {}

bool IndirectFittingModel::hasWorkspace(std::string const &workspaceName) const {
  return m_fitDataModel->hasWorkspace(workspaceName);
}

MatrixWorkspace_sptr IndirectFittingModel::getWorkspace(WorkspaceID workspaceID) const {
  return m_fitDataModel->getWorkspace(workspaceID);
}

FunctionModelSpectra IndirectFittingModel::getSpectra(WorkspaceID workspaceID) const {
  return m_fitDataModel->getSpectra(workspaceID);
}

std::pair<double, double> IndirectFittingModel::getFittingRange(WorkspaceID workspaceID,
                                                                WorkspaceIndex spectrum) const {
  return m_fitDataModel->getFittingRange(workspaceID, spectrum);
}

std::string IndirectFittingModel::getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return m_fitDataModel->getExcludeRegion(workspaceID, spectrum);
}

std::vector<std::string> IndirectFittingModel::getWorkspaceNames() const { return m_fitDataModel->getWorkspaceNames(); }

std::vector<double> IndirectFittingModel::getExcludeRegionVector(WorkspaceID workspaceID,
                                                                 WorkspaceIndex spectrum) const {
  return m_fitDataModel->getExcludeRegionVector(workspaceID, spectrum);
}

std::string IndirectFittingModel::createDisplayName(WorkspaceID workspaceID) const {
  if (m_fitDataModel->getNumberOfWorkspaces() > workspaceID)
    return getFitDataName(m_fitDataModel->getWorkspaceNames()[workspaceID.value],
                          m_fitDataModel->getSpectra(workspaceID));
  else
    throw std::runtime_error("Cannot create a display name for a workspace:"
                             "the workspace index provided is too large.");
}

void IndirectFittingModel::setFitTypeString(const std::string &fitType) { m_fitString = fitType; }

std::string IndirectFittingModel::createOutputName(const std::string &fitMode) const {
  std::string inputWorkspace = isMultiFit() ? "Multi" : m_fitDataModel->getWorkspaceNames()[0];
  std::string spectra = isMultiFit() ? "" : m_fitDataModel->getSpectra(WorkspaceID{0}).getString();
  return inputWorkspace + "_" + m_fitType + "_" + fitMode + "_" + m_fitString + "_" + spectra + "_Results";
}

std::string IndirectFittingModel::sequentialFitOutputName() const { return createOutputName(SEQ_STRING); }

std::string IndirectFittingModel::simultaneousFitOutputName() const { return createOutputName(SIM_STRING); }

bool IndirectFittingModel::isMultiFit() const { return getNumberOfWorkspaces().value > 1; }

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

WorkspaceID IndirectFittingModel::getNumberOfWorkspaces() const { return m_fitDataModel->getNumberOfWorkspaces(); }

size_t IndirectFittingModel::getNumberOfSpectra(WorkspaceID workspaceID) const {
  return m_fitDataModel->getNumberOfSpectra(workspaceID);
}

size_t IndirectFittingModel::getNumberOfDomains() const { return m_fitDataModel->getNumberOfDomains(); }

FitDomainIndex IndirectFittingModel::getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return m_fitDataModel->getDomainIndex(workspaceID, spectrum);
}

std::vector<std::string> IndirectFittingModel::getFitParameterNames() const {
  if (!m_fitOutput->isEmpty())
    return m_fitOutput->getResultParameterNames();
  return std::vector<std::string>();
}

Mantid::API::MultiDomainFunction_sptr IndirectFittingModel::getFitFunction() const { return m_activeFunction; }

void IndirectFittingModel::setSpectra(const std::string &spectra, WorkspaceID workspaceID) {
  setSpectra(FunctionModelSpectra(spectra), workspaceID);
}

void IndirectFittingModel::setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) {
  m_fitDataModel->setSpectra(std::forward<FunctionModelSpectra>(spectra), workspaceID);
}

void IndirectFittingModel::setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) {
  m_fitDataModel->setSpectra(spectra, workspaceID);
}

void IndirectFittingModel::setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  m_fitDataModel->setStartX(startX, workspaceID, spectrum);
}

void IndirectFittingModel::setStartX(double startX, WorkspaceID workspaceID) {
  m_fitDataModel->setStartX(startX, workspaceID);
}

void IndirectFittingModel::setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  m_fitDataModel->setEndX(endX, workspaceID, spectrum);
}

void IndirectFittingModel::setEndX(double endX, WorkspaceID workspaceID) { m_fitDataModel->setEndX(endX, workspaceID); }

void IndirectFittingModel::setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID,
                                            WorkspaceIndex spectrum) {
  m_fitDataModel->setExcludeRegion(exclude, workspaceID, spectrum);
}

void IndirectFittingModel::addWorkspace(const std::string &workspaceName) {
  m_fitDataModel->addWorkspace(workspaceName);
  m_defaultParameters.emplace_back(createDefaultParameters(WorkspaceID{0}));
}

void IndirectFittingModel::addWorkspace(const std::string &workspaceName, const std::string &spectra) {
  m_fitDataModel->addWorkspace(workspaceName, spectra);
  m_defaultParameters.emplace_back(createDefaultParameters(WorkspaceID{0}));
}

void IndirectFittingModel::addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) {
  m_fitDataModel->addWorkspace(workspaceName, spectra);
  m_defaultParameters.emplace_back(createDefaultParameters(WorkspaceID{0}));
}

void IndirectFittingModel::addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                                        const FunctionModelSpectra &spectra) {
  m_fitDataModel->addWorkspace(workspace, spectra);
  m_defaultParameters.emplace_back(createDefaultParameters(WorkspaceID{0}));
}

void IndirectFittingModel::removeWorkspace(WorkspaceID workspaceID) {
  m_fitDataModel->removeWorkspace(workspaceID);
  m_defaultParameters.remove(workspaceID);
}

void IndirectFittingModel::removeFittingData() { m_fitOutput->clear(); }

void IndirectFittingModel::clearWorkspaces() {
  m_fitOutput->clear();
  m_fitDataModel->clear();
}

void IndirectFittingModel::clear() {}

void IndirectFittingModel::setFittingMode(FittingMode mode) { m_fittingMode = mode; }

void IndirectFittingModel::setFitFunction(MultiDomainFunction_sptr function) {
  m_activeFunction = std::move(function);
  m_previousModelSelected = isPreviousModelSelected();
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

IAlgorithm_sptr IndirectFittingModel::getFittingAlgorithm() const { return getFittingAlgorithm(m_fittingMode); }

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
  fitAlgorithm->setProperty("OutputWorkspace", singleFitOutputName(workspaceID, spectrum));
  return fitAlgorithm;
}

Mantid::API::IFunction_sptr IndirectFittingModel::getSingleFunction(WorkspaceID workspaceID,
                                                                    WorkspaceIndex spectrum) const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == getNumberOfDomains());
  if (function->getNumberDomains() == 0) {
    throw std::runtime_error("Cannot set up a fit: is the function defined?");
  }
  return function->getFunction(getDomainIndex(workspaceID, spectrum).value);
}

Mantid::API::IAlgorithm_sptr IndirectFittingModel::sequentialFitAlgorithm() const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == getNumberOfDomains());
  return AlgorithmManager::Instance().create("QENSFitSequential");
}

Mantid::API::IAlgorithm_sptr IndirectFittingModel::simultaneousFitAlgorithm() const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == getNumberOfDomains());
  return AlgorithmManager::Instance().create("QENSFitSimultaneous");
}

IAlgorithm_sptr IndirectFittingModel::createSequentialFit(IFunction_sptr function) const {
  const auto input = constructInputString(m_fitDataModel.get());
  return createSequentialFit(std::move(function), input);
}

IAlgorithm_sptr IndirectFittingModel::createSequentialFit(const IFunction_sptr &function,
                                                          const std::string &input) const {
  auto fitAlgorithm = sequentialFitAlgorithm();
  addFitProperties(*fitAlgorithm, std::move(function), getResultXAxisUnit());
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

std::string IndirectFittingModel::singleFitOutputName(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  std::string inputWorkspace = isMultiFit() ? "Multi" : m_fitDataModel->getWorkspaceNames()[workspaceID.value];
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

DataForParameterEstimationCollection
IndirectFittingModel::getDataForParameterEstimation(const EstimationDataSelector &selector) const {
  DataForParameterEstimationCollection dataCollection;
  for (auto i = WorkspaceID{0}; i < m_fitDataModel->getNumberOfWorkspaces(); ++i) {
    auto const ws = m_fitDataModel->getWorkspace(i);
    for (const auto &spectrum : m_fitDataModel->getSpectra(i)) {
      auto const &x = ws->readX(spectrum.value);
      auto const &y = ws->readY(spectrum.value);
      auto range = getFittingRange(i, spectrum);
      dataCollection.emplace_back(selector(x, y, range));
    }
  }
  return dataCollection;
}

std::vector<double> IndirectFittingModel::getQValuesForData() const { return m_fitDataModel->getQValuesForData(); }

std::vector<std::pair<std::string, size_t>> IndirectFittingModel::getResolutionsForFit() const {
  return std::vector<std::pair<std::string, size_t>>();
}

IIndirectFitDataTableModel *IndirectFittingModel::getFitDataModel() { return m_fitDataModel.get(); }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
