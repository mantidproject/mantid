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
    declareProperty(new FileProperty("SampleFile", "", FileProperty::OptionalLoad, "_event.nxs"),
        "File containing the data to reduce");
    declareProperty(new WorkspaceProperty<>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
        "Workspace to be reduced");
    //declareProperty("SampleData", "", "Run numbers, files or workspaces of the data sets to be reduced");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("IncidentEnergy", EMPTY_DBL(), mustBePositive,
      "Set the value of the incident energy in meV.");
    declareProperty("FixedIncidentEnergy", false,
        "Declare the value of the incident energy to be fixed (will not be calculated).");
    declareProperty(new ArrayProperty<double>("EnergyTransferRange",
        boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary.\n"
      "Negative width value indicates logarithmic binning.");
    declareProperty("SofPhiEIsDistribution", true,
        "The final S(Phi, E) data is made to be a distribution.");
    declareProperty("HardMaskFile", "", "A file or workspace containing a hard mask.");
    declareProperty("GroupingFile", "", "A file containing grouping (mapping) information.");
    declareProperty("FilterBadPulses", false, "If true, filter bad pulses from data.");
    std::vector<std::string> incidentBeamNormOptions;
    incidentBeamNormOptions.push_back("None");
    incidentBeamNormOptions.push_back("ByCurrent");
    incidentBeamNormOptions.push_back("ToMonitor");
    declareProperty("IncidentBeamNormalisation", "None",
        boost::make_shared<StringListValidator>(incidentBeamNormOptions),
        "Options for incident beam normalisation on data.");
    declareProperty("MonitorIntRangeLow", EMPTY_DBL(),
        "Set the lower bound for monitor integration.");
    setPropertySettings("MonitorIntRangeLow",
        new VisibleWhenProperty("IncidentBeamNormalisation", IS_EQUAL_TO, "ToMonitor"));
    declareProperty("MonitorIntRangeHigh", EMPTY_DBL(),
           "Set the upper bound for monitor integration.");
    setPropertySettings("MonitorIntRangeHigh",
        new VisibleWhenProperty("IncidentBeamNormalisation", IS_EQUAL_TO, "ToMonitor"));
    declareProperty("TimeIndepBackgroundSub", false,
        "If true, time-independent background will be calculated and removed.");
    declareProperty("TibTofRangeStart", EMPTY_DBL(),
        "Set the lower TOF bound for time-independent background subtraction.");
    setPropertySettings("TibTofRangeStart",
        new VisibleWhenProperty("TimeIndepBackgroundSub", IS_EQUAL_TO, "1"));
    declareProperty("TibTofRangeEnd", EMPTY_DBL(),
        "Set the upper TOF bound for time-independent background subtraction.");
    setPropertySettings("TibTofRangeEnd",
        new VisibleWhenProperty("TimeIndepBackgroundSub", IS_EQUAL_TO, "1"));
    declareProperty("DetectorVanadium", "", "Run numbers, files or workspaces of detector vanadium data.");
    declareProperty("UseBoundsForDetVan", false,
        "If true, integrate the detector vanadium over a given range.");
    declareProperty("DetVanIntRangeLow", EMPTY_DBL(),
        "Set the lower bound for integrating the detector vanadium.");
    setPropertySettings("DetVanIntRangeLow",
        new VisibleWhenProperty("UseBoundsForDetVan", IS_EQUAL_TO, "1"));
    declareProperty("DetVanIntRangeHigh", EMPTY_DBL(),
        "Set the upper bound for integrating the detector vanadium.");
    setPropertySettings("DetVanIntRangeHigh",
        new VisibleWhenProperty("UseBoundsForDetVan", IS_EQUAL_TO, "1"));
    std::vector<std::string> detvanIntRangeUnits;
    detvanIntRangeUnits.push_back("DeltaE");
    detvanIntRangeUnits.push_back("Wavelength");
    declareProperty("DetVanIntRangeUnits", "DeltaE",
        boost::make_shared<StringListValidator>(detvanIntRangeUnits),
        "Options for the units on the detector vanadium integration.");
    setPropertySettings("DetVanIntRangeUnits",
        new VisibleWhenProperty("UseBoundsForDetVan", IS_EQUAL_TO, "1"));
    declareProperty("FindBadDetectors", false,
        "If true, run all of the detector diagnostics tests and create a mask.");
    declareProperty("OutputMaskFile", "",
        "The output mask file name used for the results of the detector tests.");
    setPropertySettings("OutputMaskFile",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("ErrorBarCriterion", 3.3, mustBePositive,
        "Some selection criteria for the detector tests.");
    setPropertySettings("ErrorBarCriterion",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("DetectorVanadium1", "",
        "The detector vanadium file to run the tests on.");
    setPropertySettings("DetectorVanadium1",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("HighCounts", 1.e+10, mustBePositive,
        "Mask detectors above this threshold.");
    setPropertySettings("HighCounts",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("LowCounts", 1.e-10, mustBePositive,
        "Mask detectors below this threshold.");
    setPropertySettings("LowCounts",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("MedianTestHigh", 3.0, mustBePositive,
        "Mask detectors above this threshold.");
    setPropertySettings("MedianTestHigh",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("MedianTestLow", 0.1, mustBePositive,
        "Mask detectors below this threshold.");
    setPropertySettings("MedianTestLow",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("DetectorVanadium2", "",
        "The detector vanadium to check against for time variations.");
    setPropertySettings("DetectorVanadium2",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("ProptionalChangeCriterion", 1.1, mustBePositive,
        "Mask detectors if the time variation is above this threshold.");
    setPropertySettings("ProptionalChangeCriterion",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("BackgroundCheck", false,
        "If true, run a background check on detector vanadium.");
    setPropertySettings("BackgroundCheck",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("AcceptanceFactor", 5.0, mustBePositive,
        "Mask detectors above this threshold.");
    setPropertySettings("AcceptanceFactor",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    auto mustBeIntPositive = boost::make_shared<BoundedValidator<size_t> >();
    mustBeIntPositive->setLower(0);
    size_t tof_start = 18000;
    declareProperty("BackgroundTofStart", tof_start, mustBeIntPositive,
        "Start TOF for the background check.");
    setPropertySettings("BackgroundTofStart",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    size_t tof_end = 19500;
    declareProperty("BackgroundTofEnd", tof_end, mustBeIntPositive,
        "End TOF for the background check.");
    setPropertySettings("BackgroundTofEnd",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("RejectZeroBackground", true,
        "If true, check the background region for anomolies.");
    setPropertySettings("RejectZeroBackground",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    setPropertySettings("RejectZeroBackground",
        new VisibleWhenProperty("BackgroundCheck", IS_EQUAL_TO, "1"));
    declareProperty("PsdBleed", false, "If true, perform a PSD bleed test.");
    setPropertySettings("PsdBleed",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    declareProperty("MaxFramerate", "", "The maximum framerate to check.");
    setPropertySettings("MaxFramerate",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    setPropertySettings("MaxFramerate",
        new VisibleWhenProperty("PsdBleed", IS_EQUAL_TO, "1"));
    declareProperty("IgnoredPixels", "",
        "A list of pixels to ignore in the calculations.");
    setPropertySettings("IgnoredPixels",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    setPropertySettings("IgnoredPixels",
        new VisibleWhenProperty("PsdBleed", IS_EQUAL_TO, "1"));
    declareProperty("DoAbsoluteUnits", false,
        "If true, perform an absolute units normalisation.");
    declareProperty("AbsUnitsVanadium" "",
        "The vanadium file used as the sample in the absolute units normalisation.");
    setPropertySettings("AbsUnitsVanadium",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("AbsUnitsGroupingFile", "",
        "Grouping file for absolute units normalisation.");
    setPropertySettings("AbsUnitsGroupingFile",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("AbsUnitsDetectorVanadium", "",
        "The detector vanadium file used in the absolute units normalisation.");
    setPropertySettings("AbsUnitsDetectorVanadium",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("AbsUnitsIncidentEnergy", EMPTY_DBL(), mustBePositive,
        "The incident energy for the vanadium sample.");
    setPropertySettings("AbsUnitsIncidentEnergy",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("AbsUnitsMinimumEnergy", -1.0,
        "The minimum energy for the integration range.");
    setPropertySettings("AbsUnitsMinimumEnergy",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("AbsUnitsMaximumEnergy", 1.0,
        "The maximum energy for the integration range.");
    setPropertySettings("AbsUnitsMaximumEnergy",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("VanadiumMass", 32.58, "The mass of vanadium.");
    setPropertySettings("VanadiumMass",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("SampleMass", 1.0, "The mass of sample.");
    setPropertySettings("SampleMass",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("SampleRmm", 1.0, "The rmm of sample.");
    setPropertySettings("SampleRmm",
        new VisibleWhenProperty("DoAbsoluteUnits", IS_EQUAL_TO, "1"));
    declareProperty("ReductionProperties", "__dgs_reduction_properties",
        Direction::Input);
  }

  /**
   * Create a workspace by either loading a file or using an existing
   * workspace.
   */
  Workspace_sptr DgsReduction::loadInputData(boost::shared_ptr<PropertyManager> manager)
  {
    const std::string facility = ConfigService::Instance().getFacility().name();
    if ("SNS" == facility)
      {
        this->setLoadAlg("LoadEventNexus");
      }
    else
      {
        this->setLoadAlg("LoadRaw");
      }
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
        manager->declareProperty(new PropertyWithValue<std::string>("MonitorFilename", inputData));
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
    const double initial_energy = this->getProperty("IncidentEnergy");
    const bool fixed_ei = this->getProperty("FixedIncidentEnergy");
    const std::vector<double> et_binning = this->getProperty("EnergyTransferRange");
    const bool sofphie_is_distribution = this->getProperty("SofPhiEIsDistribution");
    const bool tib_subtraction = this->getProperty("TimeIndepBackgroundSub");
    const double tib_tof_start = this->getProperty("TibTofRangeStart");
    const double tib_tof_end = this->getProperty("TibTofRangeEnd");

    IAlgorithm_sptr et_conv = this->createSubAlgorithm("DgsConvertToEnergyTransfer");
    et_conv->setProperty("InputWorkspace", inputWS);
    et_conv->setProperty("IncidentEnergy", initial_energy);
    et_conv->setProperty("FixedIncidentEnergy", fixed_ei);
    et_conv->setProperty("EnergyTransferRange", et_binning);
    et_conv->setProperty("SofPhiEIsDistribution", sofphie_is_distribution);
    et_conv->setProperty("TimeIndepBackgroundSub", tib_subtraction);
    et_conv->setProperty("TibTofRangeStart", tib_tof_start);
    et_conv->setProperty("TibTofRangeEnd", tib_tof_end);
    et_conv->execute();
  }

} // namespace Mantid
} // namespace WorkflowAlgorithms
