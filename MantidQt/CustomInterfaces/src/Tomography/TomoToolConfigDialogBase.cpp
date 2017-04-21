#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogBase.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogTomoPy.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogAstra.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogSavu.h"
#include "MantidQtCustomInterfaces/Tomography/TomoToolConfigDialogCustom.h"

#include "MantidKernel/make_unique.h"

namespace MantidQt {
namespace CustomInterfaces {

std::unique_ptr<TomoToolConfigDialogBase>
TomoToolConfigDialogBase::getToolDialogFor(const std::string &toolName) {

  if (toolName == "TomoPy") {
    return Mantid::Kernel::make_unique<TomoToolConfigDialogTomoPy>();
  } else if (toolName == "Astra") {
    return Mantid::Kernel::make_unique<TomoToolConfigDialogAstra>();
  } else if (toolName == "Savu") {
    return Mantid::Kernel::make_unique<TomoToolConfigDialogSavu>();
  } else if (toolName == "Custom command") {
    return Mantid::Kernel::make_unique<TomoToolConfigDialogCustom>();
  }
  // tool doesn't exist, return nullptr
  return nullptr;
}

void TomoToolConfigDialogBase::handleDialogResult(const int result) {
  if (QDialog::Accepted == result) {
    // setup the new settings if the user has Accepted
    setupMethodSelected();
    setupToolSettingsFromPaths();
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt
