//--------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------
#include "MantidAlgorithms/DetectorDiagnostic.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidDataObjects/EventWorkspaceHelpers.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <gsl/gsl_statistics.h>
#include <cfloat>
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {

namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(DetectorDiagnostic)

using API::MatrixWorkspace_sptr;
using API::IAlgorithm_sptr;
using Geometry::IDetector_const_sptr;
using std::string;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

//--------------------------------------------------------------------------
// Functions to make this a proper workflow algorithm
//--------------------------------------------------------------------------
const std::string DetectorDiagnostic::category() const {
  return "Diagnostics;Workflow\\Diagnostics";
}

const std::string DetectorDiagnostic::name() const {
  return "DetectorDiagnostic";
}

int DetectorDiagnostic::version() const { return 1; }

void DetectorDiagnostic::init() {
  this->declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "Name of the integrated detector vanadium (white beam) workspace.");
  this->declareProperty(new WorkspaceProperty<>("HardMaskWorkspace", "",
                                                Direction::Input,
                                                PropertyMode::Optional),
                        "A hard mask to apply to the inputworkspace");
  this->declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "A MaskWorkspace containing the masked spectra as zeroes and ones.");
  auto mustBePosInt = boost::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  this->declareProperty(
      "StartWorkspaceIndex", 0, mustBePosInt,
      "The index number of the first spectrum to include in the calculation\n"
      "(default 0)");
  this->declareProperty(
      "EndWorkspaceIndex", EMPTY_INT(), mustBePosInt,
      "The index number of the last spectrum to include in the calculation\n"
      "(default the last histogram)");
  this->declareProperty(
      "RangeLower", EMPTY_DBL(),
      "No bin with a boundary at an x value less than this will be used\n"
      "in the summation that decides if a detector is 'bad' (default: the\n"
      "start of each histogram)");
  this->declareProperty(
      "RangeUpper", EMPTY_DBL(),
      "No bin with a boundary at an x value higher than this value will\n"
      "be used in the summation that decides if a detector is 'bad'\n"
      "(default: the end of each histogram)");

  string findDetOutLimGrp("Find Detectors Outside Limits");
  this->declareProperty(
      "LowThreshold", 0.0,
      "Spectra whose total number of counts are equal to or below this value\n"
      "will be marked bad (default 0)");
  this->setPropertyGroup("LowThreshold", findDetOutLimGrp);
  this->declareProperty(
      "HighThreshold", EMPTY_DBL(),
      "Spectra whose total number of counts are equal to or above this value\n"
      "will be marked bad (default off)");
  this->setPropertyGroup("HighThreshold", findDetOutLimGrp);

  string medianDetTestGrp("Median Detector Test");
  auto mustBePositiveDbl = boost::make_shared<BoundedValidator<double>>();
  mustBePositiveDbl->setLower(0);
  this->declareProperty(
      "LevelsUp", 0, mustBePosInt,
      "Levels above pixel that will be used to compute the median.\n"
      "If no level is specified, or 0, the median is over the whole "
      "instrument.");
  this->setPropertyGroup("LevelsUp", medianDetTestGrp);
  this->declareProperty("SignificanceTest", 0.0, mustBePositiveDbl,
                        "Error criterion as a multiple of error bar i.e. to "
                        "fail the test, the magnitude of the\n"
                        "difference with respect to the median value must also "
                        "exceed this number of error bars");
  this->setPropertyGroup("SignificanceTest", medianDetTestGrp);
  this->declareProperty("LowThresholdFraction", 0.1,
                        "Lower acceptable bound as fraction of median value");
  this->setPropertyGroup("LowThresholdFraction", medianDetTestGrp);
  this->declareProperty("HighThresholdFraction", 1.5,
                        "Upper acceptable bound as fraction of median value");
  this->setPropertyGroup("HighThresholdFraction", medianDetTestGrp);
  this->declareProperty(
      "LowOutlier", 0.01,
      "Lower bound defining outliers as fraction of median value");
  this->setPropertyGroup("LowOutlier", medianDetTestGrp);
  this->declareProperty(
      "HighOutlier", 100.,
      "Upper bound defining outliers as fraction of median value");
  this->setPropertyGroup("HighOutlier", medianDetTestGrp);
  this->declareProperty(
      "CorrectForSolidAngle", false,
      "Flag to correct for solid angle efficiency. False by default.");
  this->setPropertyGroup("CorrectForSolidAngle", medianDetTestGrp);
  this->declareProperty("ExcludeZeroesFromMedian", false,
                        "If false (default) zeroes will be included in "
                        "the median calculation, otherwise they will not be "
                        "included but they will be left unmasked");
  this->setPropertyGroup("ExcludeZeroesFromMedian", medianDetTestGrp);

  string detEffVarGrp("Detector Efficiency Variation");
  this->declareProperty(
      new WorkspaceProperty<>("DetVanCompare", "", Direction::Input,
                              PropertyMode::Optional),
      "Name of a matching second detector vanadium run from the same\n"
      "instrument. It must be treated in the same manner as the input detector "
      "vanadium.");
  this->setPropertyGroup("DetVanCompare", detEffVarGrp);
  this->declareProperty(
      "DetVanRatioVariation", 1.1, mustBePositiveDbl,
      "Identify spectra whose total number of counts has changed by more\n"
      "than this factor of the median change between the two input workspaces");
  this->setPropertyGroup("DetVanRatioVariation", detEffVarGrp);
  this->setPropertySettings(
      "DetVanRatioVariation",
      new EnabledWhenProperty("DetVanCompare", IS_NOT_DEFAULT));

  string countsCheck("Check Sample Counts");
  this->declareProperty(
      new WorkspaceProperty<>("SampleTotalCountsWorkspace", "",
                              Direction::Input, PropertyMode::Optional),
      "A sample workspace integrated over the full axis range.");
  this->setPropertyGroup("SampleTotalCountsWorkspace", countsCheck);

  string backgroundCheck("Check Sample Background");
  this->declareProperty(
      new WorkspaceProperty<>("SampleBackgroundWorkspace", "", Direction::Input,
                              PropertyMode::Optional),
      "A sample workspace integrated over the background region.");
  this->setPropertyGroup("SampleBackgroundWorkspace", backgroundCheck);
  this->declareProperty(
      "SampleBkgLowAcceptanceFactor", 0.0, mustBePositiveDbl,
      "Low threshold for the background check MedianDetectorTest.");
  this->setPropertyGroup("SampleBkgLowAcceptanceFactor", backgroundCheck);
  this->declareProperty(
      "SampleBkgHighAcceptanceFactor", 5.0, mustBePositiveDbl,
      "High threshold for the background check MedianDetectorTest.");
  this->setPropertyGroup("SampleBkgHighAcceptanceFactor", backgroundCheck);
  this->declareProperty("SampleBkgSignificanceTest", 3.3, mustBePositiveDbl,
                        "Error criterion as a multiple of error bar i.e. to "
                        "fail the test, the magnitude of the\n"
                        "difference with respect to the median value must also "
                        "exceed this number of error bars");
  this->setPropertyGroup("SampleBkgSignificanceTest", backgroundCheck);
  this->declareProperty("SampleCorrectForSolidAngle", false,
                        "Flag to correct for solid angle efficiency for "
                        "background check MedianDetectorTest. False by "
                        "default.");
  this->setPropertyGroup("SampleCorrectForSolidAngle", backgroundCheck);

  string psdBleedMaskGrp("Create PSD Bleed Mask");
  this->declareProperty(
      new WorkspaceProperty<>("SampleWorkspace", "", Direction::Input,
                              PropertyMode::Optional),
      "A sample workspace. This is used in the PSD Bleed calculation.");
  this->setPropertyGroup("SampleWorkspace", psdBleedMaskGrp);
  this->declareProperty(
      "MaxTubeFramerate", 0.0, mustBePositiveDbl,
      "The maximum rate allowed for a tube in counts/us/frame.");
  this->setPropertyGroup("MaxTubeFramerate", psdBleedMaskGrp);
  this->declareProperty("NIgnoredCentralPixels", 80, mustBePosInt,
                        "The number of pixels about the centre to ignore.");
  this->setPropertyGroup("NIgnoredCentralPixels", psdBleedMaskGrp);
  this->setPropertySettings(
      "NIgnoredCentralPixels",
      new EnabledWhenProperty("MaxTubeFramerate", IS_NOT_DEFAULT));

  this->declareProperty("NumberOfFailures", 0, Direction::Output);
}

