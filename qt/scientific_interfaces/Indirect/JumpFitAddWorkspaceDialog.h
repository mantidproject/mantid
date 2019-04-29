// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_JUMPFITADDWORKSPACEDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_JUMPFITADDWORKSPACEDIALOG_H_

#include "IAddWorkspaceDialog.h"
#include "ui_JumpFitAddWorkspaceDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class JumpFitAddWorkspaceDialog : public IAddWorkspaceDialog {
  Q_OBJECT
public:
  explicit JumpFitAddWorkspaceDialog(QWidget *parent);

  std::string workspaceName() const override;
  std::string parameterType() const;
  int parameterNameIndex() const;

  void setParameterTypes(const std::vector<std::string> &types);
  void setParameterNames(const std::vector<std::string> &names);
  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;

  void enableParameterSelection();
  void disableParameterSelection();

public slots:
  void emitWorkspaceChanged(const QString &name);
  void emitParameterTypeChanged(const QString &index);

signals:
  void workspaceChanged(JumpFitAddWorkspaceDialog *dialog,
                        const std::string &workspace);
  void parameterTypeChanged(JumpFitAddWorkspaceDialog *dialog,
                            const std::string &type);

private:
  Ui::JumpFitAddWorkspaceDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_JUMPFITADDWORKSPACEDIALOG_H_ */
