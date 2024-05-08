// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "ui_AddWorkspaceDialog.h"

#include <vector>

#include <QDialog>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON AddWorkspaceDialog : public QDialog, public IAddWorkspaceDialog {
  Q_OBJECT
public:
  explicit AddWorkspaceDialog(QWidget *parent);

  std::string workspaceName() const override;
  FunctionModelSpectra workspaceIndices() const;

  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;

  void updateSelectedSpectra() override;

  std::string getFileName() const;

signals:
  void addData(MantidWidgets::IAddWorkspaceDialog *dialog);

private slots:
  void selectAllSpectra(int state);
  void workspaceChanged(const QString &workspaceName);
  void emitAddData();
  void handleAutoLoaded();

private:
  void setWorkspace(const std::string &workspace);
  void setAllSpectraSelectionEnabled(bool doEnable);

  Ui::AddWorkspaceDialog m_uiForm;
};

} // namespace MantidWidgets
} // namespace MantidQt
