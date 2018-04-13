#include "IndirectFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <numeric>
#include <set>

#include <boost/algorithm/string.hpp>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

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

std::vector<double>
excludeRegionsStringToVector(const std::string &excludeRegions) {
  std::vector<std::string> regionStrings;
  boost::split(regionStrings, excludeRegions, boost::is_any_of("\,,-"));
  std::vector<double> regions;
  std::transform(
      regionStrings.begin(), regionStrings.end(), std::back_inserter(regions),
      [](const std::string &str) { return boost::lexical_cast<double>(str); });
  return regions;
}

std::ostringstream &addInputString(IndirectFitData *fitData,
                                   std::ostringstream &stream) {
  const auto &name = fitData->workspace->getName();
  auto addToStream = [&](std::size_t spectrum) {
    stream << name << ",i" << spectrum << ";";
  };
  fitData->applySpectra(addToStream);
  return stream;
}

std::string constructInputString(
    const std::vector<std::unique_ptr<IndirectFitData>> &fittingData) {
  std::ostringstream input;
  for (const auto &fitData : fittingData)
    addInputString(fitData.get(), input);
  return input.str();
}

void addInputDataToSimultaneousFit(IAlgorithm_sptr fitAlgorithm,
                                   MatrixWorkspace_sptr workspace,
                                   std::size_t spectra,
                                   const std::pair<double, double> &xRange,
                                   const std::vector<double> excludeRegions,
                                   const std::string &suffix) {
  fitAlgorithm->setProperty("InputWorkspace" + suffix, workspace);
  fitAlgorithm->setProperty("StartX" + suffix, xRange.first);
  fitAlgorithm->setProperty("EndX" + suffix, xRange.second);
  fitAlgorithm->setProperty("WorkspaceIndex" + suffix, spectra);

  if (!excludeRegions.empty())
    fitAlgorithm->setProperty("Exclude" + suffix, excludeRegions);
}

void addInputDataToSimultaneousFit(
    IAlgorithm_sptr fitAlgorithm,
    const std::unique_ptr<IndirectFitData> &fitData) {
  const auto workspace = fitData->workspace;

  const auto addData = [&](std::size_t i, std::size_t spectrum) {
    const auto suffix = i == 0 ? "" : "_" + std::to_string(i);
    addInputDataToSimultaneousFit(
        fitAlgorithm, workspace, spectrum, fitData->range(spectrum),
        fitData->excludeRegionsVector(spectrum), suffix);
  };
  fitData->applyEnumeratedSpectra(addData);
}

void addInputDataToSimultaneousFit(
    IAlgorithm_sptr fitAlgorithm,
    const std::vector<std::unique_ptr<IndirectFitData>> &fittingData) {
  for (auto i = 0u; i < fittingData.size(); ++i)
    addInputDataToSimultaneousFit(fitAlgorithm, fittingData[i]);
}

IAlgorithm_sptr saveNexusProcessedAlgorithm(Workspace_sptr workspace,
                                            const std::string &filename) {
  IAlgorithm_sptr saveAlg =
      AlgorithmManager::Instance().create("SaveNexusProcessed");
  saveAlg->setProperty("InputWorkspace", workspace);
  saveAlg->setProperty("Filename", filename);
  return saveAlg;
}

