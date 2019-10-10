// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputLegacy.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"

#include <boost/functional/hash.hpp>

#include <unordered_set>

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

struct TableRowExtractor {
  explicit TableRowExtractor(ITableWorkspace_sptr table)
      : m_table(table), m_columns(m_table->getColumnNames()) {
    m_chiIndex = std::find(m_columns.begin(), m_columns.end(), "Chi_squared") -
                 m_columns.begin();
  }

  std::unordered_map<std::string, ParameterValueLegacy>
  operator()(std::size_t index) {
    TableRow row = m_table->getRow(index);
    std::unordered_map<std::string, ParameterValueLegacy> parameters;

    for (auto i = 1u; i < m_chiIndex; i += 2) {
      const auto &columnName = m_columns[i];
      parameters[columnName] = ParameterValueLegacy(row.Double(i), row.Double(i + 1));
    }
    return parameters;
  }

private:
  ITableWorkspace_sptr m_table;
  const std::vector<std::string> m_columns;
  std::size_t m_chiIndex;
};

template <typename Map, typename Key>
typename Map::mapped_type &extractOrAddDefault(Map &map, const Key &key) {
  auto values = map.find(key);
  if (values != map.end())
    return values->second;
  return map[key] = typename Map::mapped_type();
}

template <typename F>
void applyEnumeratedData(F &&functor, const FitDataIteratorLegacy &fitDataBegin,
                         const FitDataIteratorLegacy &fitDataEnd) {
  std::size_t start = 0;
  for (auto it = fitDataBegin; it < fitDataEnd; ++it)
    start = (*it)->applyEnumeratedSpectraLegacy(functor(it->get()), start);
}

template <typename F>
void applyData(F &&functor, const FitDataIteratorLegacy &fitDataBegin,
               const FitDataIteratorLegacy &fitDataEnd) {
  for (auto it = fitDataBegin; it < fitDataEnd; ++it)
    (*it)->applySpectra(functor(it->get()));
}

void extractParametersFromTable(
    ITableWorkspace_sptr tableWs, const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd,
    std::unordered_map<IndirectFitDataLegacy const *, ParameterValues>
        &parameters) {
  TableRowExtractor extractRowFromTable(tableWs);
  auto extract = [&](IndirectFitDataLegacy const *inputData) {
    auto &values = extractOrAddDefault(parameters, inputData);
    return [&](std::size_t index, std::size_t spectrum) {
      values[spectrum] = extractRowFromTable(index);
    };
  };
  applyEnumeratedData(extract, fitDataBegin, fitDataEnd);
}

template <typename Map, typename Value, typename Key>
Value getValueOr(const Map &map, const Value &defaultValue, const Key &key) {
  const auto value = map.find(key);
  if (value != map.end())
    return value->second;
  return defaultValue;
}

template <typename MapND, typename Value, typename Key, typename... Keys>
Value getValueOr(const MapND &map, const Value &defaultValue, const Key &key,
                 const Keys &... keys) {
  const auto values = map.find(key);
  if (values != map.end())
    return getValueOr(values->second, defaultValue, keys...);
  return defaultValue;
}

template <typename Map, typename KeyMap>
Map mapKeys(const Map &map, const KeyMap &keyMap) {
  Map newMap;

  for (const auto unmapped : map) {
    const auto mapping = keyMap.find(unmapped.first);
    if (mapping != keyMap.end())
      newMap[mapping->second] = unmapped.second;
    else
      newMap[unmapped.first] = unmapped.second;
  }
  return newMap;
}

MatrixWorkspace_sptr getMatrixWorkspaceFromGroup(WorkspaceGroup_sptr group,
                                                 std::size_t index) {
  if (group->size() > index)
    return boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(index));
  return nullptr;
}

std::vector<std::string> getAxisLabels(TextAxis const *axis) {
  std::vector<std::string> labels;
  labels.reserve(axis->length());
  for (auto i = 0u; i < axis->length(); ++i)
    labels.emplace_back(axis->label(i));
  return labels;
}

