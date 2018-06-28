#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTDATATABLEPRESENTER_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTDATATABLEPRESENTER_H_

#include "IndirectFittingModel.h"

#include <QTableWidget>

#include <cstddef>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
  Presenter for a table of indirect fitting data.
  Copyright &copy; 2015-2016 ISIS Rutherford Appleton Laboratory, NScD
  Oak Ridge National Laboratory & European Spallation Source
  This file is part of Mantid.
  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IndirectDataTablePresenter : public QObject {
  Q_OBJECT
public:
  IndirectDataTablePresenter(IndirectFittingModel *model,
                             QTableWidget *dataTable);

  IndirectDataTablePresenter(IndirectFittingModel *model,
                             QTableWidget *dataTable,
                             const QStringList &headers);

  bool tableDatasetsMatchModel() const;
  bool isTableEmpty() const;

  void setStartX(double startX, std::size_t dataIndex, int spectrumIndex);
  void setEndX(double endX, std::size_t dataIndex, int spectrumIndex);
  void setExclude(const std::string &exclude, std::size_t dataIndex,
                  int spectrumIndex);

signals:
  void startXChanged(double, std::size_t, std::size_t);
  void endXChanged(double, std::size_t, std::size_t);
  void excludeRegionChanged(const std::string &, std::size_t, std::size_t);

public slots:
  void addData(std::size_t index);
  void updateData(std::size_t index);
  void removeSelectedData();
  void setStartX(double startX, int index);
  void setEndX(double endX, int index);
  void setExcludeRegion(const std::string &exclude, int index);
  void setGlobalFittingRange(bool global);
  void enableTable();
  void disableTable();
  void clearTable();

private slots:
  void setModelFittingRange(int row, int column);
  void updateAllFittingRangeFrom(int row, int column);

protected:
  int getFirstRow(std::size_t dataIndex) const;
  std::string getString(int row, int column) const;

  virtual void addTableEntry(std::size_t dataIndex, std::size_t spectrum,
                             int row);
  void setCell(QTableWidgetItem *cell, int row, int column);
  virtual void updateTableEntry(std::size_t dataIndex, std::size_t spectrum,
                                int row);
  void setCellText(const QString &text, int row, int column);

private:
  virtual int workspaceIndexColumn() const;
  virtual int startXColumn() const;
  virtual int endXColumn() const;
  virtual int excludeColumn() const;
  double startX(int row) const;
  double endX(int row) const;
  std::string getExcludeString(int row) const;
  std::string getWorkspaceName(int row) const;
  std::size_t getWorkspaceIndex(int row) const;
  double getDouble(int row, int column) const;
  QString getText(int row, int column) const;
  int getNextPosition(std::size_t index) const;
  std::size_t getDataIndex(int row) const;
  boost::optional<Spectra> getSpectra(std::size_t dataIndex) const;
  boost::optional<Spectra> getSpectra(int start, int end) const;
  boost::optional<int> getRowIndex(std::size_t dataIndex,
                                   int spectrumIndex) const;

  void setModelStartXAndEmit(double startX, std::size_t dataIndex,
                             std::size_t workspaceIndex);
  void setModelEndXAndEmit(double endX, std::size_t dataIndex,
                           std::size_t workspaceIndex);
  void setModelExcludeAndEmit(const std::string &exclude, std::size_t dataIndex,
                              std::size_t workspaceIndex);

  void enableGlobalFittingRange();
  void disableGlobalFittingRange();

  void addTableEntry(std::size_t dataIndex, std::size_t spectrum);
  std::size_t removeTableEntry(int row);
  std::pair<std::vector<std::size_t>, std::vector<std::size_t>>
  removeTableRows(QModelIndexList &selectedRows);
  void setStartX(double startX);
  void setEndX(double endX);
  void setExcludeRegion(const std::string &exclude);
  void setExcludeRegion(const QString &exclude);
  void setColumnValues(int column, const QString &value);
  void setHorizontalHeaders(const QStringList &headers);

  void collapseData(int from, int to, int initialSize, std::size_t dataIndex);
  void updateFromRemovedIndices(const std::vector<std::size_t> &indices);
  void shiftDataPositions(int value, std::size_t from, std::size_t to);
  void updateDataPositionsInCells(std::size_t from, std::size_t to);

  std::vector<int> m_dataPositions;
  IndirectFittingModel *m_model;
  QTableWidget *m_dataTable;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTDATATABLEPRESENTER_H_ */
