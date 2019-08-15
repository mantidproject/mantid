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
#include "../Batch/BatchPresenter.h"
#include "../Batch/QtBatchView.h"
#include "../Experiment/QtExperimentView.h"
#include "../Instrument/QtInstrumentView.h"
#include "../MainWindow/QtMainWindowView.h"
#include "../Runs/QtRunsView.h"
#include "../Runs/RunsPresenter.h"
#include "../RunsTable/QtRunsTableView.h"
#include "../RunsTable/RunsTablePresenter.h"
#include "../Save/QtSaveView.h"
#include "MantidQtWidgets/Common/BaseDecoder.h"

#include <QMap>
#include <QString>
#include <QTableWidget>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL Decoder
    : public MantidQt::API::BaseDecoder {
public:
  void decode(QWidget *gui, const QMap<QString, QVariant> &map) override;
  QList<QString> tags() override;
  void decodeBatch(const QtBatchView *gui, const QtMainWindowView &mwv,
                   const QMap<QString, QVariant> &map,
                   const BatchPresenter *presenter = nullptr);
  void decodeBatch(const IBatchPresenter *presenter, const IMainWindowView *mwv,
                   const QMap<QString, QVariant> &map);

private:
  BatchPresenter *findBatchPresenter(const QtBatchView *gui,
                                     const QtMainWindowView &mww);
  void decodeExperiment(const QtExperimentView *gui,
                        const QMap<QString, QVariant> &map);
  void decodePerAngleDefaults(QTableWidget *tab,
                              const QMap<QString, QVariant> &map);
  void decodePerAngleDefaultsRow(QTableWidget *tab, int rowIndex,
                                 int columnsNum, const QList<QVariant> &list);
  void decodePerAngleDefaultsRows(QTableWidget *tab, int rowsNum,
                                  int columnsNum, const QList<QVariant> &list);
  void decodeInstrument(const QtInstrumentView *gui,
                        const QMap<QString, QVariant> &map);
  void decodeRuns(QtRunsView *gui, ReductionJobs *redJobs,
                  RunsTablePresenter *presenter,
                  const QMap<QString, QVariant> &map);
  void decodeRunsTable(QtRunsTableView *gui, ReductionJobs *redJobs,
                       RunsTablePresenter *presenter,
                       const QMap<QString, QVariant> &map);
  void decodeRunsTableModel(ReductionJobs *jobs, const QList<QVariant> &list);
  MantidQt::CustomInterfaces::ISISReflectometry::Group
  decodeGroup(const QMap<QString, QVariant> &map);
  std::vector<
      boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>>
  decodeRows(const QList<QVariant> &list);
  boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>
  decodeRow(const QMap<QString, QVariant> &map);
  RangeInQ decodeRangeInQ(const QMap<QString, QVariant> &map);
  TransmissionRunPair
  decodeTransmissionRunPair(const QMap<QString, QVariant> &map);
  ReductionWorkspaces
  decodeReductionWorkspace(const QMap<QString, QVariant> &map);
  void decodeSave(const QtSaveView *gui, const QMap<QString, QVariant> &map);
  void decodeEvent(const QtEventView *gui, const QMap<QString, QVariant> &map);
  void updateRunsTableViewFromModel(QtRunsTableView *view,
                                    const ReductionJobs *model);
  friend class CoderCommonTester;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_DECODER_H */