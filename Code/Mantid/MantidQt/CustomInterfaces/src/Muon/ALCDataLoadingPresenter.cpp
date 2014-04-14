#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"

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

    connect(m_view, SIGNAL(loadData()), SLOT(loadData()));
  }

  void ALCDataLoadingPresenter::loadData()
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
      m_view->displayData(m_loadedData);
    }
    catch(std::exception& e)
    {
      m_view->displayError(e.what());
    }
  }
} // namespace CustomInterfaces
} // namespace MantidQt
