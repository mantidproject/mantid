#include "MantidQtCustomInterfaces/TomoReconstruction/TomoToolConfigDialog.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigTomoPy::TomoToolConfigTomoPy(QWidget *parent) : QDialog(parent) {}

TomoToolConfigSavu::TomoToolConfigSavu(QWidget *parent) : QMainWindow(parent) {}

TomoToolConfigAstra::TomoToolConfigAstra(QWidget *parent) : QDialog(parent) {}

TomoToolConfigCustom::TomoToolConfigCustom(QWidget *parent) : QDialog(parent) {}

TomoToolConfigDialog::TomoToolConfigDialog(QWidget *parent) : QDialog(parent) {
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

  layout = new QGridLayout();
  layout->addLayout(hRun, 0, 0);
  layout->addLayout(hOpt, 1, 0);

  connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void TomoToolConfigDialog::okClicked() {}

void TomoToolConfigDialog::cancelClicked() {}

} // namespace CustomInterfaces
} // namespace MantidQt
