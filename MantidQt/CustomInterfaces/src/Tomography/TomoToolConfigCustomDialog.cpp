#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigCustomDialog.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoToolConfigCustomDialog::DEFAULT_TOOL_NAME =
    "Custom command";

// custom tool doesn't have a default method
const std::string TomoToolConfigCustomDialog::DEFAULT_TOOL_METHOD = "";

std::string TomoToolConfigCustomDialog::m_backupCommandLine = "";

void TomoToolConfigCustomDialog::setupToolConfig() {

  // None of the other paths matter, because the user could've changed
  // them, so ignore them and load the current ones on the dialogue
  QString run = m_customUi.lineEdit_runnable->text();        // current path
  QString opts = m_customUi.textEdit_cl_opts->toPlainText(); // current commands

  // update the settings with the newest information
  m_tempSettings = std::shared_ptr<ToolConfigCustom>(
      new ToolConfigCustom(run.toStdString(), opts.toStdString()));
}

void TomoToolConfigCustomDialog::setupDialogUi() {
  m_customUi.setupUi(this);

  // sets the correct runnable path, overriding the default one
  m_customUi.lineEdit_runnable->setText(QString::fromStdString(m_runPath));

  if (m_backupCommandLine != "") {
    m_customUi.textEdit_cl_opts->setText(
        QString::fromStdString(m_backupCommandLine));
  }

  // get default options from command line
  QString opts = m_customUi.textEdit_cl_opts->toPlainText();

  // create the default settings
  m_tempSettings = std::shared_ptr<ToolConfigCustom>(
      new ToolConfigCustom(m_runPath, opts.toStdString()));
}

void TomoToolConfigCustomDialog::handleDialogResult(int result) {
  if (QDialog::Accepted == result) {
    // if accepted we want to save the information
    m_backupCommandLine =
        m_customUi.textEdit_cl_opts->toPlainText().toStdString();
    setupToolConfig();
  }
}
int TomoToolConfigCustomDialog::executeQt() { return this->exec(); }
} // CustomInterfaces
} // MantidQt