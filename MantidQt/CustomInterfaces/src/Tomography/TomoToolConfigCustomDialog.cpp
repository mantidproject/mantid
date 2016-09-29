#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigCustomDialog.h"
namespace MantidQt {
namespace CustomInterfaces {
TomoToolConfigCustomDialog::TomoToolConfigCustomDialog(QWidget *parent)
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

TomoToolConfigCustomDialog::~TomoToolConfigCustomDialog() {}

void TomoToolConfigCustomDialog::setUpDialog() { m_customUi.setupUi(this); }
int TomoToolConfigCustomDialog::execute() { return this->exec(); }

void TomoToolConfigCustomDialog::okClicked() {}

void TomoToolConfigCustomDialog::cancelClicked() {}

} // CustomInterfaces
} // MantidQt