#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigCustomDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
TomoToolConfigCustomDialog::TomoToolConfigCustomDialog(QWidget *parent)
    : QDialog(parent) {}

TomoToolConfigCustomDialog::~TomoToolConfigCustomDialog() {}

void TomoToolConfigCustomDialog::setupToolConfig() {

  // None of the other paths really matter, because the user could've changed
  // them, so ignore them and load the current ones on the dialogue
  QString run = m_customUi.lineEdit_runnable->text();
  QString opts = m_customUi.textEdit_cl_opts->toPlainText();

  m_toolSettings.custom =
      ToolConfigCustom(run.toStdString(), opts.toStdString());
}

void TomoToolConfigCustomDialog::setupDialogUi() { m_customUi.setupUi(this); }

int TomoToolConfigCustomDialog::executeQt() { return this->exec(); }
} // CustomInterfaces
} // MantidQt