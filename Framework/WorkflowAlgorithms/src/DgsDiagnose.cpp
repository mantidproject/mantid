#include "MantidWorkflowAlgorithms/DgsDiagnose.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidWorkflowAlgorithms/WorkflowAlgorithmHelpers.h"

#include <boost/lexical_cast.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/tokenizer.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace WorkflowAlgorithmHelpers;

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DgsDiagnose)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
DgsDiagnose::DgsDiagnose() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
DgsDiagnose::~DgsDiagnose() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string DgsDiagnose::name() const { return "DgsDiagnose"; }

/// Algorithm's version for identification. @see Algorithm::version
int DgsDiagnose::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DgsDiagnose::category() const {
  return "Workflow\\Inelastic\\UsesPropertyManager";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DgsDiagnose::init() {
  this->declareProperty(
      new WorkspaceProperty<>("DetVanWorkspace", "", Direction::Input),
      "The detector vanadium workspace.");
  this->declareProperty(
      new WorkspaceProperty<>("DetVanMonitorWorkspace", "", Direction::Input,
                              PropertyMode::Optional),
      "A monitor workspace associated with the detector vanadium workspace.");
  this->declareProperty(
      new WorkspaceProperty<>("DetVanCompWorkspace", "", Direction::Input,
                              PropertyMode::Optional),
      "A detector vanadium workspace to compare against the primary one.");
  this->declareProperty(new WorkspaceProperty<>("DetVanCompMonitorWorkspace",
                                                "", Direction::Input,
                                                PropertyMode::Optional),
                        "A monitor workspace associated with the comparison "
                        "detector vanadium workspace.");
  this->declareProperty(new WorkspaceProperty<>("SampleWorkspace", "",
                                                Direction::Input,
                                                PropertyMode::Optional),
                        "A sample workspace to run some diagnostics on.");
  this->declareProperty(
      new WorkspaceProperty<>("SampleMonitorWorkspace", "", Direction::Input,
                              PropertyMode::Optional),
      "A monitor workspace associated with the sample workspace.");
  this->declareProperty(new WorkspaceProperty<>("HardMaskWorkspace", "",
                                                Direction::Input,
                                                PropertyMode::Optional),
                        "A hard mask workspace to apply.");
  this->declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "This is the resulting mask workspace.");
  this->declareProperty("ReductionProperties", "__dgs_reduction_properties",
                        Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DgsDiagnose::exec() {
  g_log.notice() << "Starting DgsDiagnose" << std::endl;
  // Get the reduction property manager
  const std::string reductionManagerName =
      this->getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    throw std::runtime_error(
        "DgsDiagnose cannot run without a reduction PropertyManager.");
  }

  // Gather all the necessary properties
  MatrixWorkspace_sptr detVanWS = this->getProperty("DetVanWorkspace");
  MatrixWorkspace_sptr detVanMonWS =
      this->getProperty("DetVanMonitorWorkspace");
  MatrixWorkspace_sptr detVanCompWS = this->getProperty("DetVanCompWorkspace");
  MatrixWorkspace_sptr detVanCompMonWS =
      this->getProperty("DetVanCompMonitorWorkspace");
  MatrixWorkspace_sptr hardMaskWS = this->getProperty("HardMaskWorkspace");
  MatrixWorkspace_sptr sampleWS;
  MatrixWorkspace_sptr sampleMonWS;

  // Boolean properties
  const bool checkBkg = getBoolPropOrParam("BackgroundCheck", reductionManager,
                                           "check_background", detVanWS);
  const bool rejectZeroBkg = getBoolPropOrParam(
      "RejectZeroBackground", reductionManager, "diag_samp_zero", detVanWS);
  const bool createPsdBleed = getBoolPropOrParam("PsdBleed", reductionManager,
                                                 "diag_bleed_test", detVanWS);
  const bool vanSA =
      getBoolPropOrParam("MedianTestCorrectForSolidAngle", reductionManager,
                         "diag_correct_solid_angle", detVanWS);

  // Numeric properties
  const double huge =
      getDblPropOrParam("HighCounts", reductionManager, "diag_huge", detVanWS);
  const double tiny =
      getDblPropOrParam("LowCounts", reductionManager, "diag_tiny", detVanWS);
  const double vanOutHi = getDblPropOrParam("HighOutlier", reductionManager,
                                            "diag_van_out_hi", detVanWS);
  const double vanOutLo = getDblPropOrParam("LowOutlier", reductionManager,
                                            "diag_van_out_lo", detVanWS);
  const double vanHi = getDblPropOrParam("MedianTestHigh", reductionManager,
                                         "diag_van_hi", detVanWS);
  const double vanLo = getDblPropOrParam("MedianTestLow", reductionManager,
                                         "diag_van_lo", detVanWS);
  const double vanLevelsUp = getDblPropOrParam(
      "MedianTestLevelsUp", reductionManager, "diag_van_levels", detVanWS, 0);
  const double vanSigma = getDblPropOrParam(
      "ErrorBarCriterion", reductionManager, "diag_van_sig", detVanWS);
  const double variation = getDblPropOrParam(
      "DetVanRatioVariation", reductionManager, "diag_variation", detVanWS);
  const double samHi = getDblPropOrParam(
      "SamBkgMedianTestHigh", reductionManager, "diag_samp_hi", detVanWS);
  const double samLo = getDblPropOrParam(
      "SamBkgMedianTestLow", reductionManager, "diag_samp_lo", detVanWS);
  const double samSigma = getDblPropOrParam(
      "SamBkgErrorBarCriterion", reductionManager, "diag_samp_sig", detVanWS);
  double bleedRate = getDblPropOrParam("MaxFramerate", reductionManager,
                                       "diag_bleed_maxrate", detVanWS);
  const double bleedPixels = getDblPropOrParam(
      "IgnoredPixels", reductionManager, "diag_bleed_pixels", detVanWS, 80.0);

  // Make some internal names for workspaces
  const std::string dvInternal = "__det_van";
  const std::string dvCompInternal = "__det_van_comp";
  const std::string sampleInternal = "__sample";
  const std::string bkgInternal = "__background_int";
  const std::string countsInternal = "__total_counts";

  // If we are running this standalone, the IncidentEnergyGuess property in
  // the reduction property manager does not exist. If that is true, then we
  // don't have to clone workspaces.
  bool isStandAlone = !reductionManager->existsProperty("IncidentEnergyGuess");

  // Process the detector vanadium
  IAlgorithm_sptr detVan =
      this->createChildAlgorithm("DgsProcessDetectorVanadium");
  detVan->setProperty("InputWorkspace", detVanWS);
  detVan->setProperty("OutputWorkspace", dvInternal);
  detVan->setProperty("InputMonitorWorkspace", detVanMonWS);
  detVan->setProperty("ReductionProperties", reductionManagerName);
  detVan->executeAsChildAlg();
  MatrixWorkspace_sptr dvWS = detVan->getProperty("OutputWorkspace");

  // Process the comparison detector vanadium workspace if present
  MatrixWorkspace_sptr dvCompWS;
  if (detVanCompWS) {
    detVan->setProperty("InputWorkspace", detVanCompWS);
    detVan->setProperty("OutputWorkspace", dvCompInternal);
    detVan->setProperty("InputMonitorWorkspace", detVanCompMonWS);
    detVan->executeAsChildAlg();
    dvCompWS = detVan->getProperty("OutputWorkspace");
    detVanCompWS.reset();
  } else {
    dvCompWS = boost::shared_ptr<MatrixWorkspace>();
  }

  // Process the sample data if any of the sample checks are requested.
  if (checkBkg || rejectZeroBkg || createPsdBleed) {
    sampleWS = this->getProperty("SampleWorkspace");
    sampleMonWS = this->getProperty("SampleMonitorWorkspace");

    Workspace_sptr tmp;
    if (!isStandAlone) {
      IAlgorithm_sptr cloneWs = this->createChildAlgorithm("CloneWorkspace");
      cloneWs->setProperty("InputWorkspace", sampleWS);
      cloneWs->setProperty("OutputWorkspace", sampleInternal);
      cloneWs->executeAsChildAlg();
      tmp = cloneWs->getProperty("OutputWorkspace");
      sampleWS = boost::static_pointer_cast<MatrixWorkspace>(tmp);
    }

    IAlgorithm_sptr norm = this->createChildAlgorithm("DgsPreprocessData");
    norm->setProperty("InputWorkspace", sampleWS);
    norm->setProperty("OutputWorkspace", sampleWS);
    norm->setProperty("InputMonitorWorkspace", sampleMonWS);
    norm->setProperty("ReductionProperties", reductionManagerName);
    norm->executeAsChildAlg();
    sampleWS = norm->getProperty("OutputWorkspace");
  }

  // Create the total counts workspace if necessary
  MatrixWorkspace_sptr totalCountsWS;
  if (rejectZeroBkg) {
    IAlgorithm_sptr integrate = this->createChildAlgorithm("Integration");
    integrate->setProperty("InputWorkspace", sampleWS);
    integrate->setProperty("OutputWorkspace", countsInternal);
    integrate->setProperty("IncludePartialBins", true);
    integrate->executeAsChildAlg();
    totalCountsWS = integrate->getProperty("OutputWorkspace");
  } else {
    totalCountsWS = boost::shared_ptr<MatrixWorkspace>();
  }

  // Create the background workspace if necessary
  MatrixWorkspace_sptr backgroundIntWS;
  if (checkBkg) {
    double rangeStart = getDblPropOrParam(
        "BackgroundTofStart", reductionManager, "bkgd-range-min", sampleWS);

    double rangeEnd = getDblPropOrParam("BackgroundTofEnd", reductionManager,
                                        "bkgd-range-max", sampleWS);

    IAlgorithm_sptr integrate = this->createChildAlgorithm("Integration");
    integrate->setProperty("InputWorkspace", sampleWS);
    integrate->setProperty("OutputWorkspace", bkgInternal);
    integrate->setProperty("RangeLower", rangeStart);
    integrate->setProperty("RangeUpper", rangeEnd);
    integrate->setProperty("IncludePartialBins", true);
    integrate->executeAsChildAlg();
    backgroundIntWS = integrate->getProperty("OutputWorkspace");

    // Need to match the units between background and detector vanadium
    const std::string detVanIntRangeUnits =
        reductionManager->getProperty("DetVanIntRangeUnits");
    IAlgorithm_sptr cvu = this->createChildAlgorithm("ConvertUnits");
    cvu->setProperty("InputWorkspace", backgroundIntWS);
    cvu->setProperty("OutputWorkspace", backgroundIntWS);
    cvu->setProperty("Target", detVanIntRangeUnits);
    cvu->executeAsChildAlg();
    backgroundIntWS = cvu->getProperty("OutputWorkspace");

    // Normalise the background integral workspace
    if (dvCompWS) {
      MatrixWorkspace_sptr hmean = 2.0 * dvWS * dvCompWS;
      hmean /= (dvWS + dvCompWS);
      backgroundIntWS /= hmean;
    } else {
      backgroundIntWS /= dvWS;
    }
  } else {
    backgroundIntWS = boost::shared_ptr<MatrixWorkspace>();
  }

  // Handle case where one of the other tests (checkBkg or rejectZeroBkg)
  // are requested, but not createPsdBleed.
  if (!createPsdBleed) {
    sampleWS = boost::shared_ptr<MatrixWorkspace>();
  }

  IAlgorithm_sptr diag = this->createChildAlgorithm("DetectorDiagnostic");
  diag->setProperty("InputWorkspace", dvWS);
  diag->setProperty("DetVanCompare", dvCompWS);
  diag->setProperty("SampleWorkspace", sampleWS);
  diag->setProperty("SampleTotalCountsWorkspace", totalCountsWS);
  diag->setProperty("SampleBackgroundWorkspace", backgroundIntWS);
  diag->setProperty("HardMaskWorkspace", hardMaskWS);
  diag->setProperty("LowThreshold", tiny);
  diag->setProperty("HighThreshold", huge);
  diag->setProperty("LowOutlier", vanOutLo);
  diag->setProperty("HighOutlier", vanOutHi);
  diag->setProperty("LowThresholdFraction", vanLo);
  diag->setProperty("HighThresholdFraction", vanHi);
  diag->setProperty("LevelsUp", static_cast<int>(vanLevelsUp));
  diag->setProperty("CorrectForSolidAngle", vanSA);
  diag->setProperty("SignificanceTest", vanSigma);
  diag->setProperty("DetVanRatioVariation", variation);
  diag->setProperty("SampleBkgLowAcceptanceFactor", samLo);
  diag->setProperty("SampleBkgHighAcceptanceFactor", samHi);
  diag->setProperty("SampleBkgSignificanceTest", samSigma);
  diag->setProperty("MaxTubeFramerate", bleedRate);
  diag->setProperty("NIgnoredCentralPixels", static_cast<int>(bleedPixels));

  MatrixWorkspace_sptr maskWS;
  std::vector<std::string> diag_spectra =
      dvWS->getInstrument()->getStringParameter("diag_spectra");
  if (diag_spectra.empty() || "None" == diag_spectra[0]) {
    diag->execute();
    maskWS = diag->getProperty("OutputWorkspace");
  } else {
    typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
    boost::char_separator<char> sep("(,);");
    tokenizer tokens(diag_spectra[0], sep);
    for (tokenizer::iterator tok_iter = tokens.begin();
         tok_iter != tokens.end();) {
      int startIndex = boost::lexical_cast<int>(*tok_iter);
      startIndex -= 1;
      ++tok_iter;
      int endIndex = boost::lexical_cast<int>(*tok_iter);
      endIndex -= 1;
      g_log.information() << "Pixel range: (" << startIndex << ", " << endIndex
                          << ")" << std::endl;
      diag->setProperty("StartWorkspaceIndex", startIndex);
      diag->setProperty("EndWorkspaceIndex", endIndex);
      diag->execute();
      if (maskWS) {
        MatrixWorkspace_sptr tmp = diag->getProperty("OutputWorkspace");
        IAlgorithm_sptr comb = createChildAlgorithm("BinaryOperateMasks");
        comb->setProperty("InputWorkspace1", maskWS);
        comb->setProperty("InputWorkspace2", tmp);
        comb->setProperty("OutputWorkspace", maskWS);
        comb->setProperty("OperationType", "OR");
        comb->execute();
      } else {
        maskWS = diag->getProperty("OutputWorkspace");
      }
      ++tok_iter;
    }
  }

  // Cleanup
  dvWS.reset();
  dvCompWS.reset();
  sampleWS.reset();
  totalCountsWS.reset();
  backgroundIntWS.reset();

  // If mask file name is set save out the diagnostic mask.
  if (reductionManager->existsProperty("OutputMaskFile")) {
    std::string maskFilename =
        reductionManager->getPropertyValue("OutputMaskFile");
    if (!maskFilename.empty()) {
      IAlgorithm_sptr saveNxs = this->createChildAlgorithm("SaveMask");
      saveNxs->setProperty("InputWorkspace", maskWS);
      saveNxs->setProperty("OutputFile", maskFilename);
      saveNxs->execute();
    }
  }

  MaskWorkspace_sptr m = boost::dynamic_pointer_cast<MaskWorkspace>(maskWS);
  g_log.information() << "Number of masked pixels = " << m->getNumberMasked()
                      << std::endl;

  this->setProperty("OutputWorkspace", maskWS);
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
