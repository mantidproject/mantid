#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigTomoPyDialog.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigTomoPyDialog::TomoToolConfigTomoPyDialog(QWidget *parent)
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

TomoToolConfigTomoPyDialog::~TomoToolConfigTomoPyDialog() {}

void TomoToolConfigTomoPyDialog::setUpDialog() {
  m_tomoPyUi.setupUi(this);
  m_tomoPyUi.comboBox_method->clear();

  const auto methods = ToolConfigTomoPy::methods();
  for (size_t i = 0; i < methods.size(); i++) {
    m_tomoPyUi.comboBox_method->addItem(
        QString::fromStdString(methods[i].second));
  }
}

int TomoToolConfigTomoPyDialog::execute() {
  // TODO enum?
  return this->exec();
}

void TomoToolConfigTomoPyDialog::okClicked() {}

void TomoToolConfigTomoPyDialog::cancelClicked() {}

} // Custominterfaces
} // MantidQt
