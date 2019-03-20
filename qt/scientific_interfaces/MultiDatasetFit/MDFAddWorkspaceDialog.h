// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MDFADDWORKSPACEDIALOG_H_
#define MDFADDWORKSPACEDIALOG_H_

#include "ui_MDFAddWorkspaceDialog.h"
#include <QDialog>

namespace MantidQt {
namespace CustomInterfaces {
namespace MDF {

/**
 * A dialog for selecting a workspace from the ADS.
 */
class AddWorkspaceDialog : public QDialog {
  Q_OBJECT
public:
  explicit AddWorkspaceDialog(QWidget *parent);
  QString workspaceName() const { return m_workspaceName; }
  std::vector<int> workspaceIndices() const { return m_wsIndices; }
private slots:
  void accept() override;
  void reject() override;
  void workspaceNameChanged(const QString &);
  void selectAllSpectra(int state);

private:
  QStringList availableWorkspaces() const;
  void findCommonMaxIndex(const QString &wsName);
  /// Name of the selected workspace
  QString m_workspaceName;
  /// Selected workspace index
  std::vector<int> m_wsIndices;
  /// Maximum index in the selected workspace
  int m_maxIndex;
  Ui::MDFAddWorkspaceDialog m_uiForm;
};

} // namespace MDF
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /*MDFADDWORKSPACEDIALOG_H_*/
