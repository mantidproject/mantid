#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTADDWORKSPACEDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTADDWORKSPACEDIALOG_H_

#include "IAddWorkspaceDialog.h"
#include "ui_IndirectAddWorkspaceDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class AddWorkspaceDialog : public IAddWorkspaceDialog {
  Q_OBJECT
public:
  explicit AddWorkspaceDialog(QWidget *parent);

  std::string workspaceName() const override;
  std::string workspaceIndices() const;

  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;

private slots:
  void selectAllSpectra(int state);
  void workspaceChanged(const QString &workspaceName);

private:
  void setWorkspace(const std::string &workspace);
  void setAllSpectraSelectionEnabled(bool doEnable);

  Ui::IndirectAddWorkspaceDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTADDWORKSPACEDIALOG_H_ */
