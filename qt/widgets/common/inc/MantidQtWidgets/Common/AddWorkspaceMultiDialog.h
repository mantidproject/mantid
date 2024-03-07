// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "ui_AddWorkspaceMultiDialog.h"

#include <vector>

#include <QDialog>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON AddWorkspaceMultiDialog : public QDialog, public IAddWorkspaceDialog {
  Q_OBJECT
public:
  explicit AddWorkspaceMultiDialog(QWidget *parent);

  std::string workspaceName() const override;
  stringPairVec selectedNameIndexPairs() const;

  bool isEmpty() const;
  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;
  void setup();

signals:
  void addData(MantidWidgets::IAddWorkspaceDialog *dialog);

private slots:
  void updateSelectedSpectra() override;
  void selectAllSpectra();
  void unifyRange();
  void handleFilesFound();
  void emitAddData();

private:
  Ui::AddWorkspaceMultiDialog m_uiForm;
};

} // namespace MantidWidgets
} // namespace MantidQt
