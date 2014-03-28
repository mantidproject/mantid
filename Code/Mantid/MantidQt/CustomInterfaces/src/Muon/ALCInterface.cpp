#include "MantidQtCustomInterfaces/Muon/ALCInterface.h"

#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingView.h"

namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(ALCInterface);

  void ALCInterface::initLayout()
  {
    auto ws = AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>("ALCWorkspace");
    auto view = new ALCBaselineModellingView(this, ws);
    view->initialize();
  }

} // namespace CustomInterfaces
} // namespace MantidQt
