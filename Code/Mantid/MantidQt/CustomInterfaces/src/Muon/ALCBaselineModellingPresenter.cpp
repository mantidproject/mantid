#include "MantidQtCustomInterfaces/Muon/ALCBaselineModellingPresenter.h"

namespace MantidQt
{
namespace CustomInterfaces
{

  ALCBaselineModellingPresenter::ALCBaselineModellingPresenter(IALCBaselineModellingView* view)
    : m_view(view)
  {}
    
  ALCBaselineModellingPresenter::~ALCBaselineModellingPresenter()
  {
  }

} // namespace CustomInterfaces
} // namespace MantidQt
