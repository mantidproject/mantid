// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../Common/DllConfig.h"
#include "../../Reduction/ReductionOptionsMap.h"
#include "IDecoder.h"
#include "MantidQtWidgets/Common/BaseDecoder.h"

#include <QComboBox>
#include <QMap>
#include <QString>
#include <QTableWidget>
#include <QVariant>
#include <boost/optional.hpp>
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
class QtRunsView;
class RunsPresenter;
class QtRunsTableView;
class QtSaveView;
class QtEventView;
class RunsTablePresenter;
class IBatchPresenter;
class IMainWindowView;
class RangeInQ;
class TransmissionRunPair;
class QtCatalogSearcher;
class SearchResult;
using SearchResults = std::vector<SearchResult>;

class MANTIDQT_ISISREFLECTOMETRY_DLL Decoder : public MantidQt::API::BaseDecoder, public IDecoder {
public:
  QWidget *decode(const QMap<QString, QVariant> &map, const std::string &directory) override;
  QList<QString> tags() override;
  void decodeBatch(const IMainWindowView *mwv, int batchIndex, const QMap<QString, QVariant> &batchMap) override;

  size_t decodeVersion(const QMap<QString, QVariant> &batchMap) const override;

private:
  BatchPresenter *findBatchPresenter(const QtBatchView *gui, const IMainWindowView *mww);
  void decodeExperiment(QtExperimentView *gui, const QMap<QString, QVariant> &map);
  void decodePolarizationCorrectionsComboBox(QComboBox *polCorrComboBox, const QMap<QString, QVariant> &map) const;
  void decodePerAngleDefaults(QTableWidget *tab, const QMap<QString, QVariant> &map);
  void decodeLegacyPerAngleDefaultsRow(QTableWidget *tab, int rowIndex, int columnsNum, QList<QVariant> list);
  void decodePerAngleDefaultsRow(QTableWidget *tab, int rowIndex, int columnsNum, const QList<QVariant> &list);
  void decodeLegacyPerAngleDefaultsRows(QTableWidget *tab, int rowsNum, int columnsNum, const QList<QVariant> &list);
  void decodePerAngleDefaultsRows(QTableWidget *tab, int rowsNum, int columnsNum, const QList<QVariant> &list);
  void decodeInstrument(const QtInstrumentView *gui, const QMap<QString, QVariant> &map);
  void decodeRuns(QtRunsView *gui, ReductionJobs *redJobs, RunsTablePresenter *presenter,
                  const QMap<QString, QVariant> &map, std::optional<int> precision, QtCatalogSearcher *searcher);
  void decodeRunsTable(QtRunsTableView *gui, ReductionJobs *redJobs, RunsTablePresenter *presenter,
                       const QMap<QString, QVariant> &map, std::optional<int> precision);
  void decodeRunsTableModel(ReductionJobs *jobs, const QList<QVariant> &list);
  MantidQt::CustomInterfaces::ISISReflectometry::Group decodeGroup(const QMap<QString, QVariant> &map);
  std::vector<boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row>>
  decodeRows(const QList<QVariant> &list);
  boost::optional<MantidQt::CustomInterfaces::ISISReflectometry::Row> decodeRow(const QMap<QString, QVariant> &map);
  RangeInQ decodeRangeInQ(const QMap<QString, QVariant> &map);
  TransmissionRunPair decodeTransmissionRunPair(const QMap<QString, QVariant> &map);
  MantidQt::CustomInterfaces::ISISReflectometry::SearchResults decodeSearchResults(const QList<QVariant> &list);
  MantidQt::CustomInterfaces::ISISReflectometry::SearchResult decodeSearchResult(const QMap<QString, QVariant> &map);
  ReductionWorkspaces decodeReductionWorkspace(const QMap<QString, QVariant> &map);
  void decodeSave(const QtSaveView *gui, const QMap<QString, QVariant> &map);
  void decodeEvent(const QtEventView *gui, const QMap<QString, QVariant> &map);
  void updateRunsTableViewFromModel(QtRunsTableView *view, const ReductionJobs *model,
                                    const std::optional<int> &precision);

  bool m_projectSave = false;
  size_t m_currentBatchVersion = 0;
  friend class CoderCommonTester;
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
