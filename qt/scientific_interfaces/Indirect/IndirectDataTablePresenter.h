// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATATABLEPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATATABLEPRESENTER_H_

#include "IndexTypes.h"
#include "IndirectFittingModel.h"

#include <QTableWidget>

#include <cstddef>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using DataPositionType = IndexCollectionType<DatasetIndex, SpectrumRowIndex>;

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

  void setStartX(double startX, DatasetIndex dataIndex,
                 WorkspaceIndex spectrumIndex);
  void setStartX(double startX, DatasetIndex dataIndex);
  void setEndX(double endX, DatasetIndex dataIndex,
               WorkspaceIndex spectrumIndex);
  void setEndX(double endX, DatasetIndex dataIndex);
  void setExclude(const std::string &exclude, DatasetIndex dataIndex,
                  WorkspaceIndex spectrumIndex);

signals:
  void startXChanged(double /*_t1*/, DatasetIndex /*_t2*/,
                     WorkspaceIndex /*_t3*/);
  void endXChanged(double /*_t1*/, DatasetIndex /*_t2*/,
                   WorkspaceIndex /*_t3*/);
  void excludeRegionChanged(const std::string & /*_t1*/, DatasetIndex /*_t2*/,
                            WorkspaceIndex /*_t3*/);

public slots:
  void addData(DatasetIndex index);
  void updateData(DatasetIndex index);
  void removeSelectedData();
  void setStartX(double startX, SpectrumRowIndex index);
  void setEndX(double endX, SpectrumRowIndex index);
  void setExcludeRegion(const std::string &exclude, SpectrumRowIndex index);
  void setGlobalFittingRange(bool global);
  void enableTable();
  void disableTable();
  void clearTable();

private slots:
  void setModelFittingRange(int row, int column);
  void updateAllFittingRangeFrom(int row, int column);

protected:
  SpectrumRowIndex getFirstRow(DatasetIndex dataIndex) const;
  std::string getString(SpectrumRowIndex row, int column) const;

  virtual void addTableEntry(DatasetIndex dataIndex, WorkspaceIndex spectrum,
                             SpectrumRowIndex row);
  void setCell(std::unique_ptr<QTableWidgetItem> cell, SpectrumRowIndex row,
               int column);
  virtual void updateTableEntry(DatasetIndex dataIndex, WorkspaceIndex spectrum,
                                SpectrumRowIndex row);
  void setCellText(const QString &text, SpectrumRowIndex row, int column);

private:
  virtual int workspaceIndexColumn() const;
  virtual int startXColumn() const;
  virtual int endXColumn() const;
  virtual int excludeColumn() const;
  double startX(SpectrumRowIndex row) const;
  double endX(SpectrumRowIndex row) const;
  std::string getExcludeString(SpectrumRowIndex row) const;
  std::string getWorkspaceName(SpectrumRowIndex row) const;
  WorkspaceIndex getWorkspaceIndex(SpectrumRowIndex row) const;
  double getDouble(SpectrumRowIndex row, int column) const;
  QString getText(SpectrumRowIndex row, int column) const;
  SpectrumRowIndex getNextPosition(DatasetIndex index) const;
  DatasetIndex getDataIndex(SpectrumRowIndex row) const;
  boost::optional<Spectra> getSpectra(DatasetIndex dataIndex) const;
  boost::optional<Spectra> getSpectra(SpectrumRowIndex start,
                                      SpectrumRowIndex end) const;
  boost::optional<SpectrumRowIndex>
  getRowIndex(DatasetIndex dataIndex, WorkspaceIndex spectrumIndex) const;

  void setModelStartXAndEmit(double startX, DatasetIndex dataIndex,
                             WorkspaceIndex workspaceIndex);
  void setModelEndXAndEmit(double endX, DatasetIndex dataIndex,
                           WorkspaceIndex workspaceIndex);
  void setModelExcludeAndEmit(const std::string &exclude,
                              DatasetIndex dataIndex,
                              WorkspaceIndex workspaceIndex);

  void enableGlobalFittingRange();
  void disableGlobalFittingRange();

  void updateExistingData(DatasetIndex index);
  void addNewData(DatasetIndex index);
  void addTableEntry(DatasetIndex dataIndex, WorkspaceIndex spectrum);
  DatasetIndex removeTableEntry(SpectrumRowIndex row);
  std::pair<std::vector<DatasetIndex>, std::vector<SpectrumRowIndex>>
  removeTableRows(QModelIndexList &selectedRows);
  void setStartX(double startX);
  void setEndX(double endX);
  void setExcludeRegion(const std::string &exclude);
  void setExcludeRegion(const QString &exclude);
  void setColumnValues(int column, const QString &value);
  void setHorizontalHeaders(const QStringList &headers);

  void collapseData(SpectrumRowIndex from, SpectrumRowIndex to,
                    SpectrumRowIndex initialSize, DatasetIndex dataIndex);
  void updateFromRemovedIndices(const std::vector<DatasetIndex> &indices);
  void shiftDataPositions(SpectrumRowIndex value, DatasetIndex from,
                          DatasetIndex to);
  void updateDataPositionsInCells(DatasetIndex from, DatasetIndex to);

  DataPositionType m_dataPositions;
  IndirectFittingModel *m_model;
  QTableWidget *m_dataTable;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTDATATABLEPRESENTER_H_ */
