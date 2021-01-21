// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IAddWorkspaceDialog.h"
#include "ui_FqFitAddWorkspaceDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class FqFitAddWorkspaceDialog : public IAddWorkspaceDialog {
  Q_OBJECT
public:
  explicit FqFitAddWorkspaceDialog(QWidget *parent);

  std::string workspaceName() const override;
  std::string parameterType() const;
  int parameterNameIndex() const;

  void setParameterTypes(const std::vector<std::string> &types);
  void setParameterNames(const std::vector<std::string> &names);
  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;

  void enableParameterSelection();
  void disableParameterSelection();

  void updateSelectedSpectra() override{};

public slots:
  void emitWorkspaceChanged(const QString &name);
  void emitParameterTypeChanged(const QString &index);

signals:
  void workspaceChanged(FqFitAddWorkspaceDialog *dialog, const std::string &workspace);
  void parameterTypeChanged(FqFitAddWorkspaceDialog *dialog, const std::string &type);

private:
  Ui::FqFitAddWorkspaceDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
