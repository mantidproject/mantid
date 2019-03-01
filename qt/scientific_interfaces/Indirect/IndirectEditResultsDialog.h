// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTADDWORKSPACEDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTADDWORKSPACEDIALOG_H_

#include "ui_IndirectEditResultsDialog.h"

#include <QDialog>
#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class IndirectEditResultsDialog : public QDialog {
  Q_OBJECT

public:
  explicit IndirectEditResultsDialog(QWidget *parent);

  void setWorkspaceSelectorSuffices(QStringList const &suffices);

  std::string getSelectedInputWorkspaceName() const;
  std::string getSelectedSingleFitWorkspaceName() const;
  std::string getOutputWorkspaceName() const;

  void setReplaceFitResultText(QString const &text);
  void setReplaceFitResultEnabled(bool enable);

signals:
  void replaceSingleFitResult();
  void closeDialog();

private slots:
  void setOutputWorkspaceName();

private:
  Ui::IndirectEditResultsDialog m_uiForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTADDWORKSPACEDIALOG_H_ */
