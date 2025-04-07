// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FittingModel.h"
#include "FitOutput.h"
#include "FitTabConstants.h"
#include "MantidQtWidgets/Spectroscopy/DataModel.h"

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
using namespace MantidQt::CustomInterfaces::Inelastic;

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

void addInputDataToSimultaneousFit(const IAlgorithm_sptr &fitAlgorithm, const IDataModel *fittingData) {
  for (auto index = FitDomainIndex{0}; index < FitDomainIndex{fittingData->getNumberOfDomains()}; ++index) {
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

std::string constructInputString(const IDataModel *fittingData) {
  std::ostringstream input;
  for (auto index = FitDomainIndex{0}; index < fittingData->getNumberOfDomains(); ++index) {
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
  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(algorithm->getProperty(propertyName)))
    return ads.retrieveWS<WorkspaceType>(algorithm->getProperty(propertyName));
  return nullptr;
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
  firstFunctionWithParameter(function, "Background", "A0")->setParameter("A0", value);
}

size_t getNumberOfSpecificFunctionContained(const std::string &functionName, const IFunction *compositeFunction) {
  assert(compositeFunction);

  if (compositeFunction->nFunctions() == 0) {
    return compositeFunction->name() == functionName ? 1 : 0;
  }

  size_t count{0};
  for (size_t i{0}; i < compositeFunction->nFunctions(); i++) {
    count += getNumberOfSpecificFunctionContained(functionName, compositeFunction->getFunction(i).get());
  }
  return count;
}

size_t getNumberOfCustomFunctions(MultiDomainFunction_const_sptr const &fittingFunction,
                                  const std::string &functionName) {
  if (fittingFunction && fittingFunction->nFunctions() > 0)
    return getNumberOfSpecificFunctionContained(functionName, fittingFunction->getFunction(0).get());
  return 0u;
}

} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

std::unordered_map<FittingMode, std::string> fitModeToName = std::unordered_map<FittingMode, std::string>(
    {{FittingMode::SEQUENTIAL, "Seq"}, {FittingMode::SIMULTANEOUS, "Sim"}});

FittingModel::FittingModel()
    : m_fitType("FitType"), m_fitString("FitString"), m_fitDataModel(std::make_unique<DataModel>()), m_fitPlotModel(),
      m_previousModelSelected(false), m_fittingMode(FittingMode::SEQUENTIAL),
      m_fitOutput(std::make_unique<FitOutput>()), m_activeFunction(), m_fitFunction(), m_defaultParameters() {
  m_fitPlotModel = std::make_unique<FitPlotModel>(m_fitDataModel->getFittingData(), m_fitOutput.get());
}

void FittingModel::validate(IUserInputValidator *validator) const {
  if (auto const invalidFunction = isInvalidFunction())
    validator->addErrorMessage(*invalidFunction);
}

// Functions that interact with FitDataModel

void FittingModel::addDefaultParameters() { m_defaultParameters.emplace_back(createDefaultParameters(WorkspaceID{0})); }

void FittingModel::removeDefaultParameters() {
  if (m_fitDataModel->getNumberOfWorkspaces() < m_defaultParameters.size()) {
    m_defaultParameters.remove(WorkspaceID{0});
  }
}

void FittingModel::clearWorkspaces() {
  m_fitOutput->clear();
  m_fitDataModel->clear();
}

MatrixWorkspace_sptr FittingModel::getWorkspace(WorkspaceID workspaceID) const {
  return m_fitDataModel->getWorkspace(workspaceID);
}

WorkspaceID FittingModel::getNumberOfWorkspaces() const { return m_fitDataModel->getNumberOfWorkspaces(); }

bool FittingModel::isMultiFit() const { return m_fitDataModel->getNumberOfWorkspaces().value > 1; }

// Other Functions
void FittingModel::updateFitTypeString() {
  auto const multiDomainFunction = getFitFunction();
  if (!multiDomainFunction || multiDomainFunction->nFunctions() == 0) {
    m_fitString = "NoCurrentFunction";
    return;
  }

  m_fitString.clear();
  for (auto const &fitFunctionName : FUNCTION_STRINGS) {
    auto occurances = getNumberOfCustomFunctions(multiDomainFunction, fitFunctionName.first);
    if (occurances > 0) {
      m_fitString += std::to_string(occurances) + fitFunctionName.second;
    }
  }

  if (getNumberOfCustomFunctions(multiDomainFunction, "DeltaFunction") > 0) {
    m_fitString += "Delta";
  }
}

std::string FittingModel::createOutputName(const std::string &fitMode, const std::string &workspaceName,
                                           const std::string &spectra) const {
  std::string inputWorkspace = isMultiFit() ? "Multi" : workspaceName;
  std::string inputSpectra = isMultiFit() ? "" : spectra;
  return inputWorkspace + "_" + m_fitType + "_" + fitMode + "_" + m_fitString + "_" + inputSpectra + getResultsSuffix();
}

std::optional<std::string> FittingModel::sequentialFitOutputName() const {
  auto const workspaceNames = m_fitDataModel->getWorkspaceNames();
  if (workspaceNames.empty())
    return std::nullopt;
  return createOutputName(SEQ_STRING, workspaceNames[0], m_fitDataModel->getSpectra(WorkspaceID{0}).getString());
}

std::optional<std::string> FittingModel::simultaneousFitOutputName() const {
  auto const workspaceNames = m_fitDataModel->getWorkspaceNames();
  if (workspaceNames.empty())
    return std::nullopt;
  return createOutputName(SIM_STRING, workspaceNames[0], m_fitDataModel->getSpectra(WorkspaceID{0}).getString());
}

bool FittingModel::isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  auto domainIndex = m_fitDataModel->getDomainIndex(workspaceID, spectrum);
  return m_fitOutput->isSpectrumFit(domainIndex);
}

