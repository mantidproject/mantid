#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigTomoPyDialog.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigAstraDialog.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigSavuDialog.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigCustomDialog.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigDialogBase *
TomoToolConfigDialogBase::getCorrectDialogForToolFromString(
    const std::string &toolName) {

  if (toolName == "TomoPy") {
    return new TomoToolConfigTomoPyDialog;
  }
  if (toolName == "Astra") {
    return new TomoToolConfigAstraDialog;
  }
  if (toolName == "Savu") {
    return new TomoToolConfigSavuDialog;
  }
  if (toolName == "Custom command") {
    return new TomoToolConfigCustomDialog;
  }

  return nullptr;
}

int TomoToolConfigDialogBase::execute() {
  int res = this->executeQt();
  this->handleDialogResult(res);
  return res;
}

/** If user clicked OK, it will run setupToolConfig()
*/
void TomoToolConfigDialogBase::handleDialogResult(int result) {
  if (QDialog::Accepted == result) {
    setupToolConfig();
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt
