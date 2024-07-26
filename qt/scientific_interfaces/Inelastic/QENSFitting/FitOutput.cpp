// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <memory>

#include "FitOutput.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"

#include "MantidAPI/WorkspaceGroup.h"

using namespace MantidQt::MantidWidgets;

namespace {
using namespace MantidQt::CustomInterfaces::Inelastic;

struct TableRowExtractor {
  explicit TableRowExtractor(Mantid::API::ITableWorkspace_sptr table)
      : m_table(std::move(table)), m_columns(m_table->getColumnNames()) {
    m_chiIndex = std::find(m_columns.begin(), m_columns.end(), "Chi_squared") - m_columns.begin();
  }

  std::unordered_map<std::string, MantidQt::CustomInterfaces::Inelastic::ParameterValue> operator()(size_t index) {
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

Mantid::API::MatrixWorkspace_sptr getMatrixWorkspaceFromGroup(const Mantid::API::WorkspaceGroup_sptr &group,
                                                              std::size_t index) {
  if (group->size() > index)
    return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(group->getItem(index));
  return nullptr;
}

std::vector<std::string> getAxisLabels(Mantid::API::TextAxis const *axis) {
  std::vector<std::string> labels;
  labels.reserve(axis->length());
  for (auto i = 0u; i < axis->length(); ++i)
    labels.emplace_back(axis->label(i));
  return labels;
}

std::vector<std::string> getAxisLabels(const Mantid::API::MatrixWorkspace_sptr &workspace, std::size_t index) {
  if (auto const *axis = dynamic_cast<Mantid::API::TextAxis *>(workspace->getAxis(index)))
    return getAxisLabels(axis);
  return std::vector<std::string>();
}
} // namespace

namespace MantidQt::CustomInterfaces::Inelastic {

FitOutput::FitOutput() = default;

bool FitOutput::isEmpty() const { return m_parameters.empty(); }

bool FitOutput::isSpectrumFit(FitDomainIndex index) const {
  return static_cast<size_t>(index.value) < m_parameters.size();
}

std::unordered_map<std::string, ParameterValue> FitOutput::getParameters(FitDomainIndex index) const {
  if (isSpectrumFit(index)) {
    return m_parameters.at(index.value);
  } else {
    throw std::invalid_argument("Could not get Paramiters, not fit exists for index: " + std::to_string(index.value));
  }
}

std::optional<ResultLocationNew> FitOutput::getResultLocation(FitDomainIndex index) const {
  if (m_outputResultLocations.count(index.value) == 1) {
    return m_outputResultLocations.at(index.value);
  }
  return std::nullopt;
}

std::vector<std::string> FitOutput::getResultParameterNames() const {
  if (auto resultWorkspace = getLastResultWorkspace()) {
    if (auto workspace = getMatrixWorkspaceFromGroup(resultWorkspace, 0)) {
      return getAxisLabels(workspace, 1);
    }
  }
  return std::vector<std::string>();
}
Mantid::API::WorkspaceGroup_sptr FitOutput::getLastResultWorkspace() const { return m_resultWorkspace.lock(); }
Mantid::API::WorkspaceGroup_sptr FitOutput::getLastResultGroup() const { return m_resultGroup.lock(); }

void FitOutput::clear() {
  m_resultGroup.reset();
  m_resultWorkspace.reset();
  m_parameters.clear();
  m_outputResultLocations.clear();
}

void FitOutput::addOutput(const Mantid::API::WorkspaceGroup_sptr &resultGroup,
                          Mantid::API::ITableWorkspace_sptr parameterTable,
                          const Mantid::API::WorkspaceGroup_sptr &resultWorkspace, FitDomainIndex fitDomainIndex) {
  TableRowExtractor extractRowFromTable(std::move(parameterTable));
  m_resultGroup = resultGroup;
  m_resultWorkspace = resultWorkspace;
  if (resultGroup->size() == 1u) {
    m_parameters.insert_or_assign(fitDomainIndex.value, extractRowFromTable(0));
    m_outputResultLocations.insert_or_assign(fitDomainIndex.value, ResultLocationNew(resultGroup, WorkspaceID{0}));
    return;
  }

  for (size_t index = 0; index < resultGroup->size(); index++) {
    m_parameters.insert_or_assign(index, extractRowFromTable(index));
    m_outputResultLocations.insert_or_assign(index, ResultLocationNew(resultGroup, WorkspaceID{index}));
  }
}

} // namespace MantidQt::CustomInterfaces::Inelastic