std::vector<std::string> getAxisLabels(MatrixWorkspace_sptr workspace,
                                       std::size_t index) {
  auto axis = dynamic_cast<TextAxis *>(workspace->getAxis(index));
  if (axis)
    return getAxisLabels(axis);
  return std::vector<std::string>();
}

std::string cutLastOf(const std::string &str, const std::string &delimiter) {
  const auto cutIndex = str.rfind(delimiter);
  if (cutIndex != std::string::npos)
    return str.substr(0, cutIndex);
  return str;
}

bool containsMultipleData(const std::string &name) {
  return name.substr(0, 5) == "Multi";
}

std::string constructResultName(const std::string &name,
                                IndirectFitDataLegacy const *fitData) {
  if (containsMultipleData(name)) {
    const auto formatString = cutLastOf(name, "_Results") + "_%1%_s%2%_Result";
    return fitData->displayName(formatString, "_to_");
  } else
    return cutLastOf(name, "s_1");
}

void renameWorkspace(std::string const &name, std::string const &newName) {
  auto renamer = AlgorithmManager::Instance().create("RenameWorkspace");
  renamer->setProperty("InputWorkspace", name);
  renamer->setProperty("OutputWorkspace", newName);
  renamer->execute();
}

void renameResult(Workspace_sptr resultWorkspace,
                  const std::string &workspaceName) {
  renameWorkspace(resultWorkspace->getName(), workspaceName + "_Result");
}

void renameResult(Workspace_sptr resultWorkspace,
                  IndirectFitDataLegacy const *fitData) {
  const auto name = resultWorkspace->getName();
  const auto newName = constructResultName(name, fitData);
  if (newName != name)
    renameWorkspace(name, newName);
}

void renameResult(WorkspaceGroup_sptr resultWorkspace,
                  IndirectFitDataLegacy const *fitData) {
  for (auto const &workspace : *resultWorkspace)
    renameResult(workspace, fitData);
}

void renameResultWithoutSpectra(WorkspaceGroup_sptr resultWorkspace,
                                const FitDataIteratorLegacy &fitDataBegin,
                                const FitDataIteratorLegacy &fitDataEnd) {
  std::size_t index = 0;
  MatrixWorkspace const *previous = nullptr;

  for (auto it = fitDataBegin; it < fitDataEnd; ++it) {
    auto workspace = (*it)->workspace().get();
    if (workspace != previous) {
      renameResult(resultWorkspace->getItem(index++), workspace->getName());
      previous = workspace;
    }
  }
}

void renameResultWithSpectra(WorkspaceGroup_sptr resultWorkspace,
                             const FitDataIteratorLegacy &fitDataBegin,
                             const FitDataIteratorLegacy &fitDataEnd) {
  std::size_t index = 0;
  for (auto it = fitDataBegin; it < fitDataEnd; ++it)
    renameResult(resultWorkspace->getItem(index++), it->get());
}

void renameResult(WorkspaceGroup_sptr resultWorkspace,
                  const FitDataIteratorLegacy &fitDataBegin,
                  const FitDataIteratorLegacy &fitDataEnd) {
  if (static_cast<int>(resultWorkspace->size()) >= fitDataEnd - fitDataBegin)
    renameResultWithSpectra(resultWorkspace, fitDataBegin, fitDataEnd);
  else
    renameResultWithoutSpectra(resultWorkspace, fitDataBegin, fitDataEnd);
}

template <typename Map, typename Key>
typename Map::mapped_type &findOrCreateDefaultInMap(Map &map, const Key &key) {
  auto valueIt = map.find(key);
  if (valueIt != map.end())
    return valueIt->second;
  return map[key] = typename Map::mapped_type();
}

struct UnstructuredResultAdder {
public:
  UnstructuredResultAdder(
      WorkspaceGroup_sptr resultGroup, ResultLocations &locations,
      std::unordered_map<std::size_t, std::size_t> &defaultPositions,
      std::size_t &index)
      : m_resultGroup(resultGroup), m_locations(locations),
        m_defaultPositions(defaultPositions), m_index(index) {}

