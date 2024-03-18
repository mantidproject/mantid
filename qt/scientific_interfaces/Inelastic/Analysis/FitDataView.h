// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_FitDataView.h"

#include "IFitDataView.h"

#include "DllConfig.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <QTabWidget>

namespace MantidQt {
namespace MantidWidgets {
class IAddWorkspaceDialog;
}
namespace CustomInterfaces {
namespace IDA {

class IIndirectFitDataPresenter;

class MANTIDQT_INELASTIC_DLL IndirectFitDataView : public QTabWidget, public IIndirectFitDataView {
  Q_OBJECT
public:
  IndirectFitDataView(QWidget *parent);
  ~IndirectFitDataView() override = default;

  void subscribePresenter(IIndirectFitDataPresenter *presenter) override;

  QTableWidget *getDataTable() const override;
  bool isTableEmpty() const override;

  UserInputValidator &validate(UserInputValidator &validator) override;
  virtual void addTableEntry(size_t row, FitDataRow newRow) override;
  virtual void updateNumCellEntry(double numEntry, size_t row, size_t column) override;
  int getColumnIndexFromName(std::string const &ColName) override;
  void clearTable() override;
  QString getText(int row, int column) const override;
  QModelIndexList getSelectedIndexes() const override;
  bool dataColumnContainsText(std::string const &columnText) const override;

  void setSampleWSSuffices(const QStringList &suffices) override;
  void setSampleFBSuffices(const QStringList &suffices) override;
  void setResolutionWSSuffices(const QStringList &suffices) override;
  void setResolutionFBSuffices(const QStringList &suffices) override;

  void displayWarning(const std::string &warning) override;

protected slots:
  void notifyAddData(MantidWidgets::IAddWorkspaceDialog *dialog);

protected:
  IndirectFitDataView(const QStringList &headers, QWidget *parent);

  std::unique_ptr<Ui::IndirectFitDataView> m_uiForm;
  void setCell(std::unique_ptr<QTableWidgetItem> cell, size_t row, size_t column);

  QStringList m_wsSampleSuffixes;
  QStringList m_fbSampleSuffixes;
  QStringList m_wsResolutionSuffixes;
  QStringList m_fbResolutionSuffixes;

  IIndirectFitDataPresenter *m_presenter;

protected slots:
  virtual void showAddWorkspaceDialog();

private slots:
  void notifyRemoveClicked();
  void notifyUnifyClicked();
  void notifyCellChanged(int row, int column);

private:
  QStringList m_HeaderLabels;
  void setHorizontalHeaders(const QStringList &headers);
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
