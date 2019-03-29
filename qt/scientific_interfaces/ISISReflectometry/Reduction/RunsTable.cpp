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

bool RunsTable::hasSelection() const {
  return m_selectedRowLocations.size() > 0;
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

std::vector<Group> selectedGroups() const {
  for (const auto &rowLocation : m_selectedRowLocations) {
    const auto rowPath = rowLocation.path();
    const auto group = m_reductionJobs.getGroupFromPath();
    if () {
    }
  }
}
std::vector<Row> selectedRows() const {}
} // namespace CustomInterfaces
} // namespace MantidQt
