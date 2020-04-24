// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <memory>

#include "DllConfig.h"
#include "Indextypes.h"
#include "IndirectFitOutputModel.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"

#include "MantidAPI/WorkspaceGroup.h"

namespace {
using namespace MantidQt::CustomInterfaces::IDA;

struct TableRowExtractor {
  explicit TableRowExtractor(Mantid::API::ITableWorkspace_sptr table)
      : m_table(std::move(table)), m_columns(m_table->getColumnNames()) {
    m_chiIndex = std::find(m_columns.begin(), m_columns.end(), "Chi_squared") -
                 m_columns.begin();
  }

  std::unordered_map<std::string,
                     MantidQt::CustomInterfaces::IDA::ParameterValue>
  operator()(size_t index) {
    Mantid::API::TableRow row = m_table->getRow(index);
    std::unordered_map<std::string, ParameterValue> parameters;

    for (auto i = 1u; i < m_chiIndex; i += 2) {
      const auto &columnName = m_columns[i];
      parameters[columnName] = ParameterValue(row.Double(i), row.Double(i + 1));
    }
    return parameters;
  }

private:
  Mantid::API::ITableWorkspace_sptr m_table;
  const std::vector<std::string> m_columns;
  std::size_t m_chiIndex;
};

Mantid::API::MatrixWorkspace_sptr
getMatrixWorkspaceFromGroup(const Mantid::API::WorkspaceGroup_sptr &group,
                            std::size_t index) {
  if (group->size() > index)
    return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        group->getItem(index));
  return nullptr;
}

std::vector<std::string> getAxisLabels(Mantid::API::TextAxis const *axis) {
  std::vector<std::string> labels;
  labels.reserve(axis->length());
  for (auto i = 0u; i < axis->length(); ++i)
    labels.emplace_back(axis->label(i));
  return labels;
}

std::vector<std::string>
getAxisLabels(const Mantid::API::MatrixWorkspace_sptr &workspace,
              std::size_t index) {
  auto axis = dynamic_cast<Mantid::API::TextAxis *>(workspace->getAxis(index));
  if (axis)
    return getAxisLabels(axis);
  return std::vector<std::string>();
}

std::vector<std::unordered_map<std::string,
                               MantidQt::CustomInterfaces::IDA::ParameterValue>>
extractParametersFromTable(Mantid::API::ITableWorkspace_sptr tableWs) {
  TableRowExtractor extractRowFromTable(std::move(tableWs));
  std::vector<std::unordered_map<std::string, ParameterValue>> parameterMap;
  for (size_t rowIndex = 0; rowIndex < tableWs->rowCount(); ++rowIndex) {
    parameterMap.emplace_back(extractRowFromTable(rowIndex));
  }
  return parameterMap;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitOutputModel::IndirectFitOutputModel(){};

bool IndirectFitOutputModel::isSpectrumFit(FitDomainIndex index) const {
  return index.value < m_parameters.size();
}

std::unordered_map<std::string, ParameterValue>
IndirectFitOutputModel::getParameters(FitDomainIndex index) const {
  return m_parameters.at(index.value);
}

boost::optional<ResultLocationNew>
IndirectFitOutputModel::getResultLocation(FitDomainIndex index) const {
  return ResultLocationNew(m_resultWorkspace.lock(),
                           WorkspaceGroupIndex{index.value});
}

std::vector<std::string>
IndirectFitOutputModel::getResultParameterNames() const {
  if (auto resultWorkspace = getLastResultWorkspace()) {
    if (auto workspace = getMatrixWorkspaceFromGroup(resultWorkspace, 0)) {
      return getAxisLabels(workspace, 1);
    }
  }
  return std::vector<std::string>();
}
Mantid::API::WorkspaceGroup_sptr
IndirectFitOutputModel::getLastResultWorkspace() const {
  return m_resultWorkspace.lock();
}
Mantid::API::WorkspaceGroup_sptr
IndirectFitOutputModel::getLastResultGroup() const {
  return m_resultGroup.lock();
}

void IndirectFitOutputModel::clear() {
  m_resultGroup.reset();
  m_resultWorkspace.reset();
  m_parameters.clear();
}

void IndirectFitOutputModel::addOutput(
    const Mantid::API::WorkspaceGroup_sptr &resultGroup,
    Mantid::API::ITableWorkspace_sptr parameterTable,
    const Mantid::API::WorkspaceGroup_sptr &resultWorkspace) {
  m_parameters = extractParametersFromTable(parameterTable);
  m_resultGroup = resultGroup;
  m_resultWorkspace = resultWorkspace;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
