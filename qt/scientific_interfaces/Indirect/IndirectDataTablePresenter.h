// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndexTypes.h"
#include "IndirectFittingModel.h"

#include <QTableWidget>

#include <cstddef>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using DataPositionType = IndexCollectionType<TableDatasetIndex, FitDomainIndex>;

/**
  Presenter for a table of indirect fitting data.
*/
class MANTIDQT_INDIRECT_DLL IndirectDataTablePresenter : public QObject {
  Q_OBJECT
public:
  IndirectDataTablePresenter(IndirectFittingModel *model,
                             QTableWidget *dataTable);

  IndirectDataTablePresenter(IndirectFittingModel *model,
                             QTableWidget *dataTable,
                             const QStringList &headers);

  bool tableDatasetsMatchModel() const;
  bool isTableEmpty() const;

  void setStartX(double startX, TableDatasetIndex dataIndex,
                 WorkspaceIndex spectrumIndex);
  void setStartX(double startX, TableDatasetIndex dataIndex);
  void setEndX(double endX, TableDatasetIndex dataIndex,
               WorkspaceIndex spectrumIndex);
  void setEndX(double endX, TableDatasetIndex dataIndex);
  void setExclude(const std::string &exclude, TableDatasetIndex dataIndex,
                  WorkspaceIndex spectrumIndex);

signals:
  void startXChanged(double /*_t1*/, TableDatasetIndex /*_t2*/,
                     WorkspaceIndex /*_t3*/);
  void endXChanged(double /*_t1*/, TableDatasetIndex /*_t2*/,
                   WorkspaceIndex /*_t3*/);
  void excludeRegionChanged(const std::string & /*_t1*/,
                            TableDatasetIndex /*_t2*/, WorkspaceIndex /*_t3*/);

public slots:
  void addData(TableDatasetIndex index);
  void updateData(TableDatasetIndex index);
  void removeSelectedData();
  void setStartX(double startX, FitDomainIndex index);
  void setEndX(double endX, FitDomainIndex index);
  void setExcludeRegion(const std::string &exclude, FitDomainIndex index);
  void setGlobalFittingRange(bool global);
  void enableTable();
  void disableTable();
  void clearTable();

private slots:
  void setModelFittingRange(int row, int column);
  void updateAllFittingRangeFrom(int row, int column);

protected:
  FitDomainIndex getFirstRow(TableDatasetIndex dataIndex) const;
  std::string getString(FitDomainIndex row, int column) const;

  virtual void addTableEntry(TableDatasetIndex dataIndex,
                             WorkspaceIndex spectrum, FitDomainIndex row);
  void setCell(std::unique_ptr<QTableWidgetItem> cell, FitDomainIndex row,
               int column);
  virtual void updateTableEntry(TableDatasetIndex dataIndex,
                                WorkspaceIndex spectrum, FitDomainIndex row);
  void setCellText(const QString &text, FitDomainIndex row, int column);

private:
  virtual int workspaceIndexColumn() const;
  virtual int startXColumn() const;
  virtual int endXColumn() const;
  virtual int excludeColumn() const;
  double startX(FitDomainIndex row) const;
  double endX(FitDomainIndex row) const;
  std::string getExcludeString(FitDomainIndex row) const;
  std::string getWorkspaceName(FitDomainIndex row) const;
  WorkspaceIndex getWorkspaceIndex(FitDomainIndex row) const;
  double getDouble(FitDomainIndex row, int column) const;
  QString getText(FitDomainIndex row, int column) const;
  FitDomainIndex getNextPosition(TableDatasetIndex index) const;
  TableDatasetIndex getDataIndex(FitDomainIndex row) const;
  boost::optional<Spectra> getSpectra(TableDatasetIndex dataIndex) const;
  boost::optional<Spectra> getSpectra(FitDomainIndex start,
                                      FitDomainIndex end) const;
  boost::optional<FitDomainIndex>
  getRowIndex(TableDatasetIndex dataIndex, WorkspaceIndex spectrumIndex) const;

  void setModelStartXAndEmit(double startX, TableDatasetIndex dataIndex,
                             WorkspaceIndex workspaceIndex);
  void setModelEndXAndEmit(double endX, TableDatasetIndex dataIndex,
                           WorkspaceIndex workspaceIndex);
  void setModelExcludeAndEmit(const std::string &exclude,
                              TableDatasetIndex dataIndex,
                              WorkspaceIndex workspaceIndex);

  void enableGlobalFittingRange();
  void disableGlobalFittingRange();

  void updateExistingData(TableDatasetIndex index);
  void addNewData(TableDatasetIndex index);
  void addTableEntry(TableDatasetIndex dataIndex, WorkspaceIndex spectrum);
  TableDatasetIndex removeTableEntry(FitDomainIndex row);
  std::pair<std::vector<TableDatasetIndex>, std::vector<FitDomainIndex>>
  removeTableRows(QModelIndexList &selectedRows);
  void setStartX(double startX);
  void setEndX(double endX);
  void setExcludeRegion(const std::string &exclude);
  void setExcludeRegion(const QString &exclude);
  void setColumnValues(int column, const QString &value);
  void setHorizontalHeaders(const QStringList &headers);

  void collapseData(FitDomainIndex from, FitDomainIndex to,
                    FitDomainIndex initialSize, TableDatasetIndex dataIndex);
  void updateFromRemovedIndices(const std::vector<TableDatasetIndex> &indices);
  void shiftDataPositions(FitDomainIndex value, TableDatasetIndex from,
                          TableDatasetIndex to);
  void updateDataPositionsInCells(TableDatasetIndex from, TableDatasetIndex to);

  DataPositionType m_dataPositions;
  IndirectFittingModel *m_model;
  QTableWidget *m_dataTable;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