void DetectorDiagnostic::exec() {
  // get the generic information that everybody uses
  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  m_minIndex = this->getProperty("StartWorkspaceIndex");
  m_maxIndex = this->getProperty("EndWorkspaceIndex");
  m_rangeLower = this->getProperty("RangeLower");
  m_rangeUpper = this->getProperty("RangeUpper");

  // integrate the data once to pass to ChildAlgorithms
  m_fracDone = 0.;

  // Get the other workspaces
  MatrixWorkspace_sptr input2WS = this->getProperty("DetVanCompare");
  MatrixWorkspace_sptr totalCountsWS =
      this->getProperty("SampleTotalCountsWorkspace");
  MatrixWorkspace_sptr bkgWS = this->getProperty("SampleBackgroundWorkspace");
  MatrixWorkspace_sptr sampleWS = this->getProperty("SampleWorkspace");

  // calculate the number of tests for progress bar
  m_progStepWidth = 0;
  {
    int numTests(1); // if detector vanadium present, do it!
    if (input2WS)
      numTests += 1;
    if (totalCountsWS)
      numTests += 1;
    if (bkgWS)
      numTests += 1;
    if (sampleWS)
      numTests += 1;
    g_log.information() << "Number of tests requested: " << numTests
                        << std::endl;
    m_progStepWidth = (1. - m_fracDone) / static_cast<double>(numTests);
  }

  int numFailed(0);
  MatrixWorkspace_sptr maskWS;

  // Apply hard mask if present
  MatrixWorkspace_sptr hardMaskWS = this->getProperty("HardMaskWorkspace");
  if (hardMaskWS) {
    IAlgorithm_sptr md = this->createChildAlgorithm("MaskDetectors");
    md->setProperty("Workspace", inputWS);
    md->setProperty("MaskedWorkspace", hardMaskWS);
    md->executeAsChildAlg();
  }

  // Perform FindDetectorsOutsideLimits and MedianDetectorTest on the
  // detector vanadium
  maskWS = this->doDetVanTest(inputWS, numFailed);

  // DetectorEfficiencyVariation (only if two workspaces are specified)
  if (input2WS) {
    // apply mask to what we are going to input
    this->applyMask(input2WS, maskWS);

    maskWS = this->doDetVanTest(input2WS, numFailed);

    // get the relevant inputs
    double variation = this->getProperty("DetVanRatioVariation");

    // run the ChildAlgorithm
    IAlgorithm_sptr alg =
        this->createChildAlgorithm("DetectorEfficiencyVariation", m_fracDone,
                                   m_fracDone + m_progStepWidth);
    m_fracDone += m_progStepWidth;
    alg->setProperty("WhiteBeamBase", inputWS);
    alg->setProperty("WhiteBeamCompare", input2WS);
    alg->setProperty("StartWorkspaceIndex", m_minIndex);
    alg->setProperty("EndWorkspaceIndex", m_maxIndex);
    alg->setProperty("RangeLower", m_rangeLower);
    alg->setProperty("RangeUpper", m_rangeUpper);
    alg->setProperty("Variation", variation);
    alg->executeAsChildAlg();
    MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
    applyMask(inputWS, localMaskWS);
    applyMask(input2WS, localMaskWS);
    int localFails = alg->getProperty("NumberOfFailures");
    numFailed += localFails;
  }

  // Zero total counts check for sample counts
  if (totalCountsWS) {
    // apply mask to what we are going to input
    applyMask(totalCountsWS, maskWS);

    IAlgorithm_sptr zeroChk = this->createChildAlgorithm(
        "FindDetectorsOutsideLimits", m_fracDone, m_fracDone + m_progStepWidth);
    m_fracDone += m_progStepWidth;
    zeroChk->setProperty("InputWorkspace", totalCountsWS);
    zeroChk->setProperty("StartWorkspaceIndex", m_minIndex);
    zeroChk->setProperty("EndWorkspaceIndex", m_maxIndex);
    zeroChk->setProperty("LowThreshold", 1.0e-10);
    zeroChk->setProperty("HighThreshold", 1.0e100);
    zeroChk->executeAsChildAlg();
    MatrixWorkspace_sptr localMaskWS = zeroChk->getProperty("OutputWorkspace");
    applyMask(inputWS, localMaskWS);
    int localFails = zeroChk->getProperty("NumberOfFailures");
    numFailed += localFails;
  }

  // Background check
  if (bkgWS) {
    // apply mask to what we are going to input
    this->applyMask(bkgWS, maskWS);

    double significanceTest = this->getProperty("SampleBkgSignificanceTest");
    double lowThreshold = this->getProperty("SampleBkgLowAcceptanceFactor");
    double highThreshold = this->getProperty("SampleBkgHighAcceptanceFactor");
    bool correctSA = this->getProperty("SampleCorrectForSolidAngle");

    // run the ChildAlgorithm
    IAlgorithm_sptr alg = this->createChildAlgorithm(
        "MedianDetectorTest", m_fracDone, m_fracDone + m_progStepWidth);
    m_fracDone += m_progStepWidth;
    alg->setProperty("InputWorkspace", bkgWS);
    alg->setProperty("StartWorkspaceIndex", m_minIndex);
    alg->setProperty("EndWorkspaceIndex", m_maxIndex);
    alg->setProperty("SignificanceTest", significanceTest);
    alg->setProperty("LowThreshold", lowThreshold);
    alg->setProperty("HighThreshold", highThreshold);
    alg->setProperty("LowOutlier", 0.0);
    alg->setProperty("HighOutlier", 1.0e100);
    alg->setProperty("ExcludeZeroesFromMedian", true);
    alg->setProperty("CorrectForSolidAngle", correctSA);
    alg->executeAsChildAlg();
    MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
    applyMask(inputWS, localMaskWS);
    int localFails = alg->getProperty("NumberOfFailures");
    numFailed += localFails;
  }

  // CreatePSDBleedMask (if selected)
  if (sampleWS) {
    // get the relevant inputs
    double maxTubeFrameRate = this->getProperty("MaxTubeFramerate");
    int numIgnore = this->getProperty("NIgnoredCentralPixels");

    // run the ChildAlgorithm
    IAlgorithm_sptr alg = this->createChildAlgorithm(
        "CreatePSDBleedMask", m_fracDone, m_fracDone + m_progStepWidth);
    m_fracDone += m_progStepWidth;
    alg->setProperty("InputWorkspace", sampleWS);
    alg->setProperty("MaxTubeFramerate", maxTubeFrameRate);
    alg->setProperty("NIgnoredCentralPixels", numIgnore);
    alg->executeAsChildAlg();
    MatrixWorkspace_sptr localMaskWS = alg->getProperty("OutputWorkspace");
    applyMask(inputWS, localMaskWS);
    int localFails = alg->getProperty("NumberOfFailures");
    numFailed += localFails;
  }

  g_log.information() << numFailed << " spectra are being masked\n";
  setProperty("NumberOfFailures", numFailed);

  // Extract the mask from the vanadium workspace
  std::vector<int> detList;
  IAlgorithm_sptr extract = this->createChildAlgorithm("ExtractMask");
  extract->setProperty("InputWorkspace", inputWS);
  extract->setProperty("OutputWorkspace", "final_mask");
  extract->setProperty("DetectorList", detList);
  extract->executeAsChildAlg();
  maskWS = extract->getProperty("OutputWorkspace");

  this->setProperty("OutputWorkspace", maskWS);
}

