#include "MantidQtCustomInterfaces/Muon/ALCDataLoading.h"

namespace Mantid
{
namespace CustomInterfaces
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ALCDataLoading::ALCDataLoading(IALCDataLoadingView* view)
    : m_view(view)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ALCDataLoading::~ALCDataLoading()
  {
  }

  void ALCDataLoading::initialize()
  {
  }

} // namespace CustomInterfaces
} // namespace Mantid
