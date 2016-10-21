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

void TomoToolConfigDialogBase::handleDialogResult(int result) {
  if (QDialog::Accepted == result) {
    // setup the new settings if the user has Accepted
	  setupMethodSelected();
	  setupToolSettingsFromPaths();
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt
