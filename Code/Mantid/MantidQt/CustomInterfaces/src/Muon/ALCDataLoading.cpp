#include "MantidQtCustomInterfaces/Muon/ALCDataLoading.h"

#include "MantidAPI/AlgorithmManager.h"

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
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
    alg->setChild(true); // Don't want workspaces in the ADS
    alg->setProperty("FirstRun", m_view->firstRun());
    alg->setProperty("LastRun", m_view->lastRun());
    alg->setProperty("LogValue", m_view->log());
    alg->setPropertyValue("OutputWorkspace", "__NotUsed__");
    alg->execute();

    MatrixWorkspace_const_sptr result = alg->getProperty("OutputWorkspace");
    m_view->setData(result);
  }

} // namespace CustomInterfaces
} // namespace Mantid
