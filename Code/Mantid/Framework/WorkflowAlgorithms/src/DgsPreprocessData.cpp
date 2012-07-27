/*WIKI*

This algorithm is responsible for normalising data via a given incident beam
parameter. For SNS, monitor workspaces need to be passed.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsPreprocessData.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "Poco/Path.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsPreprocessData)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsPreprocessData::DgsPreprocessData()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsPreprocessData::~DgsPreprocessData()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsPreprocessData::name() const { return "DgsPreprocessData"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsPreprocessData::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsPreprocessData::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsPreprocessData::initDocs()
  {
    this->setWikiSummary("Preprocess data via an incident beam parameter.");
    this->setOptionalMessage("Preprocess data via an incident beam parameter.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsPreprocessData::init()
  {
    this->declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
        Direction::Input), "An input workspace.");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "",
        Direction::Output, PropertyMode::Optional), "An output workspace.");
    this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
        Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsPreprocessData::exec()
  {
    g_log.notice() << "Starting DgsPreprocessData" << std::endl;
    // Get the reduction property manager
    const std::string reductionManagerName = this->getProperty("ReductionProperties");
    boost::shared_ptr<PropertyManager> reductionManager;
    if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
      {
        reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
      }
    else
      {
        throw std::runtime_error("DgsPreprocessData cannot run without a reduction PropertyManager.");
      }

    this->enableHistoryRecordingForChild(true);

    MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
    // Make output workspace name the same as input workspace
    const std::string outWsName = inputWS->getName();

    std::string incidentBeamNorm = reductionManager->getProperty("IncidentBeamNormalisation");
    g_log.notice() << "Incident beam norm method = " << incidentBeamNorm << std::endl;
    if ("None" != incidentBeamNorm)
      {
        const std::string facility = ConfigService::Instance().getFacility().name();
        // SNS hard-wired to current normalisation
        if ("SNS" == facility)
          {
            incidentBeamNorm = "ByCurrent";
          }
        const std::string normAlg = "Normalise" + incidentBeamNorm;
        IAlgorithm_sptr norm = this->createSubAlgorithm(normAlg);
        norm->setAlwaysStoreInADS(true);
        norm->setProperty("InputWorkspace", outWsName);
        norm->setProperty("OutputWorkspace", outWsName);
        if ("ToMonitor" == incidentBeamNorm)
          {
            // Perform extra setup for monitor normalisation
            const double rangeOffset = reductionManager->getProperty("TofRangeOffset");
            double rangeMin = reductionManager->getProperty("MonitorIntRangeLow");
            if (EMPTY_DBL() == rangeMin)
              {
                rangeMin = inputWS->getInstrument()->getNumberParameter("norm-mon1-min")[0];
              }
            rangeMin += rangeOffset;
            double rangeMax = reductionManager->getProperty("MonitorIntRangeHigh");
            if (EMPTY_DBL() == rangeMin)
              {
                rangeMax = inputWS->getInstrument()->getNumberParameter("norm-mon1-max")[0];
              }
            rangeMax += rangeOffset;
            specid_t monSpec = static_cast<specid_t>(inputWS->getInstrument()->getNumberParameter("norm-mon1-spec")[0]);
            if ("ISIS" == facility)
              {
                norm->setProperty("MonitorSpectrum", monSpec);
              }
            // Do SNS
            else
              {
                MatrixWorkspace_const_sptr monitorWS = reductionManager->getProperty("MonitorWorkspace");
                if (!monitorWS)
                  {
                    throw std::runtime_error("SNS instruments require monitor workspaces for monitor normalisation.");
                  }
                std::size_t monIndex = monitorWS->getIndexFromSpectrumNumber(monSpec);
                norm->setProperty("MonitorWorkspace", monitorWS);
                norm->setProperty("MonitorWorkspaceIndex", monIndex);
              }
            norm->setProperty("IntegrationRangeMin", rangeMin);
            norm->setProperty("IntegrationRangeMax", rangeMax);
            norm->setProperty("IncludePartialBins", true);
          }
        norm->execute();
      }

    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
    this->setProperty("OutputWorkspace", outputWS);
  }

} // namespace Mantid
} // namespace WorkflowAlgorithms
