#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_

#include "ui_TomoToolConfigTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigTomoPyDialog : public QDialog, public TomoToolConfigDialogBase {
  Q_OBJECT

public:
  TomoToolConfigTomoPyDialog(QWidget *parent = 0);
  ~TomoToolConfigTomoPyDialog() override;

  void setUpDialog() override;
  int execute() override;

private slots:
  void okClicked();
  void cancelClicked();

private:
  Ui::TomoToolConfigTomoPy m_tomoPyUi;

  QLabel *labelRun, *labelOpt;
  QLineEdit *editRun, *editOpt;
  QHBoxLayout *hRun, *hOpt, *hBut;
  QGridLayout *layout;
  QPushButton *okButton, *cancelButton;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGTOMOPYDIALOG_H_
