#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogAstra.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogSavu.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogCustom.h"

namespace MantidQt {
namespace CustomInterfaces {

TomoToolConfigDialogBase *
TomoToolConfigDialogBase::getCorrectDialogForToolFromString(
    const std::string &toolName) {

  if (toolName == "TomoPy") {
    return new TomoToolConfigDialogTomoPy;
  }
  if (toolName == "Astra") {
    return new TomoToolConfigDialogAstra;
  }
  if (toolName == "Savu") {
    return new TomoToolConfigDialogSavu;
  }
  if (toolName == "Custom command") {
    return new TomoToolConfigDialogCustom;
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
