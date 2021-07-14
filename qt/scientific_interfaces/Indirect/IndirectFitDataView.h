// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_IndirectFitDataView.h"

#include "IIndirectFitDataView.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QTabWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitDataView : public IIndirectFitDataView {
  Q_OBJECT
public:
  IndirectFitDataView(QWidget *parent = nullptr);
  ~IndirectFitDataView() override = default;

  QTableWidget *getDataTable() const override;
  bool isTableEmpty() const;

  UserInputValidator &validate(UserInputValidator &validator) override;
  virtual void addTableEntry(size_t row, FitDataRow newRow) override;
  virtual int workspaceIndexColumn() const override;
  virtual int startXColumn() const override;
  virtual int endXColumn() const override;
  virtual int excludeColumn() const override;
  void clearTable() override;
  QString getText(int row, int column) const override;
  QModelIndexList getSelectedIndexes() const override;

public slots:
  void displayWarning(const std::string &warning) override;

protected:
  IndirectFitDataView(const QStringList &headers, QWidget *parent = nullptr);
  std::unique_ptr<Ui::IndirectFitDataView> m_uiForm;
  void setCell(std::unique_ptr<QTableWidgetItem> cell, size_t row, size_t column);

private:
  void setHorizontalHeaders(const QStringList &headers);
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
