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
