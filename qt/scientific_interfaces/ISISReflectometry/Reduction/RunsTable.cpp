// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsTable.h"
#include "RowLocation.h"

namespace MantidQt {
namespace CustomInterfaces {

using MantidWidgets::Batch::RowLocation;

RunsTable::RunsTable( // cppcheck-suppress passedByValue
    std::vector<std::string> instruments, double thetaTolerance,
    // cppcheck-suppress passedByValue
    ReductionJobs reductionJobs)
    : m_instruments(std::move(instruments)), m_thetaTolerance(thetaTolerance),
      m_reductionJobs(std::move(reductionJobs)), m_selectedRowLocations() {}

double RunsTable::thetaTolerance() const { return m_thetaTolerance; }

ReductionJobs const &RunsTable::reductionJobs() const {
  return m_reductionJobs;
}

ReductionJobs &RunsTable::mutableReductionJobs() { return m_reductionJobs; }

std::vector<RowLocation> const &RunsTable::selectedRowLocations() const {
  return m_selectedRowLocations;
}

void RunsTable::setSelectedRowLocations(std::vector<RowLocation> selected) {
  m_selectedRowLocations = std::move(selected);
}

void RunsTable::resetState() { m_reductionJobs.resetState(); }

void RunsTable::resetSkippedItems() { m_reductionJobs.resetSkippedItems(); }

boost::optional<Item &>
RunsTable::getItemWithOutputWorkspaceOrNone(std::string const &wsName) {
  return m_reductionJobs.getItemWithOutputWorkspaceOrNone(wsName);
}

std::vector<Group> RunsTable::selectedGroups() const {
  std::vector<Group> groups;
  for (const auto &rowLocation : m_selectedRowLocations) {
    if (!isGroupLocation(rowLocation))
      continue;
    const auto group = m_reductionJobs.getGroupFromPath(rowLocation);
    groups.emplace_back(group);
  }
  return groups;
}

std::vector<Row> RunsTable::selectedRows() const {
  std::vector<Row> rows;
  for (const auto &rowLocation : m_selectedRowLocations) {
    if (!isRowLocation(rowLocation))
      continue;
    const auto row = m_reductionJobs.getRowFromPath(rowLocation);
    if (row.is_initialized())
      rows.emplace_back(row.get());
  }
  return rows;
}
} // namespace CustomInterfaces
} // namespace MantidQt