/**
 * Function to apply a given mask to a workspace.
 * @param inputWS : the workspace to mask
 * @param maskWS : the workspace containing the masking information
 */
void DetectorDiagnostic::applyMask(API::MatrixWorkspace_sptr inputWS,
                                   API::MatrixWorkspace_sptr maskWS) {
  IAlgorithm_sptr maskAlg =
      createChildAlgorithm("MaskDetectors"); // should set progress bar
  maskAlg->setProperty("Workspace", inputWS);
  maskAlg->setProperty("MaskedWorkspace", maskWS);
  maskAlg->setProperty("StartWorkspaceIndex", m_minIndex);
  maskAlg->setProperty("EndWorkspaceIndex", m_maxIndex);
  maskAlg->executeAsChildAlg();
}

/**
 * Function that encapulates the standard detector vanadium tests.
 * @param inputWS : the detector vanadium workspace to test
 * @param nFails : placeholder for the number of failures
 * @return : the resulting mask from the checks
 */
API::MatrixWorkspace_sptr
DetectorDiagnostic::doDetVanTest(API::MatrixWorkspace_sptr inputWS,
                                 int &nFails) {
  MatrixWorkspace_sptr localMask;

  // FindDetectorsOutsideLimits
  // get the relevant inputs
  double lowThreshold = this->getProperty("LowThreshold");
  double highThreshold = this->getProperty("HighThreshold");
  // run the ChildAlgorithm
  IAlgorithm_sptr fdol = this->createChildAlgorithm(
      "FindDetectorsOutsideLimits", m_fracDone, m_fracDone + m_progStepWidth);
  m_fracDone += m_progStepWidth;
  fdol->setProperty("InputWorkspace", inputWS);
  fdol->setProperty("OutputWorkspace", localMask);
  fdol->setProperty("StartWorkspaceIndex", m_minIndex);
  fdol->setProperty("EndWorkspaceIndex", m_maxIndex);
  fdol->setProperty("RangeLower", m_rangeLower);
  fdol->setProperty("RangeUpper", m_rangeUpper);
  fdol->setProperty("LowThreshold", lowThreshold);
  fdol->setProperty("HighThreshold", highThreshold);
  fdol->executeAsChildAlg();
  localMask = fdol->getProperty("OutputWorkspace");
  int localFails = fdol->getProperty("NumberOfFailures");
  nFails += localFails;

  // get the relevant inputs for the MedianDetectorTests
  int parents = this->getProperty("LevelsUp");
  double significanceTest = this->getProperty("SignificanceTest");
  double lowThresholdFrac = this->getProperty("LowThresholdFraction");
  double highThresholdFrac = this->getProperty("HighThresholdFraction");
  double lowOutlier = this->getProperty("LowOutlier");
  double highOutlier = this->getProperty("HighOutlier");
  bool excludeZeroes = this->getProperty("ExcludeZeroesFromMedian");
  bool correctforSA = this->getProperty("CorrectForSolidAngle");

  // MedianDetectorTest
  // apply mask to what we are going to input
  this->applyMask(inputWS, localMask);

  // run the ChildAlgorithm
  IAlgorithm_sptr mdt = this->createChildAlgorithm(
      "MedianDetectorTest", m_fracDone, m_fracDone + m_progStepWidth);
  m_fracDone += m_progStepWidth;
  mdt->setProperty("InputWorkspace", inputWS);
  mdt->setProperty("StartWorkspaceIndex", m_minIndex);
  mdt->setProperty("EndWorkspaceIndex", m_maxIndex);
  mdt->setProperty("RangeLower", m_rangeLower);
  mdt->setProperty("RangeUpper", m_rangeUpper);
  mdt->setProperty("LevelsUp", parents);
  mdt->setProperty("SignificanceTest", significanceTest);
  mdt->setProperty("LowThreshold", lowThresholdFrac);
  mdt->setProperty("HighThreshold", highThresholdFrac);
  mdt->setProperty("LowOutlier", lowOutlier);
  mdt->setProperty("HighOutlier", highOutlier);
  mdt->setProperty("ExcludeZeroesFromMedian", excludeZeroes);
  mdt->setProperty("CorrectForSolidAngle", correctforSA);
  mdt->executeAsChildAlg();
  localMask = mdt->getProperty("OutputWorkspace");
  localFails = mdt->getProperty("NumberOfFailures");
  nFails += localFails;

  this->applyMask(inputWS, localMask);
  return localMask;
}

