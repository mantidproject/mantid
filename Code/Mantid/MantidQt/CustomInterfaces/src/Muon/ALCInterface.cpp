#include "MantidQtCustomInterfaces/Muon/ALCInterface.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface);

  void ALCInterface::initLayout()
  {
    new ALCDataLoadingView(this);
  }

} // namespace CustomInterfaces
} // namespace MantidQt
