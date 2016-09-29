#ifndef MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_

#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"
#include "ui_TomoToolConfigCustom.h"

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGridLayout>

namespace MantidQt {
namespace CustomInterfaces {
class TomoToolConfigCustomDialog : public QDialog, public TomoToolConfigDialogBase {
  Q_OBJECT
public:
  TomoToolConfigCustomDialog(QWidget *parent = 0);
  ~TomoToolConfigCustomDialog() override;

  void setUpDialog() override;
  int execute() override;

  private slots:
  void okClicked();
  void cancelClicked();

private:
	Ui::TomoToolConfigCustom m_customUi;

	QLabel *labelRun, *labelOpt;
	QLineEdit *editRun, *editOpt;
	QHBoxLayout *hRun, *hOpt, *hBut;
	QGridLayout *layout;
	QPushButton *okButton, *cancelButton;
};
} // CustomInterfaces
} // MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_TOMOTOOLCONFIGCUSTOMDIALOG_H_
