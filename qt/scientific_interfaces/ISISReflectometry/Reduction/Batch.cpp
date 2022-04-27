// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Batch.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

Batch::Batch(Experiment const &experiment, Instrument const &instrument, RunsTable &runsTable, Slicing const &slicing)
    : m_experiment(experiment), m_instrument(instrument), m_runsTable(runsTable), m_slicing(slicing) {}

Experiment const &Batch::experiment() const { return m_experiment; }

Instrument const &Batch::instrument() const { return m_instrument; }

RunsTable const &Batch::runsTable() const { return m_runsTable; }

RunsTable &Batch::mutableRunsTable() { return m_runsTable; }

Slicing const &Batch::slicing() const { return m_slicing; }

std::vector<MantidWidgets::Batch::RowLocation> Batch::selectedRowLocations() const {
  return m_runsTable.selectedRowLocations();
}

boost::optional<LookupRow> Batch::findLookupRow(Row const &row) const {
  return experiment().findLookupRow(row, runsTable().thetaTolerance());
}

boost::optional<LookupRow> Batch::findWildcardLookupRow() const { return experiment().findWildcardLookupRow(); }

void Batch::resetState() { m_runsTable.resetState(); }

void Batch::resetSkippedItems() { m_runsTable.resetSkippedItems(); }

boost::optional<Item &> Batch::getItemWithOutputWorkspaceOrNone(std::string const &wsName) {
  return m_runsTable.getItemWithOutputWorkspaceOrNone(wsName);
}

void Batch::updateLookupIndex(Row &row) {
  try {
    row.setLookupIndex(experiment().getLookupRowIndexFromRow(row, runsTable().thetaTolerance()));
  } catch (MultipleRowsFoundException &e) {
    row.setError(e.what());
  }
}

void Batch::updateLookupIndexesOfGroup(Group &group) {
  for (auto &row : group.mutableRows()) {
    if (row.is_initialized()) {
      updateLookupIndex(row.get());
    }
  }
}

void Batch::updateLookupIndexesOfTable() {
  for (auto &group : m_runsTable.mutableReductionJobs().mutableGroups()) {
    updateLookupIndexesOfGroup(group);
  }
}

bool Batch::isInSelection(const Item &item,
                          const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) {
  return m_runsTable.isInSelection(item, selectedRowLocations);
}

bool Batch::isInSelection(const Row &item, const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) {
  return m_runsTable.isInSelection(item, selectedRowLocations);
}

bool Batch::isInSelection(const Group &item,
                          const std::vector<MantidWidgets::Batch::RowLocation> &selectedRowLocations) {
  return m_runsTable.isInSelection(item, selectedRowLocations);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
