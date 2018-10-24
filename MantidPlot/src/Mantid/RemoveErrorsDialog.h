// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REMOVEERRORSDIALOG_H
#define REMOVEERRORSDIALOG_H

#include <QDialog>

namespace Ui {
class RemoveErrorsDialog;
}

class RemoveErrorsDialog : public QDialog {
  Q_OBJECT
public:
  explicit RemoveErrorsDialog(QWidget *parent = nullptr);
  ~RemoveErrorsDialog() override;

  //! Supply the dialog with a curves list
  void setCurveNames(const QStringList &names);

signals:

  void curveName(const QString &);

protected slots:

  void remove();

private:
  Ui::RemoveErrorsDialog *ui;
};

#endif // REMOVEERRORSDIALOG_H
