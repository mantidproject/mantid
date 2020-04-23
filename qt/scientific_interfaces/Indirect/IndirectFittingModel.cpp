// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFittingModel.h"
#include "IndirectFitDataModel.h"
#include "IndirectWorkspaceNames.h"

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
using IDAWorkspaceIndex = MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

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

bool equivalentWorkspaces(const MatrixWorkspace_const_sptr &lhs,
                          const MatrixWorkspace_const_sptr &rhs) {
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
bool functionNameComparator(const IFunction_const_sptr &first,
                            const IFunction_const_sptr &second) {
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

bool equivalentFunctions(const IFunction_const_sptr &func1,
                         const IFunction_const_sptr &func2);

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
bool equivalentFunctions(const IFunction_const_sptr &func1,
                         const IFunction_const_sptr &func2) {
  const auto composite1 =
      std::dynamic_pointer_cast<const CompositeFunction>(func1);
  const auto composite2 =
      std::dynamic_pointer_cast<const CompositeFunction>(func2);

  if (composite1 && composite2)
    return equivalentComposites(*composite1, *composite2);
  else if (func1 && func2 && !composite1 && !composite2)
    return func1->name() == func2->name();
  return false;
}

void addInputDataToSimultaneousFit(const IAlgorithm_sptr &fitAlgorithm,
                                   const MatrixWorkspace_sptr &workspace,
                                   IDAWorkspaceIndex spectrum,
                                   const std::pair<double, double> &xRange,
                                   const std::vector<double> &excludeRegions,
                                   const std::string &suffix) {
  fitAlgorithm->setProperty("InputWorkspace" + suffix, workspace);
  fitAlgorithm->setProperty("StartX" + suffix, xRange.first);
  fitAlgorithm->setProperty("EndX" + suffix, xRange.second);
  fitAlgorithm->setProperty("WorkspaceIndex" + suffix, spectrum.value);

  if (!excludeRegions.empty())
    fitAlgorithm->setProperty("Exclude" + suffix, excludeRegions);
}

void addInputDataToSimultaneousFitWithEqualRange(
    const IAlgorithm_sptr &fitAlgorithm, const IIndirectFitData *fittingData) {
  auto fittingRange = fittingData->getFittingRange(FitDomainIndex{0});
  for (auto index = FitDomainIndex{0};
       index < FitDomainIndex{fittingData->getNumberOfDomains()}; index++) {
    std::string suffix =
        index == FitDomainIndex{0} ? "" : std::to_string(index.value);
    addInputDataToSimultaneousFit(
        fitAlgorithm, fittingData->getWorkspace(index),
        fittingData->getSpectrum(index), fittingRange,
        fittingData->getExcludeRegionVector(index), suffix);
  }
}

void addInputDataToSimultaneousFit(const IAlgorithm_sptr &fitAlgorithm,
                                   const IIndirectFitData *fittingData) {
  for (auto index = FitDomainIndex{0};
       index < FitDomainIndex{fittingData->getNumberOfDomains()}; index++) {
    std::string suffix =
        index == FitDomainIndex{0} ? "" : std::to_string(index.value);
    addInputDataToSimultaneousFit(
        fitAlgorithm, fittingData->getWorkspace(index),
        fittingData->getSpectrum(index), fittingData->getFittingRange(index),
        fittingData->getExcludeRegionVector(index), suffix);
  }
}

template <typename Map> Map combine(const Map &mapA, const Map &mapB) {
  Map newMap(mapA);
  newMap.insert(std::begin(mapB), std::end(mapB));
  return newMap;
}

std::unordered_map<std::string, std::string>
shortToLongParameterNames(const IFunction_sptr &function) {
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
                      const std::unique_ptr<IndirectFitData> &fitData) {
  removeFromADSIfExists(base);

  const auto clean = [&](IDAWorkspaceIndex index,
                         IDAWorkspaceIndex /*unused*/) {
    cleanTemporaries(base + "_" + std::to_string(index.value));
  };
  fitData->applyEnumeratedSpectra(clean);
}

void cleanTemporaries(const std::string &algorithmName,
                      const IndirectFitDataCollectionType &fittingData) {
  const auto prefix = "__" + algorithmName + "_ws";
  for (TableDatasetIndex i = fittingData.zero(); i < fittingData.size(); ++i)
    cleanTemporaries(prefix + std::to_string(i.value + 1), fittingData[i]);
}

IFunction_sptr extractFirstInnerFunction(IFunction_sptr function) {
  if (const auto multiDomain =
          std::dynamic_pointer_cast<MultiDomainFunction>(function)) {
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
std::shared_ptr<WorkspaceType>
getWorkspaceOutput(const IAlgorithm_sptr &algorithm,
                   const std::string &propertyName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceType>(
      algorithm->getProperty(propertyName));
}

WorkspaceGroup_sptr getOutputResult(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<WorkspaceGroup>(std::move(algorithm),
                                            "OutputWorkspace");
}

ITableWorkspace_sptr getOutputParameters(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<ITableWorkspace>(std::move(algorithm),
                                             "OutputParameterWorkspace");
}

WorkspaceGroup_sptr getOutputGroup(const IAlgorithm_sptr &algorithm) {
  return getWorkspaceOutput<WorkspaceGroup>(std::move(algorithm),
                                            "OutputWorkspaceGroup");
}

void addFitProperties(Mantid::API::IAlgorithm &algorithm,
                      const Mantid::API::IFunction_sptr &function,
                      std::string const &xAxisUnit) {
  algorithm.setProperty("Function", function);
  algorithm.setProperty("ResultXAxisUnit", xAxisUnit);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

std::unordered_map<FittingMode, std::string> fitModeToName =
    std::unordered_map<FittingMode, std::string>(
        {{FittingMode::SEQUENTIAL, "Seq"}, {FittingMode::SIMULTANEOUS, "Sim"}});

PrivateFittingData::PrivateFittingData() : m_data() {}

PrivateFittingData::PrivateFittingData(PrivateFittingData &&privateData)
    : m_data(std::move(privateData.m_data)) {}

PrivateFittingData::PrivateFittingData(IndirectFitDataCollectionType &&data)
    : m_data(std::move(data)) {}

PrivateFittingData &
PrivateFittingData::operator=(PrivateFittingData &&fittingData) {
  m_data = std::move(fittingData.m_data);
  return *this;
}

IndirectFittingModel::IndirectFittingModel()
    : m_previousModelSelected(false), m_fittingMode(FittingMode::SEQUENTIAL) {
  m_fitDataModel = std::make_unique<IndirectFitDataModel>();
}

bool IndirectFittingModel::hasWorkspace(
    std::string const &workspaceName) const {
  return m_fitDataModel->hasWorkspace(workspaceName);
}

MatrixWorkspace_sptr
IndirectFittingModel::getWorkspace(TableDatasetIndex index) const {
  return m_fitDataModel->getWorkspace(index);
}

Spectra IndirectFittingModel::getSpectra(TableDatasetIndex index) const {
  return m_fitDataModel->getSpectra(index);
}

std::pair<double, double>
IndirectFittingModel::getFittingRange(TableDatasetIndex dataIndex,
                                      WorkspaceIndex spectrum) const {
  return m_fitDataModel->getFittingRange(dataIndex, spectrum);
}

std::string
IndirectFittingModel::getExcludeRegion(TableDatasetIndex dataIndex,
                                       WorkspaceIndex spectrum) const {
  return m_fitDataModel->getExcludeRegion(dataIndex, spectrum);
}

std::vector<std::string> IndirectFittingModel::getWorkspaceNames() const {
  return m_fitDataModel->getWorkspaceNames();
}

std::vector<double>
IndirectFittingModel::getExcludeRegionVector(TableDatasetIndex dataIndex,
                                             WorkspaceIndex index) const {
  return m_fitDataModel->getExcludeRegionVector(dataIndex, index);
}

std::string
IndirectFittingModel::createDisplayName(const std::string &formatString,
                                        const std::string &rangeDelimiter,
                                        TableDatasetIndex dataIndex) const {
  if (m_fitDataModel->numberOfWorkspaces() > dataIndex)
    return getFitDataName(m_fitDataModel->getWorkspaceNames()[dataIndex.value],
                          m_fitDataModel->getSpectra(dataIndex));
  else
    throw std::runtime_error("Cannot create a display name for a workspace:"
                             "the workspace index provided is too large.");
}

std::string
IndirectFittingModel::createOutputName(const std::string &formatString,
                                       const std::string &rangeDelimiter,
                                       TableDatasetIndex dataIndex) const {
  return createDisplayName(formatString, rangeDelimiter, dataIndex) +
         "_Results";
}

bool IndirectFittingModel::isMultiFit() const {
  return numberOfWorkspaces().value > 1;
}

bool IndirectFittingModel::isPreviouslyFit(TableDatasetIndex dataIndex,
                                           WorkspaceIndex spectrum) const {
  // if (!m_previousModelSelected || !m_fitOutput ||
  //     m_fittingData.size() <= dataIndex)
  //   return false;
  // const auto fitData = m_fittingData[dataIndex].get();
  // return m_fitOutput->isSpectrumFit(fitData, spectrum);
  return false;
}

boost::optional<std::string> IndirectFittingModel::isInvalidFunction() const {
  if (!m_activeFunction)
    return std::string("No fit function has been defined");

  const auto composite =
      std::dynamic_pointer_cast<CompositeFunction>(m_activeFunction);
  if (composite && (composite->nFunctions() == 0 || composite->nParams() == 0))
    return std::string("No fitting functions have been defined.");
  return boost::none;
}

TableDatasetIndex IndirectFittingModel::numberOfWorkspaces() const {
  return m_fitDataModel->numberOfWorkspaces();
}

int IndirectFittingModel::getNumberOfSpectra(TableDatasetIndex index) const {
  return m_fitDataModel->getNumberOfSpectra(index);
}

int IndirectFittingModel::getNumberOfDomains() const {
  return m_fitDataModel->getNumberOfDomains();
}

FitDomainIndex
IndirectFittingModel::getDomainIndex(TableDatasetIndex dataIndex,
                                     WorkspaceIndex spectrum) const {
  return m_fitDataModel->getDomainIndex(dataIndex, spectrum);
}

std::vector<std::string> IndirectFittingModel::getFitParameterNames() const {
  if (m_fitOutput)
    return m_fitOutput->getResultParameterNames();
  return std::vector<std::string>();
}

Mantid::API::MultiDomainFunction_sptr
IndirectFittingModel::getFittingFunction() const {
  return m_activeFunction;
}

// void IndirectFittingModel::setFittingData(PrivateFittingData &&fittingData) {
//   m_fittingData = std::move(fittingData.m_data);
// }

void IndirectFittingModel::setSpectra(const std::string &spectra,
                                      TableDatasetIndex dataIndex) {
  setSpectra(Spectra(spectra), dataIndex);
}

void IndirectFittingModel::setSpectra(Spectra &&spectra,
                                      TableDatasetIndex dataIndex) {
  m_fitDataModel->setSpectra(std::forward<Spectra>(spectra), dataIndex);
}

void IndirectFittingModel::setSpectra(const Spectra &spectra,
                                      TableDatasetIndex dataIndex) {
  m_fitDataModel->setSpectra(spectra, dataIndex);
}

void IndirectFittingModel::setStartX(double startX, TableDatasetIndex dataIndex,
                                     WorkspaceIndex spectrum) {
  m_fitDataModel->setStartX(startX, dataIndex, spectrum);
}

void IndirectFittingModel::setStartX(double startX,
                                     TableDatasetIndex dataIndex) {
  m_fitDataModel->setStartX(startX, dataIndex);
}

void IndirectFittingModel::setEndX(double endX, TableDatasetIndex dataIndex,
                                   WorkspaceIndex spectrum) {
  m_fitDataModel->setEndX(endX, dataIndex, spectrum);
}

void IndirectFittingModel::setEndX(double endX, TableDatasetIndex dataIndex) {
  m_fitDataModel->setEndX(endX, dataIndex);
}

void IndirectFittingModel::setExcludeRegion(const std::string &exclude,
                                            TableDatasetIndex dataIndex,
                                            WorkspaceIndex spectrum) {
  m_fitDataModel->setExcludeRegion(exclude, dataIndex, spectrum);
}

void IndirectFittingModel::addWorkspace(const std::string &workspaceName) {
  m_fitDataModel->addWorkspace(workspaceName);
}

void IndirectFittingModel::addWorkspace(const std::string &workspaceName,
                                        const std::string &spectra) {
  m_fitDataModel->addWorkspace(workspaceName, spectra);
}

void IndirectFittingModel::addWorkspace(const std::string &workspaceName,
                                        const Spectra &spectra) {
  m_fitDataModel->addWorkspace(workspaceName, spectra);
}

void IndirectFittingModel::removeWorkspace(TableDatasetIndex index) {
  m_fitDataModel->removeWorkspace(index);
  // removeWorkspaceFromFittingData(index);

  // if (index > m_fittingData.zero() && m_fittingData.size() > index) {
  //   const auto previousWS =
  //       m_fittingData[index - TableDatasetIndex{1}]->workspace();
  //   const auto subsequentWS = m_fittingData[index]->workspace();

  //   if (equivalentWorkspaces(previousWS, subsequentWS)) {
  //     m_fittingData[index - TableDatasetIndex{1}]->combine(
  //         *m_fittingData[index]);
  //     m_fittingData.remove(index);
  //   }
  // }
}

void IndirectFittingModel::removeFittingData(TableDatasetIndex index) {
  // if (m_fitOutput)
  //   m_fitOutput->removeOutput(m_fittingData[index].get());
  // m_fittingData.remove(index);
  // if (m_defaultParameters.size() > index)
  //   m_defaultParameters.remove(index);
}

void IndirectFittingModel::clearWorkspaces() {
  m_fitOutput.reset();
  m_fitDataModel->clear();
  // return std::move(m_fittingData);
}

void IndirectFittingModel::clear(){};

void IndirectFittingModel::setFittingMode(FittingMode mode) {
  m_fittingMode = mode;
}

void IndirectFittingModel::setFitFunction(MultiDomainFunction_sptr function) {
  m_activeFunction = std::move(function);
  m_previousModelSelected = isPreviousModelSelected();
}

void IndirectFittingModel::setDefaultParameterValue(
    const std::string &name, double value, TableDatasetIndex dataIndex) {
  if (m_defaultParameters.size() > dataIndex)
    m_defaultParameters[dataIndex][name] = ParameterValue(value);
}

void IndirectFittingModel::addOutput(IAlgorithm_sptr fitAlgorithm) {
  // addOutput(std::move(fitAlgorithm), m_fittingData.begin(),
  //           m_fittingData.end());
}

void IndirectFittingModel::addOutput(const IAlgorithm_sptr &fitAlgorithm,
                                     const FitDataIterator &fitDataBegin,
                                     const FitDataIterator &fitDataEnd) {
  auto group = getOutputGroup(fitAlgorithm);
  auto parameters = getOutputParameters(fitAlgorithm);
  auto result = getOutputResult(fitAlgorithm);
  m_fitFunction =
      extractFirstInnerFunction(fitAlgorithm->getPropertyValue("Function"));
  addOutput(group, parameters, result, fitDataBegin, fitDataEnd);
}

void IndirectFittingModel::addSingleFitOutput(
    const IAlgorithm_sptr &fitAlgorithm, TableDatasetIndex index) {
  // auto group = getOutputGroup(fitAlgorithm);
  // auto parameters = getOutputParameters(fitAlgorithm);
  // auto result = getOutputResult(fitAlgorithm);
  // int spectrum = fitAlgorithm->getProperty("WorkspaceIndex");
  // m_fitFunction = FunctionFactory::Instance().createInitialized(
  //     fitAlgorithm->getPropertyValue("Function"));
  // addOutput(group, parameters, result, m_fittingData[index].get(),
  //           WorkspaceIndex{spectrum});
}

void IndirectFittingModel::addOutput(const WorkspaceGroup_sptr &resultGroup,
                                     const ITableWorkspace_sptr &parameterTable,
                                     const WorkspaceGroup_sptr &resultWorkspace,
                                     const FitDataIterator &fitDataBegin,
                                     const FitDataIterator &fitDataEnd) {
  if (m_previousModelSelected && m_fitOutput)
    addOutput(m_fitOutput.get(), resultGroup, parameterTable, resultWorkspace,
              fitDataBegin, fitDataEnd);
  else
    m_fitOutput = std::make_unique<IndirectFitOutput>(
        createFitOutput(resultGroup, parameterTable, resultWorkspace,
                        fitDataBegin, fitDataEnd));
  m_previousModelSelected = isPreviousModelSelected();
}

void IndirectFittingModel::addOutput(const WorkspaceGroup_sptr &resultGroup,
                                     const ITableWorkspace_sptr &parameterTable,
                                     const WorkspaceGroup_sptr &resultWorkspace,
                                     IndirectFitData *fitData,
                                     WorkspaceIndex spectrum) {
  if (m_previousModelSelected && m_fitOutput)
    addOutput(m_fitOutput.get(), resultGroup, parameterTable, resultWorkspace,
              fitData, spectrum);
  else
    m_fitOutput = std::make_unique<IndirectFitOutput>(createFitOutput(
        resultGroup, parameterTable, resultWorkspace, fitData, spectrum));
  m_previousModelSelected = isPreviousModelSelected();
}

IndirectFitOutput IndirectFittingModel::createFitOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    WorkspaceGroup_sptr resultWorkspace, const FitDataIterator &fitDataBegin,
    const FitDataIterator &fitDataEnd) const {
  return IndirectFitOutput(std::move(resultGroup), std::move(parameterTable),
                           std::move(resultWorkspace), fitDataBegin,
                           fitDataEnd);
}

IndirectFitOutput IndirectFittingModel::createFitOutput(
    Mantid::API::WorkspaceGroup_sptr resultGroup,
    Mantid::API::ITableWorkspace_sptr parameterTable,
    Mantid::API::WorkspaceGroup_sptr resultWorkspace, IndirectFitData *fitData,
    WorkspaceIndex spectrum) const {
  return IndirectFitOutput(std::move(resultGroup), std::move(parameterTable),
                           std::move(resultWorkspace), fitData, spectrum);
}

void IndirectFittingModel::addOutput(IndirectFitOutput *fitOutput,
                                     WorkspaceGroup_sptr resultGroup,
                                     ITableWorkspace_sptr parameterTable,
                                     WorkspaceGroup_sptr resultWorkspace,
                                     const FitDataIterator &fitDataBegin,
                                     const FitDataIterator &fitDataEnd) const {
  fitOutput->addOutput(std::move(resultGroup), std::move(parameterTable),
                       std::move(resultWorkspace), fitDataBegin, fitDataEnd);
}

void IndirectFittingModel::addOutput(
    IndirectFitOutput *fitOutput, Mantid::API::WorkspaceGroup_sptr resultGroup,
    Mantid::API::ITableWorkspace_sptr parameterTable,
    Mantid::API::WorkspaceGroup_sptr resultWorkspace, IndirectFitData *fitData,
    WorkspaceIndex spectrum) const {
  fitOutput->addOutput(std::move(resultGroup), std::move(parameterTable),
                       std::move(resultWorkspace), fitData, spectrum);
}

FittingMode IndirectFittingModel::getFittingMode() const {
  return m_fittingMode;
}

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getParameterValues(TableDatasetIndex index,
                                         WorkspaceIndex spectrum) const {
  if (m_fitDataModel->numberOfWorkspaces() > index) {
    const auto parameters = getFitParameters(index, spectrum);
    if (m_previousModelSelected)
      return parameters;
    else if (parameters.empty())
      return getDefaultParameters(index);
    return combine(getDefaultParameters(index), parameters);
  }
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getFitParameters(TableDatasetIndex index,
                                       WorkspaceIndex spectrum) const {
  // if (m_fitOutput)
  //   return m_fitOutput->getParameters(m_fittingData[index].get(), spectrum);
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getDefaultParameters(TableDatasetIndex index) const {
  if (index < m_defaultParameters.size())
    return mapKeys(m_defaultParameters[index], mapDefaultParameterNames());
  return std::unordered_map<std::string, ParameterValue>();
}

std::unordered_map<std::string, std::string>
IndirectFittingModel::mapDefaultParameterNames() const {
  if (m_activeFunction)
    return shortToLongParameterNames(getFittingFunction());
  return std::unordered_map<std::string, std::string>();
}

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::createDefaultParameters(
    TableDatasetIndex /*unused*/) const {
  return std::unordered_map<std::string, ParameterValue>();
}

std::string IndirectFittingModel::getResultXAxisUnit() const {
  return "MomentumTransfer";
}

std::string IndirectFittingModel::getResultLogName() const { return "axis-1"; }

boost::optional<ResultLocationNew>
IndirectFittingModel::getResultLocation(TableDatasetIndex index,
                                        WorkspaceIndex spectrum) const {
  // if (m_fitOutput && m_fittingData.size() > index)
  //   return m_fitOutput->getResultLocation(m_fittingData[index].get(),
  //   spectrum);
  return boost::none;
}

WorkspaceGroup_sptr IndirectFittingModel::getResultWorkspace() const {
  return m_fitOutput->getLastResultWorkspace();
}

WorkspaceGroup_sptr IndirectFittingModel::getResultGroup() const {
  return m_fitOutput->getLastResultGroup();
}

bool IndirectFittingModel::isPreviousModelSelected() const {
  return m_fitFunction &&
         equivalentFunctions(extractFirstInnerFunction(getFittingFunction()),
                             m_fitFunction);
}

MultiDomainFunction_sptr IndirectFittingModel::getMultiDomainFunction() const {
  return m_activeFunction;
}

IAlgorithm_sptr IndirectFittingModel::getFittingAlgorithm() const {
  return getFittingAlgorithm(m_fittingMode);
}

IAlgorithm_sptr
IndirectFittingModel::getFittingAlgorithm(FittingMode mode) const {
  if (mode == FittingMode::SEQUENTIAL) {
    if (m_activeFunction->getNumberDomains() == 0) {
      throw std::runtime_error("Function is undefined");
    }
    return createSequentialFit(getFittingFunction());
  } else
    return createSimultaneousFit(getFittingFunction());
}

IAlgorithm_sptr
IndirectFittingModel::getSingleFit(TableDatasetIndex dataIndex,
                                   WorkspaceIndex spectrum) const {
  const auto ws = m_fitDataModel->getWorkspace(dataIndex);
  const auto range = m_fitDataModel->getFittingRange(dataIndex, spectrum);
  const auto exclude =
      m_fitDataModel->getExcludeRegionVector(dataIndex, spectrum);
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, getSingleFunction(dataIndex, spectrum),
                   getResultXAxisUnit());
  addInputDataToSimultaneousFit(fitAlgorithm, ws, spectrum, range, exclude,
                                std::string(""));
  fitAlgorithm->setProperty("OutputWorkspace",
                            singleFitOutputName(dataIndex, spectrum));
  return fitAlgorithm;
}

Mantid::API::IFunction_sptr
IndirectFittingModel::getSingleFunction(TableDatasetIndex dataIndex,
                                        WorkspaceIndex spectrum) const {
  auto function = getFittingFunction();
  assert(function->getNumberDomains() ==
         static_cast<size_t>(getNumberOfDomains()));
  if (function->getNumberDomains() == 0) {
    throw std::runtime_error("Cannot set up a fit: is the function defined?");
  }
  return function->getFunction(getDomainIndex(dataIndex, spectrum).value);
}

Mantid::API::IAlgorithm_sptr
IndirectFittingModel::sequentialFitAlgorithm() const {
  auto function = getFittingFunction();
  assert(function->getNumberDomains() ==
         static_cast<size_t>(getNumberOfDomains()));
  return AlgorithmManager::Instance().create("QENSFitSequential");
}

Mantid::API::IAlgorithm_sptr
IndirectFittingModel::simultaneousFitAlgorithm() const {
  auto function = getFittingFunction();
  assert(function->getNumberDomains() ==
         static_cast<size_t>(getNumberOfDomains()));
  return AlgorithmManager::Instance().create("QENSFitSimultaneous");
}

IAlgorithm_sptr
IndirectFittingModel::createSequentialFit(IFunction_sptr function) const {
  const auto input = std::string(); // constructInputString(m_fittingData);
  return createSequentialFit(std::move(function), input);
}

IAlgorithm_sptr
IndirectFittingModel::createSequentialFit(const IFunction_sptr &function,
                                          const std::string &input) const {
  auto fitAlgorithm = sequentialFitAlgorithm();
  addFitProperties(*fitAlgorithm, std::move(function), getResultXAxisUnit());
  fitAlgorithm->setProperty("Input", input);
  fitAlgorithm->setProperty("OutputWorkspace", sequentialFitOutputName());
  fitAlgorithm->setProperty("LogName", getResultLogName());

  auto const firstWsIndex =
      WorkspaceIndex{0}; // m_fitDataModel->getSpectrum(FitDomainIndex{0});
  const auto range =
      m_fitDataModel->getFittingRange(TableDatasetIndex{0}, firstWsIndex);
  fitAlgorithm->setProperty("StartX", range.first);
  fitAlgorithm->setProperty("EndX", range.second);

  auto excludeRegion = m_fitDataModel->getExcludeRegionVector(
      TableDatasetIndex{0}, firstWsIndex);
  if (!excludeRegion.empty()) {
    fitAlgorithm->setProperty("Exclude", excludeRegion);
  }

  return fitAlgorithm;
}

IAlgorithm_sptr IndirectFittingModel::createSimultaneousFit(
    const MultiDomainFunction_sptr &function) const {
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, function, getResultXAxisUnit());
  addInputDataToSimultaneousFit(fitAlgorithm, m_fitDataModel.get());
  return fitAlgorithm;
}

IAlgorithm_sptr IndirectFittingModel::createSimultaneousFitWithEqualRange(
    const IFunction_sptr &function) const {
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addFitProperties(*fitAlgorithm, std::move(function), getResultXAxisUnit());

  addInputDataToSimultaneousFitWithEqualRange(fitAlgorithm,
                                              m_fitDataModel.get());
  fitAlgorithm->setProperty("OutputWorkspace", simultaneousFitOutputName());
  return fitAlgorithm;
}

std::string
IndirectFittingModel::createSingleFitOutputName(const std::string &formatString,
                                                TableDatasetIndex index,
                                                WorkspaceIndex spectrum) const {
  // if (m_fittingData.size() > index)
  //   return m_fittingData[index]->displayName(formatString, spectrum);
  // else
  //   throw std::runtime_error("Cannot create a display name for a workspace:
  //   "
  //                            "the workspace index provided is too large.");
  return "SingleFitOutputName";
}

std::string IndirectFittingModel::getOutputBasename() const {
  return cutLastOf(sequentialFitOutputName(), "_Results");
}

void IndirectFittingModel::cleanFailedRun(
    const IAlgorithm_sptr &fittingAlgorithm) {
  // cleanTemporaries(fittingAlgorithm->name(), m_fittingData);
}

void IndirectFittingModel::cleanFailedSingleRun(
    const IAlgorithm_sptr &fittingAlgorithm, TableDatasetIndex index) {
  const auto base =
      "__" + fittingAlgorithm->name() + "_ws" + std::to_string(index.value + 1);
  removeFromADSIfExists(base);
  cleanTemporaries(base + "_0");
}

DataForParameterEstimationCollection
IndirectFittingModel::getDataForParameterEstimation(
    const EstimationDataSelector &selector) const {
  DataForParameterEstimationCollection dataCollection;
  for (auto i = TableDatasetIndex{0}; i < m_fitDataModel->numberOfWorkspaces();
       ++i) {
    auto const ws = m_fitDataModel->getWorkspace(i);
    for (auto spectrum : m_fitDataModel->getSpectra(i)) {
      auto const &x = ws->readX(spectrum.value);
      auto const &y = ws->readY(spectrum.value);
      dataCollection.emplace_back(selector(x, y));
    }
  }
  return dataCollection;
}

std::vector<double> IndirectFittingModel::getQValuesForData() const {
  return m_fitDataModel->getQValuesForData();
}

std::vector<std::pair<std::string, int>>
IndirectFittingModel::getResolutionsForFit() const {
  return std::vector<std::pair<std::string, int>>();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
