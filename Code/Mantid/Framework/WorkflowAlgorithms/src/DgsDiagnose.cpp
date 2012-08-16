/*WIKI*

This algorithm is responsible for setting up the necessary workspaces to
hand off to the DetectorDiagnostic algorithm.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsDiagnose.h"
#include "MantidAPI/PropertyManagerDataService.h"

#include <boost/pointer_cast.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsDiagnose)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsDiagnose::DgsDiagnose()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsDiagnose::~DgsDiagnose()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsDiagnose::name() const { return "DgsDiagnose"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsDiagnose::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsDiagnose::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsDiagnose::initDocs()
  {
    this->setWikiSummary("Setup and run DetectorDiagnostic.");
    this->setOptionalMessage("Setup and run DetectorDiagnostic.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsDiagnose::init()
  {
    this->declareProperty(new WorkspaceProperty<>("DetVanWorkspace", "",
        Direction::Input), "The detector vanadium workspace.");
    this->declareProperty(new WorkspaceProperty<>("DetVanCompWorkspace", "",
        Direction::Input, PropertyMode::Optional),
        "A detector vanadium workspace to compare against the primary one.");
    this->declareProperty(new WorkspaceProperty<>("SampleWorkspace", "",
        Direction::Input, PropertyMode::Optional),
        "A sample workspace to run some diagnostics on.");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace", "",
        Direction::Output), "This is the resulting mask workspace.");
    this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
        Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsDiagnose::exec()
  {
    g_log.notice() << "Starting DgsDiagnose" << std::endl;
    // Get the reduction property manager
    const std::string reductionManagerName = this->getProperty("ReductionProperties");
    boost::shared_ptr<PropertyManager> reductionManager;
    if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
      {
        reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
      }
    else
      {
        throw std::runtime_error("DgsDiagnose cannot run without a reduction PropertyManager.");
      }

    this->enableHistoryRecordingForChild(true);
    std::string maskName = this->getProperty("OutputWorkspace");
    if (maskName.empty())
      {
        maskName = "det_van_mask";
      }

    // Gather all the necessary properties
    MatrixWorkspace_sptr detVanWS = this->getProperty("DetVanWorkspace");
    MatrixWorkspace_sptr detVanCompWS = this->getProperty("DetVanCompWorkspace");
    MatrixWorkspace_sptr sampleWS;

    // Boolean properties
    const bool checkBkg = reductionManager->getProperty("BackgroundCheck");
    const bool rejectZeroBkg = reductionManager->getProperty("RejectZeroBackground");
    const bool createPsdBleed = reductionManager->getProperty("PsdBleed");

    // Numeric properties
    const double huge = reductionManager->getProperty("HighCounts");
    const double tiny = reductionManager->getProperty("LowCounts");
    const double vanHi = reductionManager->getProperty("MedianTestHigh");
    const double vanLo = reductionManager->getProperty("MedianTestLow");
    const double vanSigma = reductionManager->getProperty("ErrorBarCriterion");
    const double variation = reductionManager->getProperty("DetVanRatioVariation");
    const double samHi = reductionManager->getProperty("SamBkgMedianTestHigh");
    const double samLo = reductionManager->getProperty("SamBkgMedianTestLow");
    const double samSigma = reductionManager->getProperty("SamBkgErrorBarCriterion");
    const double bleedRate = reductionManager->getProperty("MaxFramerate");
    const int bleedPixels = reductionManager->getProperty("IgnoredPixels");

    // Make some internal names for workspaces
    const std::string dvInternal = "_det_van";
    const std::string dvCompInternal = "_det_van_comp";
    const std::string sampleInternal = "_sample";
    const std::string bkgInternal = "_background_int";
    const std::string countsInternal = "_total_counts";

    // Clone the incoming workspace
    IAlgorithm_sptr cloneWs = this->createSubAlgorithm("CloneWorkspace");
    cloneWs->setProperty("InputWorkspace", detVanWS);
    cloneWs->executeAsSubAlg();
    // CloneWorkspace returns Workspace_sptr
    Workspace_sptr tmp = cloneWs->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr dvWS = boost::static_pointer_cast<MatrixWorkspace>(tmp);
    dvWS->setName(dvInternal);

    // Process the detector vanadium
    IAlgorithm_sptr detVan = this->createSubAlgorithm("DgsProcessDetectorVanadium");
    detVan->setProperty("InputWorkspace", dvWS);
    detVan->setProperty("ReductionProperties", reductionManagerName);
    detVan->executeAsSubAlg();
    dvWS.reset();
    dvWS = detVan->getProperty("OutputWorkspace");

    // Process the comparison detector vanadium workspace if present
    MatrixWorkspace_sptr dvCompWS;
    if (detVanCompWS)
      {
        detVan->setProperty("InputWorkspace", detVanCompWS);
        detVan->executeAsSubAlg();
        dvCompWS = detVan->getProperty("OutputWorkspace");
      }
    else
      {
        dvCompWS = boost::shared_ptr<MatrixWorkspace>();
      }

    // Process the sample data if any of the sample checks are requested.
    if (checkBkg || rejectZeroBkg || createPsdBleed)
      {
        sampleWS = this->getProperty("SampleWorkspace");

        IAlgorithm_sptr cloneWs = this->createSubAlgorithm("CloneWorkspace");
        cloneWs->setProperty("InputWorkspace", sampleWS);
        cloneWs->executeAsSubAlg();
        Workspace_sptr tmp = cloneWs->getProperty("OutputWorkspace");
        sampleWS = boost::static_pointer_cast<MatrixWorkspace>(tmp);
        sampleWS->setName(sampleInternal);

        IAlgorithm_sptr norm = this->createSubAlgorithm("DgsPreprocessData");
        norm->setProperty("InputWorkspace", sampleWS);
        norm->setProperty("ReductionProperties", reductionManagerName);
        norm->executeAsSubAlg();
        sampleWS.reset();
        tmp = cloneWs->getProperty("OutputWorkspace");
        sampleWS = boost::static_pointer_cast<MatrixWorkspace>(tmp);
      }

    // Create the total counts workspace if necessary
    MatrixWorkspace_sptr totalCountsWS;
    if (rejectZeroBkg)
      {
        IAlgorithm_sptr integrate = this->createSubAlgorithm("Integration");
        integrate->setProperty("InputWorkspace", sampleWS);
        integrate->setPropertyValue("OutputWorkspace", countsInternal);
        integrate->setProperty("IncludePartialBins", true);
        integrate->executeAsSubAlg();
        g_log.warning() << "Total Counts being used" << std::endl;
        totalCountsWS = integrate->getProperty("OutputWorkspace");
      }
    else
      {
        g_log.warning() << "Total Counts NOT being used" << std::endl;
        totalCountsWS = boost::shared_ptr<MatrixWorkspace>();
      }

    // Create the background workspace if necessary
    MatrixWorkspace_sptr backgroundIntWS;
    if (checkBkg)
      {
        const double rangeStart = reductionManager->getProperty("BackgroundTofStart");
        const double rangeEnd = reductionManager->getProperty("BackgroundTofEnd");

        IAlgorithm_sptr integrate = this->createSubAlgorithm("Integration");
        integrate->setProperty("InputWorkspace", sampleWS);
        integrate->setPropertyValue("OutputWorkspace", bkgInternal);
        integrate->setProperty("RangeLower", rangeStart);
        integrate->setProperty("RangeUpper", rangeEnd);
        integrate->setProperty("IncludePartialBins", true);
        integrate->executeAsSubAlg();
        backgroundIntWS = integrate->getProperty("OutputWorkspace");

        IAlgorithm_sptr cvu = this->createSubAlgorithm("ConvertUnits");
        cvu->setProperty("InputWorkspace", backgroundIntWS);
        cvu->setProperty("OutputWorkspace", backgroundIntWS);
        cvu->setProperty("Target", "Energy");
        cvu->executeAsSubAlg();
        backgroundIntWS = cvu->getProperty("OutputWorkspace");

        // What is this magic value !?!?!?!?
        backgroundIntWS *= 1.7016e8;

        // Normalise the background integral workspace
        if (dvCompWS)
          {

            MatrixWorkspace_sptr hmean = 2.0 * dvWS * dvCompWS;
            hmean /= (dvWS + dvCompWS);
            backgroundIntWS /= hmean;
          }
        else
          {
            backgroundIntWS /= dvWS;
          }
      }
    else
      {
        backgroundIntWS = boost::shared_ptr<MatrixWorkspace>();
      }

    // Handle case where one of the other tests (checkBkg or rejectZeroBkg)
    // are requested, but not createPsdBleed.
    if (!createPsdBleed)
      {
        sampleWS = boost::shared_ptr<MatrixWorkspace>();
      }

    IAlgorithm_sptr diag = this->createSubAlgorithm("DetectorDiagnostic");
    diag->setProperty("InputWorkspace", dvWS);
    diag->setProperty("DetVanCompare", dvCompWS);
    diag->setProperty("SampleWorkspace", sampleWS);
    diag->setProperty("SampleTotalCountsWorkspace", totalCountsWS);
    diag->setProperty("SampleBackgroundWorkspace", backgroundIntWS);
    diag->setProperty("LowThreshold", tiny);
    diag->setProperty("HighThreshold", huge);
    diag->setProperty("LowThresholdFraction", vanLo);
    diag->setProperty("HighThresholdFraction", vanHi);
    diag->setProperty("SignificanceTest", vanSigma);
    diag->setProperty("DetVanRatioVariation", variation);
    diag->setProperty("SampleBkgLowAcceptanceFactor", samLo);
    diag->setProperty("SampleBkgHighAcceptanceFactor", samHi);
    diag->setProperty("SampleBkgSignificanceTest", samSigma);
    diag->setProperty("MaxTubeFramerate", bleedRate);
    diag->setProperty("NIgnoredCentralPixels", bleedPixels);
    diag->setProperty("OutputWorkspace", maskName);
    diag->executeAsSubAlg();

    MatrixWorkspace_sptr maskWS = diag->getProperty("OutputWorkspace");

    // Cleanup
    dvWS.reset();
    dvCompWS.reset();
    sampleWS.reset();
    totalCountsWS.reset();
    backgroundIntWS.reset();

    int numMasked = diag->getProperty("NumberOfFailures");
    g_log.information() << "Number of masked pixels = " << numMasked << std::endl;
    this->setProperty("OutputWorkspace", maskWS);
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
