// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_ENCODER_H
#define MANTID_ISISREFLECTOMETRY_ENCODER_H

#include "GUI/Batch/BatchView.h"
#include "GUI/Experiment/ExperimentView.h"
#include "GUI/Instrument/InstrumentView.h"
#include "GUI/MainWindow/MainWindowView.h"
#include "GUI/Runs/RunsView.h"
#include "GUI/RunsTable/RunsTableView.h"
#include "GUI/Save/SaveView.h"
#include "Reduction/Group.h"
#include "Reduction/ReductionJobs.h"
#include "Reduction/ReductionWorkspaces.h"
#include "Reduction/Row.h"

#include <QMap>
#include <QString>
#include <QTableWidget>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {

class ISISReflectometryEncoder {
public:
  ISISReflectometryEncoder();
  QMap<QString, QVariant> encode(const MainWindowView &gui);
  QMap<QString, QVariant> encodeBatch(const BatchView *gui,
                                      const MainWindowView &mwv,
                                      bool projectSave = false);

private:
  BatchPresenter *findBatchPresenter(const BatchView *gui,
                                     const MainWindowView &mwv);
  QMap<QString, QVariant> encodeExperiment(const ExperimentView *gui);
  QMap<QString, QVariant> encodePerAngleDefaults(const QTableWidget *tab);
  QList<QVariant> encodePerAngleDefaultsRow(const QTableWidget *tab,
                                            int rowIndex, int columnsNum);
  QList<QVariant> encodePerAngleDefaultsRows(const QTableWidget *tab,
                                             int rowsNum, int columnsNum);
  QMap<QString, QVariant> encodeInstrument(const InstrumentView *gui);
  QMap<QString, QVariant> encodeRuns(const RunsView *gui, bool projectSave,
                                     const ReductionJobs *redJobs);
  QMap<QString, QVariant> encodeRunsTable(const RunsTableView *gui,
                                          bool projectSave,
                                          const ReductionJobs *redJobs);
  QList<QVariant> encodeRunsTableModel(const ReductionJobs *redJobs);
  QMap<QString, QVariant>
  encodeGroup(const MantidQt::CustomInterfaces::Group &group);
  QList<QVariant> encodeRows(const MantidQt::CustomInterfaces::Group &group);
  QMap<QString, QVariant> encodeRangeInQ(const RangeInQ &rangeInQ);
  QMap<QString, QVariant>
  encodeTransmissionRunPair(const TransmissionRunPair &transRunPair);
  QMap<QString, QVariant>
  encodeReductionWorkspace(const ReductionWorkspaces &redWs);
  QMap<QString, QVariant>
  encodeReductionOptions(const ReductionOptionsMap &rom);
  QMap<QString, QVariant> encodeRow(const MantidQt::CustomInterfaces::Row &row);
  QMap<QString, QVariant> encodeSave(const SaveView *gui);
  QMap<QString, QVariant> encodeEvent(const EventView *gui);
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_ENCODER_H */