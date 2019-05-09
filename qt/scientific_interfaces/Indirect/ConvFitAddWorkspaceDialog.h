// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_CONVFITADDWORKSPACEDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_CONVFITADDWORKSPACEDIALOG_H_

#include "IAddWorkspaceDialog.h"
#include "ui_ConvFitAddWorkspaceDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class ConvFitAddWorkspaceDialog : public IAddWorkspaceDialog {
  Q_OBJECT
public:
  explicit ConvFitAddWorkspaceDialog(QWidget *parent);

  std::string workspaceName() const override;
  std::string resolutionName() const;
  std::string workspaceIndices() const;

  void setWSSuffices(const QStringList &suffices) override;
  void setFBSuffices(const QStringList &suffices) override;
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);

  void updateSelectedSpectra() override;

private slots:
  void selectAllSpectra(int state);
  void workspaceChanged(const QString &workspaceName);

private:
  void setWorkspace(const std::string &workspace);
  void setAllSpectraSelectionEnabled(bool doEnable);

  Ui::ConvFitAddWorkspaceDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_CONVFITADDWORKSPACEDIALOG_H_ */
