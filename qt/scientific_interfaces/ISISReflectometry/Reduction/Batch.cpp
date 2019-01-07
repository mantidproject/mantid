// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
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

RunsTable &Batch::runsTable() { return m_runsTable; }

Slicing const &Batch::slicing() const { return m_slicing; }

bool Batch::hasSelection() const { return m_runsTable.hasSelection(); }
} // namespace CustomInterfaces
} // namespace MantidQt
