#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  ALCPeakFittingPresenter::ALCPeakFittingPresenter(IALCPeakFittingView* view)
    : m_view(view)
  {}

  void ALCPeakFittingPresenter::initialize()
  {
    m_view->initialize();
  }

} // namespace CustomInterfaces
} // namespace MantidQt
