#include "MantidQtCustomInterfaces/Muon/ALCDataLoadingPresenter.h"

#include "MantidAPI/AlgorithmManager.h"

#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <Poco/ActiveResult.h>

#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::API;

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
    connect(m_view, SIGNAL(firstRunSelected()), SLOT(updateAvailableInfo()));
  }

  void ALCDataLoadingPresenter::load()
  {
    m_view->setWaitingCursor();

    try
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
      alg->setChild(true); // Don't want workspaces in the ADS
      alg->setProperty("FirstRun", m_view->firstRun());
      alg->setProperty("LastRun", m_view->lastRun());
      alg->setProperty("LogValue", m_view->log());
      alg->setProperty("Function", m_view->function());
      alg->setProperty("Type", m_view->calculationType());
      alg->setProperty("DeadTimeCorrType",m_view->deadTimeType());
      alg->setProperty("Red",m_view->redPeriod());

      // If time limiting requested, set min/max times
      if (auto timeRange = m_view->timeRange())
      {
        double timeMin = (*timeRange).first;
        double timeMax = (*timeRange).second;
        if (timeMin>=timeMax) {
          throw std::invalid_argument("Invalid time limits");
        }
        alg->setProperty("TimeMin", timeMin);
        alg->setProperty("TimeMax", timeMax);
      }

      // If corrections from custom file requested, set file property
      if (m_view->deadTimeType() == "FromSpecifiedFile") {
        alg->setProperty("DeadTimeCorrFile",m_view->deadTimeFile());
      }

      // If custom grouping requested, set forward/backward groupings
      if ( m_view->detectorGroupingType() == "Custom" ) {
        alg->setProperty("ForwardSpectra",m_view->getForwardGrouping());
        alg->setProperty("BackwardSpectra",m_view->getBackwardGrouping());
      }

      // If Subtract checkbox is selected, set green period
      if ( m_view->subtractIsChecked() ) {
        alg->setProperty("Green",m_view->greenPeriod());
      }

      alg->setPropertyValue("OutputWorkspace", "__NotUsed");

      // Execute async so we can show progress bar
      Poco::ActiveResult<bool> result(alg->executeAsync());
      while( !result.available() )
      {
        QCoreApplication::processEvents();
      }
      if (!result.error().empty())
      {
        throw std::runtime_error(result.error());
      }

      m_loadedData = alg->getProperty("OutputWorkspace");

      // If errors are properly caught, shouldn't happen
      assert(m_loadedData);
      // If subtract is not checked, only one spectrum,
      // else four spectra
      if ( !m_view->subtractIsChecked() ) {
        assert(m_loadedData->getNumberHistograms() == 1);
      } else {
        assert(m_loadedData->getNumberHistograms() == 4);
      }

      // Plot spectrum 0. It is either red period (if subtract is unchecked) or 
      // red - green (if subtract is checked)
      m_view->setDataCurve(*(ALCHelper::curveDataFromWs(m_loadedData, 0)),
                           ALCHelper::curveErrorsFromWs(m_loadedData, 0));

    }
    catch(std::exception& e)
    {
      m_view->displayError(e.what());
    }

    m_view->restoreCursor();
  }

  void ALCDataLoadingPresenter::updateAvailableInfo()
  {
    Workspace_sptr loadedWs;

    try //... to load the first run
    {
      IAlgorithm_sptr load = AlgorithmManager::Instance().create("LoadMuonNexus");
      load->setChild(true); // Don't want workspaces in the ADS
      load->setProperty("Filename", m_view->firstRun());
      // We need logs only but we have to use LoadMuonNexus 
      // (can't use LoadMuonLogs as not all the logs would be 
      // loaded), so we load the minimum amount of data, i.e., one spectrum
      load->setPropertyValue("SpectrumMin","1");
      load->setPropertyValue("SpectrumMax","1");
      load->setPropertyValue("OutputWorkspace", "__NotUsed");
      load->execute();

      loadedWs = load->getProperty("OutputWorkspace");
    }
    catch(...)
    {
      m_view->setAvailableLogs(std::vector<std::string>()); // Empty logs list
      m_view->setAvailablePeriods(std::vector<std::string>()); // Empty period list
      m_view->setTimeLimits(0,0); // "Empty" time limits
      return;
    }

    // Set logs
    MatrixWorkspace_const_sptr ws = MuonAnalysisHelper::firstPeriod(loadedWs);
    std::vector<std::string> logs;

    const auto& properties = ws->run().getProperties();
    for(auto it = properties.begin(); it != properties.end(); ++it)
    {
      logs.push_back((*it)->name());
    }
    m_view->setAvailableLogs(logs);

    // Set periods
    size_t numPeriods = MuonAnalysisHelper::numPeriods(loadedWs);
    std::vector<std::string> periods;
    for (size_t i=0; i<numPeriods; i++)
    {
      std::stringstream buffer;
      buffer << i + 1;
      periods.push_back(buffer.str());
    }
    m_view->setAvailablePeriods(periods);

    // Set time limits
    m_view->setTimeLimits(ws->readX(0).front(),ws->readX(0).back());
    // Set allowed time range
    m_view->setTimeRange (ws->readX(0).front(),ws->readX(0).back());
  }

   MatrixWorkspace_sptr ALCDataLoadingPresenter::exportWorkspace()
  {
    if ( m_loadedData ) {

      return boost::const_pointer_cast<MatrixWorkspace>(m_loadedData);

    } else {

      return MatrixWorkspace_sptr();
    }
   }

   void ALCDataLoadingPresenter::setData(MatrixWorkspace_const_sptr data) {

     if (data) {
       // Set the data
       m_loadedData = data;
       // Plot the data
       m_view->setDataCurve(*(ALCHelper::curveDataFromWs(m_loadedData, 0)),
         ALCHelper::curveErrorsFromWs(m_loadedData, 0));

     } else {
       std::invalid_argument("Cannot load an empty workspace");
     }
   }
} // namespace CustomInterfaces
} // namespace MantidQt
