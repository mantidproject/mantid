// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "ui_EditResultsDialog.h"

#include <QDialog>
#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class EditResultsDialog : public QDialog {
  Q_OBJECT

public:
  explicit EditResultsDialog(QWidget *parent);

  void setWorkspaceSelectorSuffices(QStringList const &suffices);

  std::string getSelectedInputWorkspaceName() const;
  std::string getSelectedSingleFitWorkspaceName() const;
  std::string getOutputWorkspaceName() const;

  void setReplaceFitResultText(QString const &text);
  void setReplaceFitResultEnabled(bool enable);

signals:
  void replaceSingleFitResult();

private slots:
  void setOutputWorkspaceName();

private:
  Ui::EditResultsDialog m_uiForm;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