//--------------------------------------------------------------------------
// Public member functions
//--------------------------------------------------------------------------
DetectorDiagnostic::DetectorDiagnostic()
    : API::Algorithm(), m_fracDone(0.0), m_TotalTime(RTTotal), m_parents(0),
      m_progStepWidth(0.0), m_minIndex(0), m_maxIndex(EMPTY_INT()),
      m_rangeLower(EMPTY_DBL()), m_rangeUpper(EMPTY_DBL()) {}

//--------------------------------------------------------------------------
// Protected member functions
//--------------------------------------------------------------------------

/**
 * Integrate each spectra to get the number of counts
 * @param inputWS :: The workspace to integrate
 * @param indexMin :: The lower bound of the spectra to integrate
 * @param indexMax :: The upper bound of the spectra to integrate
 * @param lower :: The lower bound
 * @param upper :: The upper bound
 * @param outputWorkspace2D :: set to true to output a workspace 2D even if the
 * input is an EventWorkspace
 * @returns A workspace containing the integrated counts
 */
MatrixWorkspace_sptr DetectorDiagnostic::integrateSpectra(
    MatrixWorkspace_sptr inputWS, const int indexMin, const int indexMax,
    const double lower, const double upper, const bool outputWorkspace2D) {
  g_log.debug() << "Integrating input spectra.\n";
  // If the input spectra only has one bin, assume it has been integrated
  // already
  // but we need to pass it to the algorithm so that a copy of the input
  // workspace is
  // actually created to use for further calculations
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetTotalCounts);
  IAlgorithm_sptr childAlg = createChildAlgorithm("Integration", t0, t1);
  childAlg->setProperty("InputWorkspace", inputWS);
  childAlg->setProperty("StartWorkspaceIndex", indexMin);
  childAlg->setProperty("EndWorkspaceIndex", indexMax);
  // pass inputed values straight to this integration trusting the checking done
  // there
  childAlg->setProperty("RangeLower", lower);
  childAlg->setProperty("RangeUpper", upper);
  childAlg->setPropertyValue("IncludePartialBins", "1");
  childAlg->executeAsChildAlg();

  // Convert to 2D if desired, and if the input was an EventWorkspace.
  MatrixWorkspace_sptr outputW = childAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr finalOutputW = outputW;
  if (outputWorkspace2D &&
      boost::dynamic_pointer_cast<EventWorkspace>(outputW)) {
    g_log.debug() << "Converting output Event Workspace into a Workspace2D."
                  << std::endl;
    childAlg = createChildAlgorithm("ConvertToMatrixWorkspace", t0, t1);
    childAlg->setProperty("InputWorkspace", outputW);
    childAlg->executeAsChildAlg();
    finalOutputW = childAlg->getProperty("OutputWorkspace");
  }

  return finalOutputW;
}