template <typename Map> Map combine(const Map &mapA, const Map &mapB) {
  Map combined(mapA);
  for (const auto it : mapB)
    combined[it.first] = it.second;
  return combined;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFittingModel::IndirectFittingModel() : m_previousModelSelected(false) {}

MatrixWorkspace_sptr
IndirectFittingModel::getWorkspace(std::size_t index) const {
  return m_fittingData[index]->workspace();
}

const Spectra &IndirectFittingModel::getSpectra(std::size_t index) const {
  return m_fittingData[index]->spectra();
}

std::string IndirectFittingModel::getExcludeRegion(std::size_t index) const {
  return m_fittingData[index]->excludeRegionString(index);
}

std::vector<std::string> IndirectFittingModel::inputDisplayNames(
    const std::string &formatString, const std::string &rangeDelimiter) const {
  std::vector<std::string> displayNames;
  for (const auto &fitData : m_fittingData)
    displayNames.emplace_back(
        fitData->displayName(formatString, rangeDelimiter));
  return displayNames;
}

std::string
IndirectFittingModel::inputDisplayName(const std::string &formatString,
                                       const std::string &rangeDelimiter,
                                       std::size_t dataIndex) const {
  return m_fittingData[dataIndex]->displayName(formatString, rangeDelimiter);
}

bool IndirectFittingModel::isMultiFit() const {
  return numberOfWorkspaces() > 1;
}

bool IndirectFittingModel::isPreviousFitSelected() const {
  return m_previousModelSelected;
}

std::size_t IndirectFittingModel::numberOfWorkspaces() const {
  return m_fittingData.size();
}

std::size_t IndirectFittingModel::numberOfSpectra() const {
  std::size_t total = 0u;
  for (const auto &fitData : m_fittingData)
    total += fitData->numberOfSpectra();
  return total;
}

std::vector<std::string> IndirectFittingModel::getFitParameterNames() const {
  if(m_fitFunction)
    return m_fitFunction->getParameterNames();
  return std::vector<std::string>();
}

Mantid::API::IFunction_sptr IndirectFittingModel::getFittingFunction() const {
  return m_activeFunction;
}

void IndirectFittingModel::setSpectra(const Spectra &spectra,
                                      std::size_t dataIndex) {
  m_fittingData[dataIndex]->setSpectra(spectra);
}

void IndirectFittingModel::setExcludeRegion(const std::string &exclude,
                                            std::size_t dataIndex,
                                            std::size_t index) {
  m_fittingData[dataIndex]->setExcludeRegionString(index, exclude);
}

void IndirectFittingModel::addWorkspace(const std::string &workspaceName) {
  auto workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
  addWorkspace(workspace, std::make_pair(0u, workspace->getNumberHistograms()));
}

void IndirectFittingModel::addWorkspace(MatrixWorkspace_sptr workspace,
                                        const Spectra &spectra) {
  if (workspace == m_fittingData.back()->workspace())
    m_fittingData.back()->combine(IndirectFitData(workspace, spectra));
  else {
    m_fittingData.emplace_back(new IndirectFitData(workspace, spectra));
    m_defaultParameters.emplace_back(
        getDefaultParameters(m_fittingData.size() - 1));
  }
}

void IndirectFittingModel::removeWorkspace(std::size_t index) {
  m_fittingData.erase(m_fittingData.begin() + index);
  m_defaultParameters.erase(m_defaultParameters.begin() + index);
}

void IndirectFittingModel::clearWorkspaces() { m_fittingData.clear(); }

void IndirectFittingModel::setFittingMode(FittingMode mode) {
  m_fittingMode = mode;
}

void IndirectFittingModel::setFitFunction(IFunction_sptr model,
                                          IFunction_sptr background) {
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(model);
  if (composite)
    composite->addFunction(background);
  else {
    composite = CompositeFunction_sptr(new CompositeFunction);
    composite->addFunction(model);
    composite->addFunction(background);
  }
  setFitFunction(composite);
}

void IndirectFittingModel::setFitFunction(IFunction_sptr function) {
  m_activeFunction = function;
  m_previousModelSelected = isPreviousModelSelected();
}

void IndirectFittingModel::setDefaultParameterValue(const std::string &name,
                                                    double value,
                                                    std::size_t dataIndex) {
  if (m_defaultParameters.size() > dataIndex)
    m_defaultParameters[dataIndex][name] = ParameterValue(value);
}

void IndirectFittingModel::addOutput(const std::string &outputBaseName) {
  const auto &ads = AnalysisDataService::Instance();
  const auto groupName = outputBaseName + "_Workspaces";
  const auto tableName = outputBaseName + "_Parameters";
  const auto resultName = outputBaseName + "_Result";

  if (ads.doesExist(groupName) && ads.doesExist(tableName) &&
      ads.doesExist(resultName))
    addOutput(ads.retrieveWS<WorkspaceGroup>(groupName),
              ads.retrieveWS<ITableWorkspace>(tableName),
              ads.retrieveWS<MatrixWorkspace>(resultName));
}

void IndirectFittingModel::addOutput(WorkspaceGroup_sptr resultGroup,
                                     ITableWorkspace_sptr parameterTable,
                                     MatrixWorkspace_sptr resultWorkspace) {
  if (m_previousModelSelected)
    addOutput(m_fitOutput.get(), resultGroup, parameterTable, resultWorkspace,
              m_fittingData);
  else
    *m_fitOutput =
        createFitOutput(resultGroup, parameterTable, resultWorkspace);
}

IndirectFitOutput IndirectFittingModel::createFitOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    MatrixWorkspace_sptr resultWorkspace) const {
  return createFitOutput(resultGroup, parameterTable, resultWorkspace,
                         m_fittingData);
}

IndirectFitOutput IndirectFittingModel::createFitOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &m_fittingData) const {
  return IndirectFitOutput(resultGroup, parameterTable, resultWorkspace,
                           m_fittingData);
}

