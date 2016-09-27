#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigTomoPyDialog.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigTomoPyDialog::TomoToolConfigTomoPyDialog(QWidget *parent)
    : TomoToolConfigDialogBase(parent) {}

void TomoToolConfigTomoPyDialog::setUpDialog() {
  m_tomoPyUi.setupUi(this);
  m_tomoPyUi.comboBox_method->clear();

  ToolConfigTomoPy tool;
  std::vector<std::pair<std::string, std::string > > methods = {{"one", "one"}, {"two","two"}, {"three","three"}};
  for (size_t i = 0; i < methods.size(); i++) {
    m_tomoPyUi.comboBox_method->addItem(
        QString::fromStdString(methods[i].second));
  }
}

int TomoToolConfigTomoPyDialog::execute() {
  // TODO enum?
  return this->exec();
}
} // Custominterfaces
} // MantidQt