/**
 * Create a masking workspace to return.
 *
 * @param inputWS The workspace to initialize from. The instrument is copied
 *from this.
 */
DataObjects::MaskWorkspace_sptr
DetectorDiagnostic::generateEmptyMask(API::MatrixWorkspace_const_sptr inputWS) {
  // Create a new workspace for the results, copy from the input to ensure that
  // we copy over the instrument and current masking
  DataObjects::MaskWorkspace_sptr maskWS(new DataObjects::MaskWorkspace());
  maskWS->initialize(inputWS->getNumberHistograms(), 1, 1);
  WorkspaceFactory::Instance().initializeFromParent(inputWS, maskWS, false);
  maskWS->setTitle(inputWS->getTitle());

  return maskWS;
}

std::vector<std::vector<size_t>>
DetectorDiagnostic::makeInstrumentMap(API::MatrixWorkspace_sptr countsWS) {
  std::vector<std::vector<size_t>> mymap;
  std::vector<size_t> single;

  for (size_t i = 0; i < countsWS->getNumberHistograms(); i++) {
    single.push_back(i);
  }
  mymap.push_back(single);
  return mymap;
}
/** This function will check how to group spectra when calculating median
 *
 *
 */
std::vector<std::vector<size_t>>
DetectorDiagnostic::makeMap(API::MatrixWorkspace_sptr countsWS) {
  std::multimap<Mantid::Geometry::ComponentID, size_t> mymap;

  Geometry::Instrument_const_sptr instrument = countsWS->getInstrument();
  if (m_parents == 0) {
    return makeInstrumentMap(countsWS);
  }
  if (!instrument) {
    g_log.warning("Workspace has no instrument. LevelsUP is ignored");
    return makeInstrumentMap(countsWS);
  }

  // check if not grouped. If grouped, it will throw
  if (countsWS->hasGroupedDetectors()) {
    throw std::runtime_error("Median detector test: not able to create "
                             "detector to spectra map. Try with LevelUp=0.");
  }

  for (size_t i = 0; i < countsWS->getNumberHistograms(); i++) {
    detid_t d = (*((countsWS->getSpectrum(i))->getDetectorIDs().begin()));
    std::vector<boost::shared_ptr<const Mantid::Geometry::IComponent>> anc =
        instrument->getDetector(d)->getAncestors();
    // std::vector<boost::shared_ptr<const IComponent> >
    // anc=(*(countsWS->getSpectrum(i)->getDetectorIDs().begin()))->getAncestors();
    if (anc.size() < static_cast<size_t>(m_parents)) {
      g_log.warning("Too many levels up. Will ignore LevelsUp");
      m_parents = 0;
      return makeInstrumentMap(countsWS);
    }
    mymap.insert(std::pair<Mantid::Geometry::ComponentID, size_t>(
        anc[m_parents - 1]->getComponentID(), i));
  }

  std::vector<std::vector<size_t>> speclist;
  std::vector<size_t> speclistsingle;

  std::multimap<Mantid::Geometry::ComponentID, size_t>::iterator m_it, s_it;

  for (m_it = mymap.begin(); m_it != mymap.end(); m_it = s_it) {
    Mantid::Geometry::ComponentID theKey = (*m_it).first;

    std::pair<std::multimap<Mantid::Geometry::ComponentID, size_t>::iterator,
              std::multimap<Mantid::Geometry::ComponentID, size_t>::iterator>
        keyRange = mymap.equal_range(theKey);

    // Iterate over all map elements with key == theKey
    speclistsingle.clear();
    for (s_it = keyRange.first; s_it != keyRange.second; ++s_it) {
      speclistsingle.push_back((*s_it).second);
    }
    speclist.push_back(speclistsingle);
  }

  return speclist;
}
/**
 *  Finds the median of values in single bin histograms rejecting spectra from
 * masked
 *  detectors and the results of divide by zero (infinite and NaN).
 * The median is an average that is less affected by small numbers of very large
 * values.
 * @param input :: A histogram workspace with one entry in each bin
 * @param excludeZeroes :: If true then zeroes will not be included in the
 * median calculation
 * @param indexmap :: indexmap
 * @return The median value of the histograms in the workspace that was passed
 * to it
 * @throw out_of_range if a value is negative
 */
