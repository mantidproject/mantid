// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../Common/DllConfig.h"
#include "../../Reduction/ReductionOptionsMap.h"
#include "IEncoder.h"
#include "MantidQtWidgets/Common/BaseEncoder.h"

#include <QMap>
#include <QString>
#include <QTableWidget>
#include <QVariant>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class Group;
class ReductionJobs;
class ReductionWorkspaces;
class Row;
class BatchPresenter;
class IMainWindowView;
class QtBatchView;
class QtExperimentView;
class QtInstrumentView;
class QtMainWindowView;
class QtRunsView;
class RunsPresenter;
class QtRunsTableView;
class QtSaveView;
class IBatchPresenter;
class IMainWindowView;
class ISearchModel;
class RangeInQ;
class QtCatalogSearcher;
class SearchResult;
class TransmissionRunPair;
class QtEventView;

class MANTIDQT_ISISREFLECTOMETRY_DLL Encoder : public MantidQt::API::BaseEncoder, public IEncoder {
public:
  QMap<QString, QVariant> encode(const QWidget *window, const std::string &directory) override;
  QList<QString> tags() override;
  QMap<QString, QVariant> encodeBatch(const IMainWindowView *mwv, int batchIndex, bool projectSave = false) override;
  QVariant extractFromEncoding(const QVariant &vMap, const std::vector<std::string> &jsonKey) const override;

private:
  BatchPresenter *findBatchPresenter(const QtBatchView *gui, const IMainWindowView *mwv);
  QMap<QString, QVariant> encodeExperiment(const QtExperimentView *gui);
  QMap<QString, QVariant> encodePerAngleDefaults(const QTableWidget *tab);
  QList<QVariant> encodePerAngleDefaultsRow(const QTableWidget *tab, int rowIndex, int columnsNum);
  QList<QVariant> encodePerAngleDefaultsRows(const QTableWidget *tab, int rowsNum, int columnsNum);
  QMap<QString, QVariant> encodeInstrument(const QtInstrumentView *gui);
  QMap<QString, QVariant> encodeRuns(const QtRunsView *gui, bool projectSave, const ReductionJobs *redJobs,
                                     QtCatalogSearcher *searcher);
  QMap<QString, QVariant> encodeRunsTable(const QtRunsTableView *gui, bool projectSave, const ReductionJobs *redJobs);
  QList<QVariant> encodeRunsTableModel(const ReductionJobs *redJobs);
  QMap<QString, QVariant> encodeGroup(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group);
  QList<QVariant> encodeRows(const MantidQt::CustomInterfaces::ISISReflectometry::Group &group);
  QMap<QString, QVariant> encodeRangeInQ(const RangeInQ &rangeInQ);
  QMap<QString, QVariant> encodeTransmissionRunPair(const TransmissionRunPair &transRunPair);
  QMap<QString, QVariant> encodeReductionWorkspace(const ReductionWorkspaces &redWs);
  QMap<QString, QVariant> encodeReductionOptions(const ReductionOptionsMap &rom);
  QMap<QString, QVariant> encodeRow(const MantidQt::CustomInterfaces::ISISReflectometry::Row &row);
  QList<QVariant> encodeSearchModel(const MantidQt::CustomInterfaces::ISISReflectometry::ISearchModel &searchModel);
  QMap<QString, QVariant> encodeSearchResult(const MantidQt::CustomInterfaces::ISISReflectometry::SearchResult &row);
  QMap<QString, QVariant> encodeSave(const QtSaveView *gui);
  QMap<QString, QVariant> encodeEvent(const QtEventView *gui);
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