void IndirectFittingModel::addOutput(
    IndirectFitOutput *fitOutput, WorkspaceGroup_sptr resultGroup,
    ITableWorkspace_sptr parameterTable, MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &m_fittingData) const {
  fitOutput->addOutput(resultGroup, parameterTable, resultWorkspace,
                       m_fittingData);
}

FittingMode IndirectFittingModel::fittingMode() const { return m_fittingMode; }

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getParameterValues(std::size_t index,
                                         std::size_t spectrum) const {
  if (m_previousModelSelected)
    return getFitParameters(index, spectrum);
  return combine(m_defaultParameters[index], getFitParameters(index, spectrum));
}

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getFitParameters(std::size_t index,
                                       std::size_t spectrum) const {
  return m_fitOutput->getParameters(m_fittingData[index].get(), spectrum);
}

std::unordered_map<std::string, ParameterValue>
IndirectFittingModel::getDefaultParameters(std::size_t) const {
  return std::unordered_map<std::string, ParameterValue>();
}

boost::optional<ResultLocation>
IndirectFittingModel::getResultLocation(std::size_t index,
                                        std::size_t spectrum) const {
  if (m_previousModelSelected)
    return m_fitOutput->getResultLocation(m_fittingData[index].get(), spectrum);
  return boost::none;
}

void IndirectFittingModel::saveResult() const {
  const auto resultWorkspace = getResultWorkspace();

  if (resultWorkspace) {
    const auto filename = Mantid::Kernel::ConfigService::Instance().getString(
                              "defaultsave.directory") +
                          resultWorkspace->getName() + ".nxs";
    saveNexusProcessedAlgorithm(resultWorkspace, filename)->execute();
  }
}

MatrixWorkspace_sptr IndirectFittingModel::getResultWorkspace() const {
  return m_fitOutput->getLastResultWorkspace();
}

WorkspaceGroup_sptr IndirectFittingModel::getResultGroup() const {
  return m_fitOutput->getLastResultGroup();
}

bool IndirectFittingModel::isPreviousModelSelected() const {
  return m_fitFunction && equivalentFunctions(m_activeFunction, m_fitFunction);
}

IAlgorithm_sptr IndirectFittingModel::getFittingAlgorithm() const {
  if (m_fittingMode == FittingMode::SEQUENTIAL)
    return createSequentialFit(m_activeFunction);
  else
    return createSimultaneousFit(m_activeFunction);
}

Mantid::API::IAlgorithm_sptr
IndirectFittingModel::getSingleFitAlgorithm(std::size_t dataIndex,
                                            std::size_t spectrum) const {
  const auto &fitData = m_fittingData[dataIndex];
  std::ostringstream input;
  input << fitData->workspace()->getName() << ",i"
        << std::to_string(fitData->getSpectrum(spectrum)) << ";";
  return createSequentialFit(m_activeFunction, input.str(), fitData.get());
}

IAlgorithm_sptr
IndirectFittingModel::createSequentialFit(IFunction_sptr function) const {
  const auto input = constructInputString(m_fittingData);
  return createSequentialFit(function, input, m_fittingData.front().get());
}

IAlgorithm_sptr IndirectFittingModel::createSequentialFit(
    IFunction_sptr function, const std::string &input,
    IndirectFitData *initialFitData) const {
  auto fitAlgorithm = sequentialFitAlgorithm();
  fitAlgorithm->setProperty("Input", input);
  fitAlgorithm->setProperty("OutputWorkspace", sequentialFitOutputName());
  fitAlgorithm->setProperty("Function", function);

  auto spectrum = initialFitData->firstSpectrum();
  auto xRange = initialFitData->range(spectrum);
  fitAlgorithm->setProperty("StartX", xRange.first);
  fitAlgorithm->setProperty("EndX", xRange.second);

  auto excludeRegion = initialFitData->excludeRegionsVector(spectrum);
  if (!excludeRegion.empty())
    fitAlgorithm->setProperty("Exclude", excludeRegion);

  return fitAlgorithm;
}

IAlgorithm_sptr
IndirectFittingModel::createSimultaneousFit(IFunction_sptr function) const {
  auto fitAlgorithm = simultaneousFitAlgorithm();
  addInputDataToSimultaneousFit(fitAlgorithm, m_fittingData);
  fitAlgorithm->setProperty("Function", function);
  fitAlgorithm->setProperty("Output", simultaneousFitOutputName());
  return fitAlgorithm;
}
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