  void operator()(std::size_t spectrum) const {
    auto defaultIt = m_defaultPositions.find(spectrum);
    if (defaultIt != m_defaultPositions.end())
      m_locations[spectrum] = ResultLocation(m_resultGroup, defaultIt->second);
    else if (m_resultGroup->size() > m_index) {
      m_locations[spectrum] = ResultLocation(m_resultGroup, m_index);
      m_defaultPositions[spectrum] = m_index++;
    }
  }

private:
  WorkspaceGroup_sptr m_resultGroup;
  ResultLocations &m_locations;
  std::unordered_map<std::size_t, std::size_t> &m_defaultPositions;
  std::size_t &m_index;
};

std::size_t numberOfSpectraIn(const FitDataIteratorLegacy &fitDataBegin,
                              const FitDataIteratorLegacy &fitDataEnd) {
  std::size_t spectra = 0;
  for (auto it = fitDataBegin; it < fitDataEnd; ++it)
    spectra += (*it)->numberOfSpectra();
  return spectra;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputLegacy::IndirectFitOutputLegacy(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    WorkspaceGroup_sptr resultWorkspace, const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd)
    : m_resultGroup(resultGroup), m_resultWorkspace(resultWorkspace),
      m_parameters(), m_outputResultLocations() {
  addOutput(resultGroup, parameterTable, resultWorkspace, fitDataBegin,
            fitDataEnd);
}

IndirectFitOutputLegacy::IndirectFitOutputLegacy(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    WorkspaceGroup_sptr resultWorkspace, IndirectFitDataLegacy const *fitData,
    std::size_t spectrum) {
  m_parameters[fitData] = ParameterValues();
  m_outputResultLocations[fitData] = ResultLocations();
  addOutput(resultGroup, parameterTable, resultWorkspace, fitData, spectrum);
}

bool IndirectFitOutputLegacy::isSpectrumFit(
    IndirectFitDataLegacy const *fitData, std::size_t spectrum) const {
  auto values = m_parameters.find(fitData);
  return values != m_parameters.end() &&
         values->second.find(spectrum) != values->second.end();
}

std::unordered_map<std::string, ParameterValueLegacy>
IndirectFitOutputLegacy::getParameters(IndirectFitDataLegacy const *fitData,
                                       std::size_t spectrum) const {
  return getValueOr(m_parameters,
                    std::unordered_map<std::string, ParameterValueLegacy>(), fitData,
                    spectrum);
}

boost::optional<ResultLocation>
IndirectFitOutputLegacy::getResultLocation(IndirectFitDataLegacy const *fitData,
                                           std::size_t spectrum) const {
  return getValueOr(m_outputResultLocations,
                    boost::optional<ResultLocation>(boost::none), fitData,
                    spectrum);
}

std::vector<std::string>
IndirectFitOutputLegacy::getResultParameterNames() const {
  if (auto resultWorkspace = getLastResultWorkspace())
    if (auto workspace = getMatrixWorkspaceFromGroup(resultWorkspace, 0))
      return getAxisLabels(workspace, 1);
  return std::vector<std::string>();
}

WorkspaceGroup_sptr IndirectFitOutputLegacy::getLastResultWorkspace() const {
  return m_resultWorkspace.lock();
}

WorkspaceGroup_sptr IndirectFitOutputLegacy::getLastResultGroup() const {
  return m_resultGroup.lock();
}

void IndirectFitOutputLegacy::mapParameterNames(
    const std::unordered_map<std::string, std::string> &parameterNameChanges,
    const FitDataIteratorLegacy &fitDataBegin, const FitDataIteratorLegacy &fitDataEnd) {
  for (auto it = fitDataBegin; it < fitDataEnd; ++it)
    mapParameterNames(parameterNameChanges, it->get());
}

void IndirectFitOutputLegacy::mapParameterNames(
    const std::unordered_map<std::string, std::string> &parameterNameChanges,
    IndirectFitDataLegacy const *fitData) {
  const auto parameterIt = m_parameters.find(fitData);
  if (parameterIt != m_parameters.end()) {
    for (const auto &values : parameterIt->second)
      parameterIt->second[values.first] =
          mapKeys(values.second, parameterNameChanges);
  }
}

void IndirectFitOutputLegacy::mapParameterNames(
    const std::unordered_map<std::string, std::string> &parameterNameChanges,
    IndirectFitDataLegacy const *fitData, std::size_t spectrum) {
  auto &parameters = m_parameters[fitData][spectrum];
  parameters = mapKeys(parameters, parameterNameChanges);
}

void IndirectFitOutputLegacy::addOutput(WorkspaceGroup_sptr resultGroup,
                                        ITableWorkspace_sptr parameterTable,
                                        WorkspaceGroup_sptr resultWorkspace,
                                        const FitDataIteratorLegacy &fitDataBegin,
                                        const FitDataIteratorLegacy &fitDataEnd) {
  updateParameters(parameterTable, fitDataBegin, fitDataEnd);
  updateFitResults(resultGroup, fitDataBegin, fitDataEnd);
  renameResult(resultWorkspace, fitDataBegin, fitDataEnd);
  m_resultWorkspace = resultWorkspace;
  m_resultGroup = resultGroup;
}

void IndirectFitOutputLegacy::addOutput(WorkspaceGroup_sptr resultGroup,
                                        ITableWorkspace_sptr parameterTable,
                                        WorkspaceGroup_sptr resultWorkspace,
                                        IndirectFitDataLegacy const *fitData,
                                        std::size_t spectrum) {
  TableRowExtractor extractRowFromTable(parameterTable);
  m_parameters[fitData][spectrum] = extractRowFromTable(0);
  m_outputResultLocations[fitData][spectrum] = ResultLocation(resultGroup, 0);
  renameResult(resultWorkspace, fitData);
  m_resultWorkspace = resultWorkspace;
  m_resultGroup = resultGroup;
}

void IndirectFitOutputLegacy::removeOutput(
    IndirectFitDataLegacy const *fitData) {
  m_parameters.erase(fitData);
  m_outputResultLocations.erase(fitData);
}

void IndirectFitOutputLegacy::updateFitResults(
    WorkspaceGroup_sptr resultGroup, const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd) {
  if (numberOfSpectraIn(fitDataBegin, fitDataEnd) <= resultGroup->size())
    updateFitResultsFromStructured(resultGroup, fitDataBegin, fitDataEnd);
  else
    updateFitResultsFromUnstructured(resultGroup, fitDataBegin, fitDataEnd);
}

void IndirectFitOutputLegacy::updateParameters(
    ITableWorkspace_sptr parameterTable, const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd) {
  extractParametersFromTable(parameterTable, fitDataBegin, fitDataEnd,
                             m_parameters);
}

void IndirectFitOutputLegacy::updateFitResultsFromUnstructured(
    WorkspaceGroup_sptr resultGroup, const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd) {
  std::unordered_map<MatrixWorkspace *,
                     std::unordered_map<std::size_t, std::size_t>>
      resultIndices;
  std::size_t index = 0;

  auto update = [&](IndirectFitDataLegacy const *inputData) {
    auto &fitResults = extractOrAddDefault(m_outputResultLocations, inputData);
    auto ws = inputData->workspace().get();
    auto &indices = findOrCreateDefaultInMap(resultIndices, ws);
    return UnstructuredResultAdder(resultGroup, fitResults, indices, index);
  };
  applyData(update, fitDataBegin, fitDataEnd);
}

void IndirectFitOutputLegacy::updateFitResultsFromStructured(
    WorkspaceGroup_sptr resultGroup, const FitDataIteratorLegacy &fitDataBegin,
    const FitDataIteratorLegacy &fitDataEnd) {
  auto update = [&](IndirectFitDataLegacy const *inputData) {
    auto &fitResults = extractOrAddDefault(m_outputResultLocations, inputData);
    return [&](std::size_t index, std::size_t spectrum) {
      fitResults[spectrum] = ResultLocation(resultGroup, index);
    };
  };
  applyEnumeratedData(update, fitDataBegin, fitDataEnd);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
