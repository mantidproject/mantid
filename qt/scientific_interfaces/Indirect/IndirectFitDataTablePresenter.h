// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFittingModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <QTableWidget>

#include <cstddef>
#include <memory>
#include <unordered_map>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

using DataPositionType = IndexCollectionType<WorkspaceID, FitDomainIndex>;

/**
  Presenter for a table of indirect fitting data.
*/
class MANTIDQT_INDIRECT_DLL IndirectFitDataTablePresenter : public QObject {
  Q_OBJECT
public:
  IndirectFitDataTablePresenter(IIndirectFitDataTableModel *model, QTableWidget *dataTable);

  bool isTableEmpty() const;

  void updateTableFromModel();
  void clearTable();

public slots:
  void removeSelectedData();

signals:
  void startXChanged(double /*_t1*/, WorkspaceID /*_t2*/, WorkspaceIndex /*_t3*/);
  void endXChanged(double /*_t1*/, WorkspaceID /*_t2*/, WorkspaceIndex /*_t3*/);

private slots:
  void handleCellChanged(int row, int column);
  // void updateAllFittingRangeFrom(int row, int column);

protected:
  IndirectFitDataTablePresenter(IIndirectFitDataTableModel *model, QTableWidget *dataTable, const QStringList &headers);
  std::string getString(FitDomainIndex row, int column) const;

  virtual void addTableEntry(FitDomainIndex row);
  void setCell(std::unique_ptr<QTableWidgetItem> cell, FitDomainIndex row, int column);
  void setCellText(const QString &text, FitDomainIndex row, int column);
  IIndirectFitDataTableModel *m_model;

private:
  virtual int workspaceIndexColumn() const;
  virtual int startXColumn() const;
  virtual int endXColumn() const;
  virtual int excludeColumn() const;
  double getDouble(FitDomainIndex row, int column) const;
  QString getText(FitDomainIndex row, int column) const;
  void setModelStartXAndEmit(double startX, FitDomainIndex row);
  void setModelEndXAndEmit(double endX, FitDomainIndex row);
  void setModelExcludeAndEmit(const std::string &exclude, FitDomainIndex row);

  void setColumnValues(int column, const QString &value);
  void setHorizontalHeaders(const QStringList &headers);

  DataPositionType m_dataPositions;
  QTableWidget *m_dataTable;
  bool m_emitCellChanged = true;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
