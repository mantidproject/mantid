// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_DECODER_H
#define MANTID_ISISREFLECTOMETRY_DECODER_H

#include "../../Reduction/Group.h"
#include "../../Reduction/ReductionJobs.h"
#include "../../Reduction/ReductionWorkspaces.h"
#include "../../Reduction/Row.h"
#include "../Batch/BatchView.h"
#include "../Experiment/ExperimentView.h"
#include "../Instrument/InstrumentView.h"
#include "../MainWindow/MainWindowView.h"
#include "../Runs/RunsView.h"
#include "../RunsTable/RunsTablePresenter.h"
#include "../RunsTable/RunsTableView.h"
#include "../Save/SaveView.h"

#include <QMap>
#include <QString>
#include <QTableWidget>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {

class Decoder {
public:
  void decode(const MainWindowView &gui, const QMap<QString, QVariant> &map);
  void decodeBatch(const BatchView *gui, const MainWindowView &mwv,
                   QMap<QString, QVariant> &map,
                   const BatchPresenter *presenter = nullptr);
  void decodeBatch(const IBatchPresenter *presenter, const IMainWindowView *mwv,
                   QMap<QString, QVariant> &map);

private:
  BatchPresenter *findBatchPresenter(const BatchView *gui,
                                     const MainWindowView &mww);
  void decodeExperiment(const ExperimentView *gui,
                        const QMap<QString, QVariant> &map);
  void decodePerAngleDefaults(QTableWidget *tab,
                              const QMap<QString, QVariant> &map);
  void decodePerAngleDefaultsRow(QTableWidget *tab, int rowIndex,
                                 int columnsNum, const QList<QVariant> &list);
  void decodePerAngleDefaultsRows(QTableWidget *tab, int rowsNum,
                                  int columnsNum, const QList<QVariant> &list);
  void decodeInstrument(const InstrumentView *gui,
                        const QMap<QString, QVariant> &map);
  void decodeRuns(RunsView *gui, ReductionJobs *redJobs,
                  RunsTablePresenter *presenter,
                  const QMap<QString, QVariant> &map);
  void decodeRunsTable(RunsTableView *gui, ReductionJobs *redJobs,
                       RunsTablePresenter *presenter,
                       const QMap<QString, QVariant> &map);
  void decodeRunsTableModel(ReductionJobs *jobs, const QList<QVariant> &list);
  MantidQt::CustomInterfaces::Group
  decodeGroup(const QMap<QString, QVariant> &map);
  std::vector<boost::optional<MantidQt::CustomInterfaces::Row>>
  decodeRows(const QList<QVariant> &list);
  boost::optional<MantidQt::CustomInterfaces::Row>
  decodeRow(const QMap<QString, QVariant> &map);
  RangeInQ decodeRangeInQ(const QMap<QString, QVariant> &map);
  TransmissionRunPair
  decodeTransmissionRunPair(const QMap<QString, QVariant> &map);
  ReductionWorkspaces
  decodeReductionWorkspace(const QMap<QString, QVariant> &map);
  void decodeSave(const SaveView *gui, const QMap<QString, QVariant> &map);
  void decodeEvent(const EventView *gui, const QMap<QString, QVariant> &map);
  void updateRunsTableViewFromModel(RunsTableView *view,
                                    const ReductionJobs *model);
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_DECODER_H */