std::optional<std::string> FittingModel::isInvalidFunction() const {
  if (!m_activeFunction)
    return std::string("No fit function has been defined");

  const auto composite = std::dynamic_pointer_cast<CompositeFunction>(m_activeFunction);
  if (composite && (composite->nFunctions() == 0 || composite->nParams() == 0))
    return std::string("No fitting functions have been defined.");
  return std::nullopt;
}

std::vector<std::string> FittingModel::getFitParameterNames() const {
  if (!m_fitOutput->isEmpty())
    return m_fitOutput->getResultParameterNames();
  return std::vector<std::string>();
}

IFitOutput *FittingModel::getFitOutput() const { return m_fitOutput.get(); }

Mantid::API::MultiDomainFunction_sptr FittingModel::getFitFunction() const { return m_activeFunction; }

void FittingModel::removeFittingData() { m_fitOutput->clear(); }

void FittingModel::setFittingMode(FittingMode mode) { m_fittingMode = mode; }

void FittingModel::setFitFunction(MultiDomainFunction_sptr function) {
  m_activeFunction = std::move(function);
  m_previousModelSelected = isPreviousModelSelected();
}

void FittingModel::setFWHM(double fwhm, WorkspaceID WorkspaceID) {
  setDefaultParameterValue("FWHM", fwhm, WorkspaceID);
  setFunctionParameters(getFitFunction(), "Peak", "FWHM", fwhm);
}

void FittingModel::setBackground(double background, WorkspaceID WorkspaceID) {
  setDefaultParameterValue("A0", background, WorkspaceID);
  setFirstBackground(getFitFunction(), background);
}

void FittingModel::setDefaultParameterValue(const std::string &name, double value, WorkspaceID workspaceID) {
  if (m_defaultParameters.size() > workspaceID)
    m_defaultParameters[workspaceID][name] = ParameterValue(value);
}

