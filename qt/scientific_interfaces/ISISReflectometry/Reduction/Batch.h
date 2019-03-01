// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCH_H_
#define MANTID_CUSTOMINTERFACES_BATCH_H_

#include "Common/DllConfig.h"
#include "Experiment.h"
#include "Instrument.h"
#include "RunsTable.h"
#include "Slicing.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class Batch

    The Batch model holds the entire reduction configuration for a batch of
    runs.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL Batch {
public:
  Batch(Experiment const &experiment, Instrument const &instrument,
        RunsTable &runsTable, Slicing const &slicing);

  Experiment const &experiment() const;
  Instrument const &instrument() const;
  RunsTable const &runsTable() const;
  RunsTable &mutableRunsTable();
  Slicing const &slicing() const;

  bool hasSelection() const;
  template <typename T> bool isSelected(T const &item) const;

  PerThetaDefaults const *defaultsForTheta(double thetaAngle) const;
  void resetState();
  void resetSkippedItems();
  boost::optional<Item &>
  getItemWithOutputWorkspaceOrNone(std::string const &wsName);

private:
  Experiment const &m_experiment;
  Instrument const &m_instrument;
  RunsTable &m_runsTable;
  Slicing const &m_slicing;
};

template <typename T> bool Batch::isSelected(T const &item) const {
  return m_runsTable.isSelected(item);
}
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_BATCH_H_
