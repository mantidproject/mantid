// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_ENCODER_H
#define MANTID_ISISREFLECTOMETRY_ENCODER_H

#include "../../Reduction/Group.h"
#include "../../Reduction/ReductionJobs.h"
#include "../../Reduction/ReductionWorkspaces.h"
#include "../../Reduction/Row.h"
#include "../Batch/QtBatchView.h"
#include "../Experiment/QtExperimentView.h"
#include "../Instrument/QtInstrumentView.h"
#include "../MainWindow/QtMainWindowView.h"
#include "../Runs/QtRunsView.h"
#include "../RunsTable/QtRunsTableView.h"
#include "../Save/QtSaveView.h"

#include <QMap>
#include <QString>
#include <QTableWidget>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL Encoder {
public:
  QMap<QString, QVariant> encode(const QtMainWindowView &gui);
  QMap<QString, QVariant>
  encodeBatch(const QtBatchView *gui, const QtMainWindowView &mwv,
              bool projectSave = false,
              const BatchPresenter *presenter = nullptr);
  QMap<QString, QVariant> encodeBatch(const IBatchPresenter *presenter,
                                      const IMainWindowView *mwv,
                                      bool projectSave = false);

private:
  BatchPresenter *findBatchPresenter(const QtBatchView *gui,
                                     const QtMainWindowView &mwv);
  QMap<QString, QVariant> encodeExperiment(const QtExperimentView *gui);
  QMap<QString, QVariant> encodePerAngleDefaults(const QTableWidget *tab);
  QList<QVariant> encodePerAngleDefaultsRow(const QTableWidget *tab,
                                            int rowIndex, int columnsNum);
  QList<QVariant> encodePerAngleDefaultsRows(const QTableWidget *tab,
                                             int rowsNum, int columnsNum);
  QMap<QString, QVariant> encodeInstrument(const QtInstrumentView *gui);
  QMap<QString, QVariant> encodeRuns(const QtRunsView *gui, bool projectSave,
                                     const ReductionJobs *redJobs);
  QMap<QString, QVariant> encodeRunsTable(const QtRunsTableView *gui,
                                          bool projectSave,
                                          const ReductionJobs *redJobs);
  QList<QVariant> encodeRunsTableModel(const ReductionJobs *redJobs);
  QMap<QString, QVariant> encodeGroup(
      const MantidQt::CustomInterfaces::ISISReflectometry::Group &group);
  QList<QVariant>
  encodeRows(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group);
  QMap<QString, QVariant> encodeRangeInQ(const RangeInQ &rangeInQ);
  QMap<QString, QVariant>
  encodeTransmissionRunPair(const TransmissionRunPair &transRunPair);
  QMap<QString, QVariant>
  encodeReductionWorkspace(const ReductionWorkspaces &redWs);
  QMap<QString, QVariant>
  encodeReductionOptions(const ReductionOptionsMap &rom);
  QMap<QString, QVariant>
  encodeRow(const MantidQt::CustomInterfaces::ISISReflectometry::Row &row);
  QMap<QString, QVariant> encodeSave(const QtSaveView *gui);
  QMap<QString, QVariant> encodeEvent(const QtEventView *gui);
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_ENCODER_H */