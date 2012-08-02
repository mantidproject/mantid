/*WIKI*

This algorithm is responsible for setting up the necessary workspaces to
hand off to the DetectorDiagnostic algorithm.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsDiagnose.h"
#include "MantidAPI/PropertyManagerDataService.h"

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
    const double vanSigma = reductionManager->getProperty("ErrorBarCriterion");
    const double huge = reductionManager->getProperty("HighCounts");
    const double tiny = reductionManager->getProperty("LowCounts");
    const double vanHi = reductionManager->getProperty("MedianTestHigh");
    const double vanLo = reductionManager->getProperty("MedianTestLow");
    const double variation = reductionManager->getProperty("ProptionalChangeCriterion");
    const double samHi = reductionManager->getProperty("AcceptanceFactor");
    const double samLo = 0.0;
    const double bleedRate = reductionManager->getProperty("MaxFramerate");
    const int bleedPixels = reductionManager->getProperty("IgnoredPixels");
    //reductionManager->getProperty();
    //reductionManager->getProperty();

    // Make some internal names for workspaces
    const std::string dvInternal = "__det_van";
    const std::string dvCompInternal = "__det_van_comp";
    const std::string sampleInternal = "__sample";
    const std::string bkgInternal = "__background_int";
    const std::string countsInternal = "__total_counts";

    // Process the detector vanadium
    IAlgorithm_sptr detVan = this->createSubAlgorithm("DgsProcessDetectorVanadium");
    detVan->setAlwaysStoreInADS(true);
    detVan->setProperty("InputWorkspace", detVanWS);
    detVan->setProperty("OutputWorkspace", dvInternal);
    detVan->setProperty("ReductionProperties", reductionManagerName);
    detVan->executeAsSubAlg();

    // Process the comparison detector vanadium workspace if present
    if (detVanCompWS)
      {
        detVan->setProperty("InputWorkspace", detVanCompWS);
        detVan->setProperty("OutputWorkspace", dvCompInternal);
        detVan->executeAsSubAlg();
      }

    AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
    // Get the processed workspaces for possible later use.
    MatrixWorkspace_sptr dvWS = dataStore.retrieveWS<MatrixWorkspace>(dvInternal);
    MatrixWorkspace_sptr dvCompWS;
    if (dataStore.doesExist(dvCompInternal))
      {
        dvCompWS = dataStore.retrieveWS<MatrixWorkspace>(dvCompInternal);
      }
    else
      {
        dvCompWS = boost::shared_ptr<MatrixWorkspace>();
      }

    // Process the sample data if any of the sample checks are requested.
    if (checkBkg || rejectZeroBkg || createPsdBleed)
      {
        sampleWS = this->getProperty("SampleWorkspace");

        IAlgorithm_sptr norm = this->createSubAlgorithm("DgsPreprocessData");
        norm->setAlwaysStoreInADS(true);
        norm->setProperty("InputWorkspace", sampleWS);
        norm->setProperty("OutputWorkspace", sampleInternal);
        norm->setProperty("ReductionProperties", reductionManagerName);
        norm->executeAsSubAlg();
      }

    // Handle case where one of the other tests (checkBkg or rejectZeroBkg)
    // are requested, but not createPsdBleed.
    if (createPsdBleed)
      {
        sampleWS = dataStore.retrieveWS<MatrixWorkspace>(sampleInternal);
      }
    else
      {
        sampleWS = boost::shared_ptr<MatrixWorkspace>();
      }

    // Create the total counts workspace if necessary
    MatrixWorkspace_sptr totalCountsWS;
    if (rejectZeroBkg)
      {
        IAlgorithm_sptr integrate = this->createSubAlgorithm("Integration");
        integrate->setAlwaysStoreInADS(true);
        integrate->setProperty("InputWorkspace", sampleInternal);
        integrate->setProperty("OutputWorkspace", countsInternal);
        integrate->setProperty("IncludePartialBins", true);
        integrate->executeAsSubAlg();

        totalCountsWS = integrate->getProperty("OutputWorkspace");
      }
    else
      {
        totalCountsWS = boost::shared_ptr<MatrixWorkspace>();
      }

    // Create the background workspace if necessary
    MatrixWorkspace_sptr backgroundIntWS;
    if (checkBkg)
      {
        const double rangeStart = reductionManager->getProperty("BackgroundTofStart");
        const double rangeEnd = reductionManager->getProperty("BackgroundTofEnd");

        IAlgorithm_sptr integrate = this->createSubAlgorithm("Integration");
        integrate->setAlwaysStoreInADS(true);
        integrate->setProperty("InputWorkspace", sampleInternal);
        integrate->setProperty("OutputWorkspace", bkgInternal);
        integrate->setProperty("RangeLower", rangeStart);
        integrate->setProperty("RangeUpper", rangeEnd);
        integrate->setProperty("IncludePartialBins", true);
        integrate->executeAsSubAlg();

        IAlgorithm_sptr cvu = this->createSubAlgorithm("ConvertUnits");
        cvu->setAlwaysStoreInADS(true);
        cvu->setProperty("InputWorkspace", bkgInternal);
        cvu->setProperty("OutputWorkspace", bkgInternal);
        cvu->setProperty("Target", "Energy");
        cvu->executeAsSubAlg();

        backgroundIntWS = dataStore.retrieveWS<MatrixWorkspace>(bkgInternal);
        // What is this magic value !?!?!?!?
        backgroundIntWS *= 1.7016e8;
        /*
        const std::string scaleFactor = "__sample_bkg_sf";
        IAlgorithm_sptr svw = this->createSubAlgorithm("CreateSingleValuedWorkspace");
        svw->setAlwaysStoreInADS(true);
        svw->setProperty("OutputWorkspace", scaleFactor);
        // What is this magic value !?!?!?!?
        svw->setProperty("DataValue", 1.7016e8);
        svw->executeAsSubAlg();

        IAlgorithm_sptr mult = this->createSubAlgorithm("Multiply");
        mult->setAlwaysStoreInADS(true);
        mult->setProperty("LHSWorkspace", bkgInternal);
        mult->setProperty("RHSWorkspace", scaleFactor);
        mult->setProperty("OutputWorkspace", bkgInternal);
        mult->executeAsSubAlg();
        */
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

    IAlgorithm_sptr diag = this->createSubAlgorithm("DetectorDiagnostic");
    diag->setAlwaysStoreInADS(true);
    diag->setProperty("InputWorkspace", dvInternal);
    diag->setProperty("WhiteBeamCompare", dvCompWS);
    diag->setProperty("SampleWorkspace", sampleWS);
    diag->setProperty("SampleTotalCountsWorkspace", totalCountsWS);
    diag->setProperty("SampleBackgroundWorkspace", backgroundIntWS);
    diag->setProperty("LowThreshold", tiny);
    diag->setProperty("HighThreshold", huge);
    diag->setProperty("SignificanceTest", vanSigma);
    diag->setProperty("LowThresholdFraction", vanLo);
    diag->setProperty("HighThresholdFraction", vanHi);
    diag->setProperty("WhiteBeamVariation", variation);
    diag->setProperty("SampleBkgLowAcceptanceFactor", samLo);
    diag->setProperty("SampleBkgHighAcceptanceFactor", samHi);
    diag->setProperty("MaxTubeFramerate", bleedRate);
    diag->setProperty("NIgnoredCentralPixels", bleedPixels);
    diag->setProperty("OutputWorkspace", maskName);
    //diag->setProperty("");
    diag->executeAsSubAlg();

    // Cleanup
    dataStore.remove(dvInternal);
    dataStore.remove(dvCompInternal);
    dataStore.remove(sampleInternal);
    dataStore.remove(countsInternal);
    dataStore.remove(bkgInternal);

    int numMasked = diag->getProperty("NumberOfFailures");
    g_log.information() << "Number of masked pixels = " << numMasked << std::endl;
  }

} // namespace WorkflowAlgorithms
} // namespace Mantid
