#include "IndirectFitOutput.h"

#include "MantidAPI/TableRow.h"

using namespace Mantid::API;

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

std::unordered_map<std::string, ParameterValue>
extractRowFromTable(ITableWorkspace_sptr tableWs, std::size_t index) {
  TableRow row = tableWs->getRow(index);
  const auto columnNames = tableWs->getColumnNames();
  auto chiIndex =
      std::find(columnNames.begin(), columnNames.end(), "Chi_squared") -
      columnNames.begin();
  std::unordered_map<std::string, ParameterValue> parameters;

  do {
    for (auto i = 1u; i < chiIndex; i += 2) {
      const auto &columnName = columnNames[i];
      parameters[columnName] = ParameterValue(row.Double(i), row.Double(i + 1));
    }
  } while (row.next());
  return parameters;
}

template <typename F>
void applyEnumeratedData(
    const F &functor,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData) {
  std::size_t start = 0;
  for (const auto &inputData : fitData)
    start = inputData->applyEnumeratedSpectra(functor(inputData.get()), start);
}

void extractParametersFromTable(ITableWorkspace_sptr tableWs,
                                IndirectFitData *fitData,
                                ParameterValues &values) {
  auto extractRow = [&](std::size_t index, std::size_t spectrum) {
    values[spectrum] = extractRowFromTable(tableWs, index);
  };
  fitData->applyEnumeratedSpectra(extractRow);
}

void extractParametersFromTable(
    ITableWorkspace_sptr tableWs,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData,
    std::unordered_map<IndirectFitData *, ParameterValues> &parameters) {
  auto extract = [&](IndirectFitData *inputData) {
    parameters[inputData] = ParameterValues();
    auto &values = parameters[inputData];
    return [&](std::size_t index, std::size_t spectrum) {
      values[spectrum] = extractRowFromTable(tableWs, index);
    };
  };
  applyEnumeratedData(extract, fitData);
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
    newMap[unmapped.first] = unmapped.second;
  }
  return newMap;
}

template <typename Map2D, typename KeyMap>
void mapInnerKeys(Map2D &map, const KeyMap &keyMap) {
  for (const auto it : map)
    map[it.first] = mapKeys(it.second, keyMap);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutput::IndirectFitOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData)
    : m_resultWorkspace(resultWorkspace) {
  addOutput(resultGroup, parameterTable, resultWorkspace, fitData);
}

IndirectFitOutput::IndirectFitOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData,
    const std::unordered_map<std::string, std::string> &parameterNameChanges)
    : m_resultWorkspace(resultWorkspace) {
  addOutput(resultGroup, parameterTable, resultWorkspace, fitData,
            parameterNameChanges);
}

std::unordered_map<std::string, ParameterValue>
IndirectFitOutput::getParameters(IndirectFitData *fitData,
                                 std::size_t spectra) const {
  return findOr(m_parameters, fitData, spectra,
                std::unordered_map<std::string, ParameterValue>());
}

boost::optional<ResultLocation>
IndirectFitOutput::getResultLocation(IndirectFitData *fitData,
                                     std::size_t spectra) const {
  return findOr(m_outputResultLocations, fitData, spectra,
                boost::optional<ResultLocation>(boost::none));
}

MatrixWorkspace_sptr IndirectFitOutput::getLastResultWorkspace() const {
  return m_resultWorkspace.lock();
}

WorkspaceGroup_sptr IndirectFitOutput::getLastResultGroup() const {
  return m_resultGroup.lock();
}

void IndirectFitOutput::addOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData) {
  updateParameters(parameterTable, fitData);
  updateFitResults(resultGroup, fitData);
  m_resultWorkspace = resultWorkspace;
}

void IndirectFitOutput::addOutput(
    WorkspaceGroup_sptr resultGroup, ITableWorkspace_sptr parameterTable,
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData,
    const std::unordered_map<std::string, std::string> parameterNameChanges) {
  updateParameters(parameterTable, fitData, parameterNameChanges);
  updateFitResults(resultGroup, fitData);
  m_resultWorkspace = resultWorkspace;
}

void IndirectFitOutput::updateParameters(
    ITableWorkspace_sptr parameterTable,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData) {
  extractParametersFromTable(parameterTable, fitData, m_parameters);
}

void IndirectFitOutput::updateParameters(
    ITableWorkspace_sptr parameterTable,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData,
    const std::unordered_map<std::string, std::string> &parameterNameChanges) {
  extractParametersFromTable(parameterTable, fitData, m_parameters);

  for (auto parameters : m_parameters)
    mapInnerKeys(parameters.second, parameterNameChanges);
}

void IndirectFitOutput::updateFitResults(
    Mantid::API::WorkspaceGroup_sptr resultGroup,
    const std::vector<std::unique_ptr<IndirectFitData>> &fitData) {
  auto update = [&](IndirectFitData *inputData) {
    auto &fitResults = m_outputResultLocations[inputData];
    return [&](std::size_t index, std::size_t spectrum) {
      fitResults[spectrum] = ResultLocation(resultGroup, index++);
    };
  };
  applyEnumeratedData(update, fitData);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