std::vector<double>
DetectorDiagnostic::calculateMedian(const API::MatrixWorkspace_sptr input,
                                    bool excludeZeroes,
                                    std::vector<std::vector<size_t>> indexmap) {
  std::vector<double> medianvec;
  g_log.debug("Calculating the median count rate of the spectra");

  for (size_t j = 0; j < indexmap.size(); ++j) {
    std::vector<double> medianInput;
    std::vector<size_t> hists = indexmap.at(j);

    const int nhists = static_cast<int>(hists.size());
    // The maximum possible length is that of workspace length
    medianInput.reserve(nhists);

    bool checkForMask = false;
    Geometry::Instrument_const_sptr instrument = input->getInstrument();
    if (instrument != NULL) {
      checkForMask = ((instrument->getSource() != NULL) &&
                      (instrument->getSample() != NULL));
    }

    PARALLEL_FOR1(input)
    for (int i = 0; i < static_cast<int>(hists.size()); ++i) {
      PARALLEL_START_INTERUPT_REGION

      if (checkForMask) {
        const std::set<detid_t> &detids =
            input->getSpectrum(hists[i])->getDetectorIDs();
        if (instrument->isDetectorMasked(detids))
          continue;
        if (instrument->isMonitor(detids))
          continue;
      }

      const double yValue = input->readY(hists[i])[0];
      if (yValue < 0.0) {
        throw std::out_of_range("Negative number of counts found, could be "
                                "corrupted raw counts or solid angle data");
      }
      if (boost::math::isnan(yValue) || boost::math::isinf(yValue) ||
          (excludeZeroes && yValue < DBL_EPSILON)) // NaNs/Infs
      {
        continue;
      }
      // Now we have a good value
      PARALLEL_CRITICAL(DetectorDiagnostic_median_d) {
        medianInput.push_back(yValue);
      }

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    if (medianInput.empty()) {
      g_log.information(
          "some group has no valid histograms. Will use 0 for median.");
      medianInput.push_back(0.);
    }

    // We need a sorted array to calculate the median
    std::sort(medianInput.begin(), medianInput.end());
    double median = gsl_stats_median_from_sorted_data(&medianInput[0], 1,
                                                      medianInput.size());

    if (median < 0 || median > DBL_MAX / 10.0) {
      throw std::out_of_range("The calculated value for the median was either "
                              "negative or unreliably large");
    }
    medianvec.push_back(median);
  }
  return medianvec;
}

/**
 * Convert to a distribution
 * @param workspace :: The input workspace to convert to a count rate
 * @return distribution workspace with equiv. data
 */
API::MatrixWorkspace_sptr
DetectorDiagnostic::convertToRate(API::MatrixWorkspace_sptr workspace) {
  if (workspace->isDistribution()) {
    g_log.information()
        << "Workspace already contains a count rate, nothing to do.\n";
    return workspace;
  }

  g_log.information("Calculating time averaged count rates");
  // get percentage completed estimates for now, t0 and when we've finished t1
  double t0 = m_fracDone, t1 = advanceProgress(RTGetRate);
  IAlgorithm_sptr childAlg =
      createChildAlgorithm("ConvertToDistribution", t0, t1);
  childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", workspace);
  // Now execute the Child Algorithm but allow any exception to bubble up
  childAlg->execute();
  return childAlg->getProperty("Workspace");
}

/** Update the percentage complete estimate assuming that the algorithm
 * has completed a task with the given estimated run time
 * @param toAdd :: the estimated additional run time passed since the last
 * update,
 * where m_TotalTime holds the total algorithm run time
 * @return estimated fraction of algorithm runtime that has passed so far
 */
double DetectorDiagnostic::advanceProgress(double toAdd) {
  m_fracDone += toAdd / m_TotalTime;
  // it could go negative as sometimes the percentage is re-estimated backwards,
  // this is worrying about if a small negative value will cause a problem some
  // where
  m_fracDone = std::abs(m_fracDone);
  interruption_point();
  return m_fracDone;
}

/** Update the percentage complete estimate assuming that the algorithm aborted
 * a task with the given
 *  estimated run time
 * @param aborted :: the amount of algorithm run time that was saved by aborting
 * a
 * part of the algorithm, where m_TotalTime holds the total algorithm run time
 */
void DetectorDiagnostic::failProgress(RunTime aborted) {
  advanceProgress(-aborted);
  m_TotalTime -= aborted;
}
}
}
