/*WIKI*

This is the top-level workflow algorithm for direct geometry spectrometer
data reduction. This algorithm is responsible for gathering the necessary
parameters and generating calls to other workflow or standard algorithms.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsReduction.h"
#include "MantidWorkflowAlgorithms/DgsConvertToEnergyTransfer.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VisibleWhenProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace WorkflowAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(DgsReduction)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DgsReduction::DgsReduction()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DgsReduction::~DgsReduction()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string DgsReduction::name() const { return "DgsReduction"; };
  
  /// Algorithm's version for identification. @see Algorithm::version
  int DgsReduction::version() const { return 1; };
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string DgsReduction::category() const { return "Workflow\\Inelastic"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void DgsReduction::initDocs()
  {
    this->setWikiSummary("Top-level workflow algorithm for DGS reduction.");
    this->setOptionalMessage("Top-level workflow algorithm for DGS reduction.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void DgsReduction::init()
  {
    // Sample setup options
    std::string sampleSetup = "Sample Setup";
    this->declareProperty(new FileProperty("SampleFile", "",
        FileProperty::OptionalLoad, "_event.nxs"),
        "File containing the data to reduce");
    this->declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
        Direction::Input, PropertyMode::Optional),
        "Workspace to be reduced");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    this->declareProperty("IncidentEnergyGuess", EMPTY_DBL(), mustBePositive,
      "Set the value of the incident energy guess in meV.");
    this->declareProperty("UseIncidentEnergyGuess", false,
        "Use the incident energy guess as the actual value (will not be calculated).");
    this->declareProperty(new ArrayProperty<double>("EnergyTransferRange",
        boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary.\n"
      "Negative width value indicates logarithmic binning.");
    this->declareProperty("SofPhiEIsDistribution", true,
        "The final S(Phi, E) data is made to be a distribution.");
    this->declareProperty("HardMaskFile", "", "A file or workspace containing a hard mask.");
    this->declareProperty("GroupingFile", "", "A file containing grouping (mapping) information.");

    this->setPropertyGroup("SampleFile", sampleSetup);
    this->setPropertyGroup("InputWorkspace", sampleSetup);
    this->setPropertyGroup("IncidentEnergyGuess", sampleSetup);
    this->setPropertyGroup("UseIncidentEnergyGuess", sampleSetup);
    this->setPropertyGroup("EnergyTransferRange", sampleSetup);
    this->setPropertyGroup("SofPhiEIsDistribution", sampleSetup);
    this->setPropertyGroup("HardMaskFile", sampleSetup);
    this->setPropertyGroup("GroupingFile", sampleSetup);

    // Data corrections
    std::string dataCorr = "Data Corrections";
    this->declareProperty("FilterBadPulses", false, "If true, filter bad pulses from data.");
    std::vector<std::string> incidentBeamNormOptions;
    incidentBeamNormOptions.push_back("None");
    incidentBeamNormOptions.push_back("ByCurrent");
    incidentBeamNormOptions.push_back("ToMonitor");
    this->declareProperty("IncidentBeamNormalisation", "None",
        boost::make_shared<StringListValidator>(incidentBeamNormOptions),
        "Options for incident beam normalisation on data.");
    this->declareProperty("MonitorIntRangeLow", EMPTY_DBL(),
        "Set the lower bound for monitor integration.");
    this->setPropertySettings("MonitorIntRangeLow",
        new VisibleWhenProperty("IncidentBeamNormalisation", IS_EQUAL_TO, "ToMonitor"));
    this->declareProperty("MonitorIntRangeHigh", EMPTY_DBL(),
           "Set the upper bound for monitor integration.");
    this->setPropertySettings("MonitorIntRangeHigh",
        new VisibleWhenProperty("IncidentBeamNormalisation", IS_EQUAL_TO, "ToMonitor"));
    this->declareProperty("TimeIndepBackgroundSub", false,
        "If true, time-independent background will be calculated and removed.");
    this->declareProperty("TibTofRangeStart", EMPTY_DBL(),
        "Set the lower TOF bound for time-independent background subtraction.");
    this->setPropertySettings("TibTofRangeStart",
        new VisibleWhenProperty("TimeIndepBackgroundSub", IS_EQUAL_TO, "1"));
    this->declareProperty("TibTofRangeEnd", EMPTY_DBL(),
        "Set the upper TOF bound for time-independent background subtraction.");
    this->setPropertySettings("TibTofRangeEnd",
        new VisibleWhenProperty("TimeIndepBackgroundSub", IS_EQUAL_TO, "1"));
    this->declareProperty("DetectorVanadium", "", "Run numbers, files or workspaces of detector vanadium data.");
    this->declareProperty("UseBoundsForDetVan", false,
        "If true, integrate the detector vanadium over a given range.");
    this->declareProperty("DetVanIntRangeLow", EMPTY_DBL(),
        "Set the lower bound for integrating the detector vanadium.");
    this->setPropertySettings("DetVanIntRangeLow",
        new VisibleWhenProperty("UseBoundsForDetVan", IS_EQUAL_TO, "1"));
    this->declareProperty("DetVanIntRangeHigh", EMPTY_DBL(),
        "Set the upper bound for integrating the detector vanadium.");
    this->setPropertySettings("DetVanIntRangeHigh",
        new VisibleWhenProperty("UseBoundsForDetVan", IS_EQUAL_TO, "1"));
    std::vector<std::string> detvanIntRangeUnits;
    detvanIntRangeUnits.push_back("Energy");
    detvanIntRangeUnits.push_back("Wavelength");
    this->declareProperty("DetVanIntRangeUnits", "Energy",
        boost::make_shared<StringListValidator>(detvanIntRangeUnits),
        "Options for the units on the detector vanadium integration.");
    this->setPropertySettings("DetVanIntRangeUnits",
        new VisibleWhenProperty("UseBoundsForDetVan", IS_EQUAL_TO, "1"));

    this->setPropertyGroup("FilterBadPulses", dataCorr);
    this->setPropertyGroup("IncidentBeamNormalisation", dataCorr);
    this->setPropertyGroup("MonitorIntRangeLow", dataCorr);
    this->setPropertyGroup("MonitorIntRangeHigh", dataCorr);
    this->setPropertyGroup("TimeIndepBackgroundSub", dataCorr);
    this->setPropertyGroup("TibTofRangeStart", dataCorr);
    this->setPropertyGroup("TibTofRangeEnd", dataCorr);
    this->setPropertyGroup("DetectorVanadium", dataCorr);
    this->setPropertyGroup("UseBoundsForDetVan", dataCorr);
    this->setPropertyGroup("DetVanIntRangeLow", dataCorr);
    this->setPropertyGroup("DetVanIntRangeHigh", dataCorr);
    this->setPropertyGroup("DetVanIntRangeUnits", dataCorr);

    // Finding bad detectors
    std::string findBadDets = "Finding Bad Detectors";
    this->declareProperty("FindBadDetectors", false,
        "If true, run all of the detector diagnostics tests and create a mask.");
    this->declareProperty("OutputMaskFile", "",
        "The output mask file name used for the results of the detector tests.");
    this->setPropertySettings("OutputMaskFile",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("ErrorBarCriterion", 3.3, mustBePositive,
        "Some selection criteria for the detector tests.");
    this->setPropertySettings("ErrorBarCriterion",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("DetectorVanadium1", "",
        "The detector vanadium file to run the tests on.");
    this->setPropertySettings("DetectorVanadium1",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("HighCounts", 1.e+10, mustBePositive,
        "Mask detectors above this threshold.");
    this->setPropertySettings("HighCounts",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("LowCounts", 1.e-10, mustBePositive,
        "Mask detectors below this threshold.");
    this->setPropertySettings("LowCounts",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("MedianTestHigh", 3.0, mustBePositive,
        "Mask detectors above this threshold.");
    this->setPropertySettings("MedianTestHigh",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("MedianTestLow", 0.1, mustBePositive,
        "Mask detectors below this threshold.");
    this->setPropertySettings("MedianTestLow",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("DetectorVanadium2", "",
        "The detector vanadium to check against for time variations.");
    this->setPropertySettings("DetectorVanadium2",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("ProptionalChangeCriterion", 1.1, mustBePositive,
        "Mask detectors if the time variation is above this threshold.");
    this->setPropertySettings("ProptionalChangeCriterion",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("BackgroundCheck", false,
        "If true, run a background check on detector vanadium.");
    this->setPropertySettings("BackgroundCheck",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("AcceptanceFactor", 5.0, mustBePositive,
        "Mask detectors above this threshold.");
    this->setPropertySettings("AcceptanceFactor",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    auto mustBeIntPositive = boost::make_shared<BoundedValidator<size_t> >();
    mustBeIntPositive->setLower(0);
    size_t tof_start = 18000;
    this->declareProperty("BackgroundTofStart", tof_start, mustBeIntPositive,
        "Start TOF for the background check.");
    this->setPropertySettings("BackgroundTofStart",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    size_t tof_end = 19500;
    this->declareProperty("BackgroundTofEnd", tof_end, mustBeIntPositive,
        "End TOF for the background check.");
    this->setPropertySettings("BackgroundTofEnd",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("RejectZeroBackground", true,
        "If true, check the background region for anomolies.");
    this->setPropertySettings("RejectZeroBackground",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->setPropertySettings("RejectZeroBackground",
        new VisibleWhenProperty("BackgroundCheck", IS_EQUAL_TO, "1"));
    this->declareProperty("PsdBleed", false, "If true, perform a PSD bleed test.");
    this->setPropertySettings("PsdBleed",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->declareProperty("MaxFramerate", "", "The maximum framerate to check.");
    this->setPropertySettings("MaxFramerate",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->setPropertySettings("MaxFramerate",
        new VisibleWhenProperty("PsdBleed", IS_EQUAL_TO, "1"));
    this->declareProperty("IgnoredPixels", "",
        "A list of pixels to ignore in the calculations.");
    this->setPropertySettings("IgnoredPixels",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    this->setPropertySettings("IgnoredPixels",
        new VisibleWhenProperty("PsdBleed", IS_EQUAL_TO, "1"));

    this->setPropertyGroup("FindBadDetectors", findBadDets);
    this->setPropertyGroup("OutputMaskFile", findBadDets);
    this->setPropertyGroup("ErrorBarCriterion", findBadDets);
    this->setPropertyGroup("DetectorVanadium1", findBadDets);
    this->setPropertyGroup("HighCounts", findBadDets);
    this->setPropertyGroup("LowCounts", findBadDets);
    this->setPropertyGroup("MedianTestHigh", findBadDets);
    this->setPropertyGroup("MedianTestLow", findBadDets);
    this->setPropertyGroup("DetectorVanadium2", findBadDets);
    this->setPropertyGroup("ProptionalChangeCriterion", findBadDets);
    this->setPropertyGroup("BackgroundCheck", findBadDets);
    this->setPropertyGroup("AcceptanceFactor", findBadDets);
    this->setPropertyGroup("BackgroundTofStart", findBadDets);
    this->setPropertyGroup("BackgroundTofEnd", findBadDets);
    this->setPropertyGroup("RejectZeroBackground", findBadDets);
    this->setPropertyGroup("PsdBleed", findBadDets);
    this->setPropertyGroup("MaxFramerate", findBadDets);
    this->setPropertyGroup("IgnoredPixels", findBadDets);

    // Absolute units correction
    std::string absUnitsCorr = "Absolute Units Correction";
    this->declareProperty("DoAbsoluteUnits", false,
        "If true, perform an absolute units normalisation.");
    this->declareProperty("AbsUnitsVanadium" "",
        "The vanadium file used as the sample in the absolute units normalisation.");
    this->setPropertySettings("AbsUnitsVanadium",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("AbsUnitsGroupingFile", "",
        "Grouping file for absolute units normalisation.");
    this->setPropertySettings("AbsUnitsGroupingFile",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("AbsUnitsDetectorVanadium", "",
        "The detector vanadium file used in the absolute units normalisation.");
    this->setPropertySettings("AbsUnitsDetectorVanadium",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("AbsUnitsIncidentEnergy", EMPTY_DBL(), mustBePositive,
        "The incident energy for the vanadium sample.");
    this->setPropertySettings("AbsUnitsIncidentEnergy",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("AbsUnitsMinimumEnergy", -1.0,
        "The minimum energy for the integration range.");
    this->setPropertySettings("AbsUnitsMinimumEnergy",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("AbsUnitsMaximumEnergy", 1.0,
        "The maximum energy for the integration range.");
    this->setPropertySettings("AbsUnitsMaximumEnergy",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("VanadiumMass", 32.58, "The mass of vanadium.");
    this->setPropertySettings("VanadiumMass",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("SampleMass", 1.0, "The mass of sample.");
    this->setPropertySettings("SampleMass",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    this->declareProperty("SampleRmm", 1.0, "The rmm of sample.");
    this->setPropertySettings("SampleRmm",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));

    this->setPropertyGroup("DoAbsoluteUnits", absUnitsCorr);
    this->setPropertyGroup("AbsUnitsVanadium", absUnitsCorr);
    this->setPropertyGroup("AbsUnitsGroupingFile", absUnitsCorr);
    this->setPropertyGroup("AbsUnitsDetectorVanadium", absUnitsCorr);
    this->setPropertyGroup("AbsUnitsIncidentEnergy", absUnitsCorr);
    this->setPropertyGroup("AbsUnitsMinimumEnergy", absUnitsCorr);
    this->setPropertyGroup("AbsUnitsMaximumEnergy", absUnitsCorr);
    this->setPropertyGroup("VanadiumMass", absUnitsCorr);
    this->setPropertyGroup("SampleMass", absUnitsCorr);
    this->setPropertyGroup("SampleRmm", absUnitsCorr);

    this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
        Direction::Output);
  }

  /**
   * Create a workspace by either loading a file or using an existing
   * workspace.
   */
  Workspace_sptr DgsReduction::loadInputData(boost::shared_ptr<PropertyManager> manager)
  {

    Workspace_sptr inputWS;

    std::string inputData = this->getPropertyValue("SampleFile");
    const std::string inputWSName = this->getPropertyValue("InputWorkspace");
    if (!inputWSName.empty() && !inputData.empty())
      {
        throw std::runtime_error("DgsReduction: Either the Filename property or InputWorkspace property must be provided, NOT BOTH");
      }
    else if (!inputWSName.empty())
      {
        inputWS = this->load(inputWSName);
      }
    else if (!inputData.empty())
      {
        const std::string facility = ConfigService::Instance().getFacility().name();
        if ("SNS" == facility)
          {
            if (boost::ends_with(inputData, "_event.nxs"))
              {
                this->setLoadAlg("LoadEventNexus");
              }
            if (boost::ends_with(inputData, "_neutron_event.dat"))
              {
                this->setLoadAlg("LoadEventPreNexus");
                this->setLoadAlgFileProp("EventFilename");
              }
            manager->declareProperty(new PropertyWithValue<std::string>("MonitorFilename", inputData));
          }
        // Do ISIS
        else
          {
            this->setLoadAlg("LoadRaw");
            manager->declareProperty(new PropertyWithValue<std::string>("DetCalFilename", inputData));
          }

        inputWS = this->load(inputData);
      }
    else
      {
        throw std::runtime_error("DgsReduction: Either the Filename property or InputWorkspace property must be provided");
      }

    return inputWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsReduction::exec()
  {
    // Reduction property manager
    const std::string reductionManagerName = this->getProperty("ReductionProperties");
    if (reductionManagerName.empty())
    {
      g_log.error() << "ERROR: Reduction Property Manager name is empty" << std::endl;
      return;
    }
    boost::shared_ptr<PropertyManager> reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);

    Workspace_sptr inputWS = this->loadInputData(reductionManager);

    // Setup for the convert to energy transfer workflow algorithm
    const double incidentEnergyGuess = this->getProperty("IncidentEnergyGuess");
    const bool useEiGuess = this->getProperty("UseIncidentEnergyGuess");
    const std::vector<double> etBinning = this->getProperty("EnergyTransferRange");
    const bool sofphieIsDistribution = this->getProperty("SofPhiEIsDistribution");
    const std::string incidentBeamNormType = this->getProperty("IncidentBeamNormalisation");
    const double monRangeLow = this->getProperty("MonitorIntRangeLow");
    const double monRangeHigh = this->getProperty("MonitorIntRangeHigh");
    const bool tibSubtraction = this->getProperty("TimeIndepBackgroundSub");
    const double tibTofStart = this->getProperty("TibTofRangeStart");
    const double tibTofEnd = this->getProperty("TibTofRangeEnd");

    IAlgorithm_sptr etConv = this->createSubAlgorithm("DgsConvertToEnergyTransfer");
    etConv->setProperty("InputWorkspace", inputWS);
    etConv->setProperty("IncidentEnergyGuess", incidentEnergyGuess);
    etConv->setProperty("UseIncidentEnergyGuess", useEiGuess);
    etConv->setProperty("EnergyTransferRange", etBinning);
    etConv->setProperty("SofPhiEIsDistribution", sofphieIsDistribution);
    etConv->setProperty("IncidentBeamNormalisation", incidentBeamNormType);
    etConv->setProperty("MonitorIntRangeLow", monRangeLow);
    etConv->setProperty("MonitorIntRangeHigh", monRangeHigh);
    etConv->setProperty("TimeIndepBackgroundSub", tibSubtraction);
    etConv->setProperty("TibTofRangeStart", tibTofStart);
    etConv->setProperty("TibTofRangeEnd", tibTofEnd);
    etConv->execute();
  }

} // namespace Mantid
} // namespace WorkflowAlgorithms
