#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigTomoPyDialog.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigDialogBase::TomoToolConfigDialogBase(QWidget *parent)
    : QDialog(parent) {
  labelRun = new QLabel("Runnable script");
  editRun = new QLineEdit("/work/imat/");
  hRun = new QHBoxLayout();
  hRun->addWidget(labelRun);
  hRun->addWidget(editRun);

  labelOpt = new QLabel("Command line options");
  editOpt = new QLineEdit("/work/imat");
  hOpt = new QHBoxLayout();
  hOpt->addWidget(labelOpt);

  hOpt->addWidget(editOpt);

  okButton = new QPushButton("Ok");
  cancelButton = new QPushButton("Cancel");
  hBut = new QHBoxLayout();
  hBut->insertStretch(0, 1);
  hBut->addWidget(okButton);
  hBut->addWidget(cancelButton);

  layout = new QGridLayout();
  layout->addLayout(hRun, 0, 0);
  layout->addLayout(hOpt, 1, 0);
  layout->addLayout(hOpt, 2, 0);

  connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void TomoToolConfigDialogBase::okClicked() {}

void TomoToolConfigDialogBase::cancelClicked() {}

TomoToolConfigDialogBase *
TomoToolConfigDialogBase::fromString(const std::string &toolName) {
  if (toolName == "") {
    return new TomoToolConfigTomoPyDialog();
  }
  return nullptr;
}
} // namespace CustomInterfaces
} // namespace MantidQt
