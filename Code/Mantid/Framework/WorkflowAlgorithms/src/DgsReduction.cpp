/*WIKI*

This is the top-level workflow algorithm for direct geometry spectrometer
data reduction. This algorithm is responsible for gathering the necessary
parameters and generating calls to other workflow or standard algorithms.

*WIKI*/

#include "MantidWorkflowAlgorithms/DgsReduction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include "MantidKernel/RebinParamsValidator.h"
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
    //declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    //declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
    declareProperty("SampleData", "", "Run numbers, files or workspaces of the data sets to be reduced");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("IncidentEnergy", EMPTY_DBL(), mustBePositive,
      "Set the value of the incident energy in meV.");
    declareProperty("FixedIncidentEnergy", false,
        "Declare the value of the incident energy to be fixed (will not be calculated).");
    declareProperty(new ArrayProperty<double>("EnergyTransferRange",
        boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary.\n"
      "Negative width value indicates logarithmic binning.");
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
    declareProperty("AcceptanceFactor", 5, mustBePositive,
        "Mask detectors above this threshold.");
    setPropertySettings("AcceptanceFactor",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    setPropertySettings("AcceptanceFactor",
        new VisibleWhenProperty("BackgroundCheck", IS_EQUAL_TO, "1"));
    auto mustBeIntPositive = boost::make_shared<BoundedValidator<size_t> >();
    mustBeIntPositive->setLower(0);
    declareProperty("BackgroundTofStart", 18000, mustBeIntPositive,
        "Start TOF for the background check.");
    setPropertySettings("BackgroundTofStart",
        new VisibleWhenProperty("FindBadDetectors", IS_EQUAL_TO, "1"));
    setPropertySettings("BackgroundTofStart",
        new VisibleWhenProperty("BackgroundCheck", IS_EQUAL_TO, "1"));
    declareProperty("BackgroundTofEnd", 19500, mustBeIntPositive,
        "End TOF for the background check.");
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
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void DgsReduction::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Mantid
} // namespace WorkflowAlgorithms
