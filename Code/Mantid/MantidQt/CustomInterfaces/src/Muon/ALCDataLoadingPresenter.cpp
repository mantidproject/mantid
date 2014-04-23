#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include <Poco/ActiveResult.h>

#include <QApplication>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{
  ALCDataLoadingPresenter::ALCDataLoadingPresenter(IALCDataLoadingView* view)
    : m_view(view)
  {}

  void ALCDataLoadingPresenter::initialize()
  {
    m_view->initialize();

    connect(m_view, SIGNAL(loadRequested()), SLOT(load()));
  }

  void ALCDataLoadingPresenter::load()
  {
    try
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
      alg->setChild(true); // Don't want workspaces in the ADS
      alg->setProperty("FirstRun", m_view->firstRun());
      alg->setProperty("LastRun", m_view->lastRun());
      alg->setProperty("LogValue", m_view->log());
      alg->setPropertyValue("OutputWorkspace", "__NotUsed__");

      Poco::ActiveResult<bool> result(alg->executeAsync());

      while (!result.available())
      {
        QApplication::processEvents(); // So that progress bar gets updated
      }

      m_loadedData = alg->getProperty("OutputWorkspace");
      assert(m_loadedData->getNumberHistograms() == 1); // PlotAsymmetryByLogValue guarantees that
      m_view->setDataCurve(*(ALCHelper::curveDataFromWs(m_loadedData, 0)));
    }
    catch(std::exception& e)
    {
      m_view->displayError(e.what());
    }
  }
} // namespace CustomInterfaces
} // namespace MantidQt
