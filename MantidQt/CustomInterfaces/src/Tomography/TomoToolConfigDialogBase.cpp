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
  std::cout << toolName << '\n';
  // TODO move to global STRINGS from View!

  auto *pp = new QDialog;
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
//	  throw Mantid::Kernel::Exception::NotFoundError(
//		  "Selected tool dialog not found!", toolName);

  return nullptr;
}

int TomoToolConfigDialogBase::execute() {
  // TODO enum?
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
