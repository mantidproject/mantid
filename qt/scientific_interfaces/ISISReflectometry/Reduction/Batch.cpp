// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Batch.h"
namespace MantidQt {
namespace CustomInterfaces {

Batch::Batch(Experiment const &experiment, Instrument const &instrument,
             RunsTable &runsTable, Slicing const &slicing)
    : m_experiment(experiment), m_instrument(instrument),
      m_runsTable(runsTable), m_slicing(slicing) {}

Experiment const &Batch::experiment() const { return m_experiment; }

Instrument const &Batch::instrument() const { return m_instrument; }

RunsTable const &Batch::runsTable() const { return m_runsTable; }

RunsTable &Batch::mutableRunsTable() { return m_runsTable; }

Slicing const &Batch::slicing() const { return m_slicing; }

std::vector<MantidWidgets::Batch::RowLocation>
Batch::selectedRowLocations() const {
  return m_runsTable.selectedRowLocations();
}

PerThetaDefaults const *Batch::defaultsForTheta(double thetaAngle) const {
  return experiment().defaultsForTheta(thetaAngle,
                                       runsTable().thetaTolerance());
}

void Batch::resetState() { m_runsTable.resetState(); }

void Batch::resetSkippedItems() { m_runsTable.resetSkippedItems(); }

boost::optional<Item &>
Batch::getItemWithOutputWorkspaceOrNone(std::string const &wsName) {
  return m_runsTable.getItemWithOutputWorkspaceOrNone(wsName);
}
} // namespace CustomInterfaces
} // namespace MantidQt
