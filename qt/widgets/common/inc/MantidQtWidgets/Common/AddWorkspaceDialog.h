// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_AddWorkspaceDialog.h"

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>

namespace MantidQt {
namespace MantidWidgets {

/**
 * A dialog for selecting a workspace from the ADS.
 */
class EXPORT_OPT_MANTIDQT_COMMON AddWorkspaceDialog : public QDialog {
  Q_OBJECT

public:
  explicit AddWorkspaceDialog(QWidget *parent);
  QString workspaceName() const { return m_workspaceName; }
  std::vector<int> workspaceIndices() const { return m_wsIndices; }

public:
  /// Testing accessors
  QComboBox *workspaceNameComboBox() const { return m_uiForm.cbWorkspaceName; }
  QLineEdit *workspaceIndiceLineEdit() const { return m_uiForm.leWSIndices; }

public slots:
  void accept() override;

private slots:
  void reject() override;
  void workspaceNameChanged(const QString & /*wsName*/);
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
  Ui::AddWorkspaceDialog m_uiForm;
};

} // namespace MantidWidgets
} // namespace MantidQt