void FittingModel::addOutput(IAlgorithm_sptr fitAlgorithm) {
  auto group = getOutputGroup(fitAlgorithm);
  auto parameters = getOutputParameters(fitAlgorithm);
  auto result = getOutputResult(fitAlgorithm);
  if (!group || !parameters || !result) {
    return;
  }
  if (group->size() == 1u) {
    m_fitFunction = FunctionFactory::Instance().createInitialized(fitAlgorithm->getPropertyValue("Function"));
  } else {
    m_fitFunction = extractFirstInnerFunction(fitAlgorithm->getPropertyValue("Function"));
  }
  m_fitOutput->addOutput(group, parameters, result, m_fitPlotModel->getActiveDomainIndex());
  m_previousModelSelected = isPreviousModelSelected();
}

FittingMode FittingModel::getFittingMode() const { return m_fittingMode; }

std::unordered_map<std::string, ParameterValue> FittingModel::getParameterValues(WorkspaceID workspaceID,
                                                                                 WorkspaceIndex spectrum) const {
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

std::unordered_map<std::string, ParameterValue> FittingModel::getFitParameters(WorkspaceID workspaceID,
                                                                               WorkspaceIndex spectrum) const {
  auto fitDomainIndex = m_fitDataModel->getDomainIndex(workspaceID, spectrum);
  if (!m_fitOutput->isEmpty())
    return m_fitOutput->getParameters(fitDomainIndex);
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, ParameterValue> FittingModel::getDefaultParameters(WorkspaceID workspaceID) const {
  if (workspaceID < m_defaultParameters.size())
    return mapKeys(m_defaultParameters[workspaceID], mapDefaultParameterNames());
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, std::string> FittingModel::mapDefaultParameterNames() const {
  if (m_activeFunction)
    return shortToLongParameterNames(getFitFunction());
  return std::unordered_map<std::string, std::string>();
}

std::unordered_map<std::string, ParameterValue> FittingModel::createDefaultParameters(WorkspaceID) const {
  return std::unordered_map<std::string, ParameterValue>();
}

std::string FittingModel::getResultXAxisUnit() const { return "MomentumTransfer"; }

std::string FittingModel::getResultLogName() const { return "axis-1"; }

std::optional<ResultLocationNew> FittingModel::getResultLocation(WorkspaceID workspaceID,
                                                                 WorkspaceIndex spectrum) const {
  auto fitDomainIndex = m_fitDataModel->getDomainIndex(workspaceID, spectrum);
  if (!m_fitOutput->isEmpty() && m_fitDataModel->getNumberOfWorkspaces() > workspaceID)
    return m_fitOutput->getResultLocation(fitDomainIndex);
  return std::nullopt;
}

WorkspaceGroup_sptr FittingModel::getResultWorkspace() const { return m_fitOutput->getLastResultWorkspace(); }

WorkspaceGroup_sptr FittingModel::getResultGroup() const { return m_fitOutput->getLastResultGroup(); }

bool FittingModel::isPreviousModelSelected() const {
  return m_fitFunction && equivalentFunctions(extractFirstInnerFunction(getFitFunction()), m_fitFunction);
}

MultiDomainFunction_sptr FittingModel::getMultiDomainFunction() const { return m_activeFunction; }

IAlgorithm_sptr FittingModel::getFittingAlgorithm(FittingMode mode) const {
  if (mode == FittingMode::SEQUENTIAL) {
    if (m_activeFunction->getNumberDomains() == 0) {
      throw std::runtime_error("Function is undefined");
    }
    return createSequentialFit(getFitFunction());
  } else
    return createSimultaneousFit(getFitFunction());
}

IAlgorithm_sptr FittingModel::getSingleFittingAlgorithm() const {
  const auto workspaceID = m_fitPlotModel->getActiveWorkspaceID();
  const auto spectrum = m_fitPlotModel->getActiveWorkspaceIndex();
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

Mantid::API::IFunction_sptr FittingModel::getSingleFunction(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == m_fitDataModel->getNumberOfDomains());
  if (function->getNumberDomains() == 0) {
    throw std::runtime_error("Cannot set up a fit: is the function defined?");
  }
  return function->getFunction(m_fitDataModel->getDomainIndex(workspaceID, spectrum).value);
}

Mantid::API::IAlgorithm_sptr FittingModel::sequentialFitAlgorithm() const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == m_fitDataModel->getNumberOfDomains());
  return AlgorithmManager::Instance().create("QENSFitSequential");
}

Mantid::API::IAlgorithm_sptr FittingModel::simultaneousFitAlgorithm() const {
  auto function = getFitFunction();
  assert(function->getNumberDomains() == m_fitDataModel->getNumberOfDomains());
  return AlgorithmManager::Instance().create("QENSFitSimultaneous");
}

IAlgorithm_sptr FittingModel::createSequentialFit(const IFunction_sptr function) const {
  const auto input = constructInputString(m_fitDataModel.get());
  return createSequentialFit(function, input);
}

IAlgorithm_sptr FittingModel::createSequentialFit(const IFunction_sptr function, const std::string &input) const {
  auto const outputName = sequentialFitOutputName();
  if (!outputName) {
    throw std::runtime_error("Data has not been loaded.");
  }
  auto fitAlgorithm = sequentialFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());
  fitAlgorithm->setProperty("Input", input);
  fitAlgorithm->setProperty("OutputWorkspace", *outputName);
  fitAlgorithm->setProperty("LogName", getResultLogName());
  std::stringstream startX;
  std::stringstream endX;
  for (size_t i = 0; i < m_fitDataModel->getNumberOfDomains(); i++) {
    const auto range = m_fitDataModel->getFittingRange(FitDomainIndex(i));
    startX << std::setprecision(6) << std::floor(range.first * 1E6) / 1E6 << ",";
    endX << std::setprecision(6) << std::ceil(range.second * 1E6) / 1E6 << ",";
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

IAlgorithm_sptr FittingModel::createSimultaneousFit(const MultiDomainFunction_sptr &function) const {
  auto const outputName = simultaneousFitOutputName();
  if (!outputName) {
    throw std::runtime_error("Data has not been loaded.");
  }
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());
  addInputDataToSimultaneousFit(fitAlgorithm, m_fitDataModel.get());
  fitAlgorithm->setProperty("OutputWorkspace", *outputName);
  return fitAlgorithm;
}
std::string FittingModel::getResultsSuffix() const { return isMultiFit() ? "_Results" : "_Result"; }
std::string FittingModel::singleFitOutputName(std::string workspaceName, WorkspaceIndex spectrum) const {
  std::string inputWorkspace = isMultiFit() ? "Multi" : workspaceName;
  std::string spectra = std::to_string(spectrum.value);
  return inputWorkspace + "_" + m_fitType + "_" + m_fitString + "_" + spectra + getResultsSuffix();
}

std::optional<std::string> FittingModel::getOutputBasename() const {
  if (auto const outputName = sequentialFitOutputName())
    return cutLastOf(*outputName, getResultsSuffix());
  return std::nullopt;
}

void FittingModel::cleanFailedRun(const IAlgorithm_sptr &fittingAlgorithm) {
  const auto prefix = "__" + fittingAlgorithm->name() + "_ws";

  auto group = getOutputGroup(fittingAlgorithm);
  if (group && group->size() == 1u) {
    const auto base = prefix + std::to_string(m_fitPlotModel->getActiveWorkspaceID().value + 1);
    removeFromADSIfExists(base);
    cleanTemporaries(base + "_0");
    return;
  }

  for (WorkspaceID datasetIndex = 0; datasetIndex < m_fitDataModel->getNumberOfWorkspaces(); ++datasetIndex) {
    const auto base = prefix + std::to_string(datasetIndex.value + 1);
    removeFromADSIfExists(base);
    for (size_t index = 0; index < m_fitDataModel->getNumberOfSpectra(datasetIndex); index++) {
      cleanTemporaries(base + "_" + std::to_string(index));
    }
  }
}

IDataModel *FittingModel::getFitDataModel() const { return m_fitDataModel.get(); }

IFitPlotModel *FittingModel::getFitPlotModel() const { return m_fitPlotModel.get(); }

} // namespace MantidQt::CustomInterfaces::Inelastic
