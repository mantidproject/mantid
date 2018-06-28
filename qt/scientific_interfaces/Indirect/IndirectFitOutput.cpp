#include "IndirectFitOutput.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

struct TableRowExtractor {
  explicit TableRowExtractor(ITableWorkspace_sptr table)
      : m_table(table), m_columns(m_table->getColumnNames()) {
    m_chiIndex = std::find(m_columns.begin(), m_columns.end(), "Chi_squared") -
                 m_columns.begin();
  }

  std::unordered_map<std::string, ParameterValue>
  operator()(std::size_t index) {
    TableRow row = m_table->getRow(index);
    std::unordered_map<std::string, ParameterValue> parameters;

    for (auto i = 1u; i < m_chiIndex; i += 2) {
      const auto &columnName = m_columns[i];
      parameters[columnName] = ParameterValue(row.Double(i), row.Double(i + 1));
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
void applyEnumeratedData(const F &functor, const FitDataIterator &fitDataBegin,
                         const FitDataIterator &fitDataEnd) {
  std::size_t start = 0;
  for (auto it = fitDataBegin; it < fitDataEnd; ++it)
    start = (*it)->applyEnumeratedSpectra(functor(it->get()), start);
}

void extractParametersFromTable(
    ITableWorkspace_sptr tableWs, const FitDataIterator &fitDataBegin,
    const FitDataIterator &fitDataEnd,
    std::unordered_map<IndirectFitData const *, ParameterValues> &parameters) {
  TableRowExtractor extractRowFromTable(tableWs);
  auto extract = [&](IndirectFitData const *inputData) {
    auto &values = extractOrAddDefault(parameters, inputData);
    return [&](std::size_t index, std::size_t spectrum) {
      values[spectrum] = extractRowFromTable(index);
    };
  };
  applyEnumeratedData(extract, fitDataBegin, fitDataEnd);
}

template <typename Map2D, typename T, typename U, typename Value>
Value findOr(const Map2D &map, const T &firstKey, const U &secondKey,
             const Value &defaultValue) {
  const auto values = map.find(firstKey);
  if (values != map.end()) {
    const auto value = values->second.find(secondKey);
    if (value != values->second.end())
      return value->second;
  }
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

void renameResult(Workspace_sptr resultWorkspace,
                  IndirectFitData const *fitData) {
  const auto name = resultWorkspace->getName();
  const auto newName = fitData->displayName("%1%_s%2%_Result", "_to_");
  AnalysisDataService::Instance().rename(name, newName);
}

void renameResult(WorkspaceGroup_sptr resultWorkspace,
                  const FitDataIterator &fitDataBegin,
                  const FitDataIterator &fitDataEnd) {
  std::size_t index = 0;
  for (auto it = fitDataBegin; it < fitDataEnd; ++it)
    renameResult(resultWorkspace->getItem(index++), it->get());
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutput::IndirectFitOutput(WorkspaceGroup_sptr resultGroup,
                                     ITableWorkspace_sptr parameterTable,
                                     WorkspaceGroup_sptr resultWorkspace,
                                     const FitDataIterator &fitDataBegin,
                                     const FitDataIterator &fitDataEnd)
    : m_resultGroup(resultGroup), m_resultWorkspace(resultWorkspace),
      m_parameters(), m_outputResultLocations() {
  addOutput(resultGroup, parameterTable, resultWorkspace, fitDataBegin,
            fitDataEnd);
}

IndirectFitOutput::IndirectFitOutput(WorkspaceGroup_sptr resultGroup,
                                     ITableWorkspace_sptr parameterTable,
                                     WorkspaceGroup_sptr resultWorkspace,
                                     IndirectFitData const *fitData,
                                     std::size_t spectrum) {
  m_parameters[fitData] = ParameterValues();
  m_outputResultLocations[fitData] = ResultLocations();
  addOutput(resultGroup, parameterTable, resultWorkspace, fitData, spectrum);
}

bool IndirectFitOutput::isSpectrumFit(IndirectFitData const *fitData,
                                      std::size_t spectrum) const {
  auto values = m_parameters.find(fitData);
  return values != m_parameters.end() &&
         values->second.find(spectrum) != values->second.end();
}

std::unordered_map<std::string, ParameterValue>
IndirectFitOutput::getParameters(IndirectFitData const *fitData,
                                 std::size_t spectrum) const {
  return findOr(m_parameters, fitData, spectrum,
                std::unordered_map<std::string, ParameterValue>());
}

boost::optional<ResultLocation>
IndirectFitOutput::getResultLocation(IndirectFitData *fitData,
                                     std::size_t spectrum) const {
  return findOr(m_outputResultLocations, fitData, spectrum,
                boost::optional<ResultLocation>(boost::none));
}

std::vector<std::string> IndirectFitOutput::getResultParameterNames() const {
  if (auto resultWorkspace = getLastResultWorkspace()) {
    if (auto workspace = getMatrixWorkspaceFromGroup(resultWorkspace, 0))
      return getAxisLabels(workspace, 1);
  }
  return std::vector<std::string>();
}

WorkspaceGroup_sptr IndirectFitOutput::getLastResultWorkspace() const {
  return m_resultWorkspace.lock();
}

WorkspaceGroup_sptr IndirectFitOutput::getLastResultGroup() const {
  return m_resultGroup.lock();
}

void IndirectFitOutput::mapParameterNames(
    const std::unordered_map<std::string, std::string> &parameterNameChanges,
    const FitDataIterator &fitDataBegin, const FitDataIterator &fitDataEnd) {
  for (auto it = fitDataBegin; it < fitDataEnd; ++it) {
    auto &parameterValues = m_parameters[it->get()];
    for (const auto &values : parameterValues)
      parameterValues[values.first] =
          mapKeys(values.second, parameterNameChanges);
  }
}

void IndirectFitOutput::mapParameterNames(
    const std::unordered_map<std::string, std::string> &parameterNameChanges,
    IndirectFitData const *fitData, std::size_t spectrum) {
  auto &parameters = m_parameters[fitData][spectrum];
  parameters = mapKeys(parameters, parameterNameChanges);
}

void IndirectFitOutput::addOutput(WorkspaceGroup_sptr resultGroup,
                                  ITableWorkspace_sptr parameterTable,
                                  WorkspaceGroup_sptr resultWorkspace,
                                  const FitDataIterator &fitDataBegin,
                                  const FitDataIterator &fitDataEnd) {
  updateParameters(parameterTable, fitDataBegin, fitDataEnd);
  updateFitResults(resultGroup, fitDataBegin, fitDataEnd);
  renameResult(resultWorkspace, fitDataBegin, fitDataEnd);
  m_resultWorkspace = resultWorkspace;
  m_resultGroup = resultGroup;
}

void IndirectFitOutput::addOutput(
    Mantid::API::WorkspaceGroup_sptr resultGroup,
    Mantid::API::ITableWorkspace_sptr parameterTable,
    Mantid::API::WorkspaceGroup_sptr resultWorkspace,
    IndirectFitData const *fitData, std::size_t spectrum) {
  TableRowExtractor extractRowFromTable(parameterTable);
  m_parameters[fitData][spectrum] = extractRowFromTable(0);
  m_outputResultLocations[fitData][spectrum] = ResultLocation(resultGroup, 0);
  renameResult(resultWorkspace, fitData);
  m_resultWorkspace = resultWorkspace;
  m_resultGroup = resultGroup;
}

void IndirectFitOutput::removeOutput(IndirectFitData const *fitData) {
  m_parameters.erase(fitData);
  m_outputResultLocations.erase(fitData);
}

void IndirectFitOutput::updateParameters(ITableWorkspace_sptr parameterTable,
                                         const FitDataIterator &fitDataBegin,
                                         const FitDataIterator &fitDataEnd) {
  extractParametersFromTable(parameterTable, fitDataBegin, fitDataEnd,
                             m_parameters);
}

void IndirectFitOutput::updateFitResults(
    Mantid::API::WorkspaceGroup_sptr resultGroup,
    const FitDataIterator &fitDataBegin, const FitDataIterator &fitDataEnd) {
  auto update = [&](IndirectFitData const *inputData) {
    auto &fitResults = extractOrAddDefault(m_outputResultLocations, inputData);
    return [&](std::size_t index, std::size_t spectrum) {
      fitResults[spectrum] = ResultLocation(resultGroup, index++);
    };
  };
  applyEnumeratedData(update, fitDataBegin, fitDataEnd);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
