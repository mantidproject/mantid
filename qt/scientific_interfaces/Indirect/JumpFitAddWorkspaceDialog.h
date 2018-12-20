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
  int parameterNameIndex() const;

  void setParameterTypes(const std::vector<std::string> &types);
  void setParameterNames(const std::vector<std::string> &names);
  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;

  void enableParameterSelection();
  void disableParameterSelection();

public slots:
  void emitWorkspaceChanged(const QString &name);
  void emitParameterTypeChanged(int index);

signals:
  void workspaceChanged(JumpFitAddWorkspaceDialog *dialog,
                        const std::string &workspace);
  void parameterTypeChanged(JumpFitAddWorkspaceDialog *dialog, int type);

private:
  Ui::JumpFitAddWorkspaceDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_JUMPFITADDWORKSPACEDIALOG_H_ */
