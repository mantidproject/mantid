#include "MantidQtCustomInterfaces/Muon/ALCDataLoading.h"

namespace MantidQt
{
namespace CustomInterfaces
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ALCDataLoading::ALCDataLoading(IALCDataLoadingView* view)
    : m_view(view)
  {}
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ALCDataLoading::~ALCDataLoading()
  {}

  void ALCDataLoading::initialize()
  {
    connectView();
  }

  void ALCDataLoading::connectView()
  {
    connect(m_view, SIGNAL(loadData()), SLOT(loadData()));
  }

  void ALCDataLoading::loadData()
  {
    m_view->setData(MatrixWorkspace_const_sptr());
  }

} // namespace CustomInterfaces
} // namespace Mantid
