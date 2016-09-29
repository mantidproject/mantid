#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_

#include "ui_TomoToolConfigAstra.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigAstraDialog : public QDialog, public TomoToolConfigDialogBase {
  Q_OBJECT
public:
  TomoToolConfigAstraDialog(QWidget *parent = 0);
  ~TomoToolConfigAstraDialog();

  void setUpDialog() override;
  int execute() override;

private slots:
  void okClicked();
  void cancelClicked();

private:
  Ui::TomoToolConfigAstra m_astraUi;

  QLabel *labelRun, *labelOpt;
  QLineEdit *editRun, *editOpt;
  QHBoxLayout *hRun, *hOpt, *hBut;
  QGridLayout *layout;
  QPushButton *okButton, *cancelButton;
};

} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGASTRADIALOG_H_
