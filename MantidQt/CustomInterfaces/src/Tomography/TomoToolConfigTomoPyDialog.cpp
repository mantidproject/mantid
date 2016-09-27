#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigTomoPyDialog.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigTomoPyDialog::TomoToolConfigTomoPyDialog(QWidget *parent)
    : TomoToolConfigDialogBase(parent) {}

void TomoToolConfigTomoPyDialog::setUpDialog() {
  m_tomoPyUi.setupUi(this);
  m_tomoPyUi.comboBox_method->clear();
}

int TomoToolConfigTomoPyDialog::execute() {
  // TODO enum?
  return this->exec();
}
} // Custominterfaces
} // MantidQt