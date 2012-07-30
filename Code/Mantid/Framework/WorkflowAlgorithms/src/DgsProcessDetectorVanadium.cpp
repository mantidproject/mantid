/*WIKI*

This algorithm is responsible for processing the detector vanadium in the form
required for the sample data normalisation in the convert to energy transfer
process.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsProcessDetectorVanadium.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsProcessDetectorVanadium)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsProcessDetectorVanadium::DgsProcessDetectorVanadium()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsProcessDetectorVanadium::~DgsProcessDetectorVanadium()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsProcessDetectorVanadium::name() const { return "DgsProcessDetectorVanadium"; }
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsProcessDetectorVanadium::version() const { return 1; }
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsProcessDetectorVanadium::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsProcessDetectorVanadium::initDocs()
  {
    this->setWikiSummary("Algorithm to process detector vanadium.");
    this->setOptionalMessage("Algorithm to process detector vanadium.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsProcessDetectorVanadium::init()
  {
    this->declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
                          "An input workspace containing the detector vanadium data in TOF units.");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
                          "The workspace containing the processed results.");
    this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
            Direction::Output);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsProcessDetectorVanadium::exec()
  {
    g_log.notice() << "Starting DgsProcessDetectorVanadium" << std::endl;
    // Get the reduction property manager
    const std::string reductionManagerName = this->getProperty("ReductionProperties");
    boost::shared_ptr<PropertyManager> reductionManager;
    if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
      {
        reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
      }
    else
      {
        throw std::runtime_error("DgsProcessDetectorVanadium cannot run without a reduction PropertyManager.");
      }

    this->enableHistoryRecordingForChild(true);

    MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");
    const std::string inWsName = inputWS->getName();
    const std::string outWsName = inWsName + "idetvan";

    // Normalise result workspace to incident beam parameter
    IAlgorithm_sptr norm = this->createSubAlgorithm("DgsPreprocessData");
    norm->setAlwaysStoreInADS(true);
    norm->setProperty("InputWorkspace", inWsName);
    norm->setProperty("OutputWorkspace", outWsName);
    norm->execute();

    double detVanIntRangeLow = reductionManager->getProperty("DetVanIntRangeLow");
    if (EMPTY_DBL() == detVanIntRangeLow)
      {
        detVanIntRangeLow = inputWS->getInstrument()->getNumberParameter("wb-integr-min")[0];
      }
    double detVanIntRangeHigh = reductionManager->getProperty("DetVanIntRangeHigh");
    if (EMPTY_DBL() == detVanIntRangeHigh)
      {
        detVanIntRangeHigh = inputWS->getInstrument()->getNumberParameter("wb-integr-max")[0];
      }
    const std::string detVanIntRangeUnits = reductionManager->getProperty("DetVanInRangeUnits");

    // Convert the data to the appropriate units
    IAlgorithm_sptr cnvun = this->createSubAlgorithm("ConvertUnits");
    cnvun->setAlwaysStoreInADS(true);
    cnvun->setProperty("InputWorkspace", outWsName);
    cnvun->setProperty("OutputWorkspace", outWsName);
    cnvun->setProperty("Target", detVanIntRangeUnits);
    cnvun->setProperty("EMode", "Elastic");
    cnvun->execute();

    // Rebin the data (not Integration !?!?!?)
    std::vector<double> binning;
    binning.push_back(detVanIntRangeLow);
    binning.push_back(detVanIntRangeHigh - detVanIntRangeLow);
    binning.push_back(detVanIntRangeHigh);

    IAlgorithm_sptr rebin = this->createSubAlgorithm("Rebin");
    rebin->setAlwaysStoreInADS(true);
    rebin->setProperty("InputWorkspace", outWsName);
    rebin->setProperty("OutputWorkspace", outWsName);
    rebin->setProperty("Params", binning);
    rebin->execute();

    const std::string facility = ConfigService::Instance().getFacility().name();
    if ("ISIS" == facility)
      {
        // Scale results by a constant
        double wbScaleFactor = inputWS->getInstrument()->getNumberParameter("wb-scale-factor")[0];
        const std::string scaleFactorName = "WbScaleFactor";
        IAlgorithm_sptr csvw = this->createSubAlgorithm("CreateSingleValuedWorkspace");
        csvw->setAlwaysStoreInADS(true);
        csvw->setProperty("OutputWorkspace", scaleFactorName);
        csvw->setProperty("DataValue", wbScaleFactor);
        csvw->execute();

        IAlgorithm_sptr mult = this->createSubAlgorithm("Multiply");
        mult->setAlwaysStoreInADS(true);
        mult->setProperty("LHSWorkspace", outWsName);
        mult->setProperty("RHSWorkspace", scaleFactorName);
        mult->setProperty("OutputWorkspace", outWsName);
        mult->execute();
      }

    MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWsName);
    this->setProperty("OutputWorkspace", outputWS);
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
