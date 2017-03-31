#include "MantidAlgorithms/ReflectometryReductionOne2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Unit.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryReductionOne2)

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void ReflectometryReductionOne2::init() {

  // Input workspace
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Run to reduce.");

  // Reduction type
  std::vector<std::string> reductionTypes = {"Normal", "DivergentBeam",
                                             "NonFlatSample"};
  std::string defaultCorrection = "None";
  declareProperty("ReductionType", "Normal",
                  boost::make_shared<StringListValidator>(reductionTypes),
                  "The type of reduction to perform.");

  // Theta0
  declareProperty("Theta0", 0.0, "Horizon angle in degrees", Direction::Input);

  // Processing instructions
  declareProperty(Kernel::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "",
                      boost::make_shared<MandatoryValidator<std::string>>(),
                      Direction::Input),
                  "Grouping pattern on workspace indexes to yield only "
                  "the detectors of interest. See GroupDetectors for details.");

  // Minimum wavelength
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "WavelengthMin", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength minimum in angstroms");

  // Maximum wavelength
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "WavelengthMax", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength maximum in angstroms");

  // Properties for direct beam normalization
  initDirectBeamProperties();

  // Init properties for monitors
  initMonitorProperties();
  // Normalization by integrated monitors
  declareProperty("NormalizeByIntegratedMonitors", true,
                  "Normalize by dividing by the integrated monitors.");

  // Init properties for transmission normalization
  initTransmissionProperties();

  // Init properties for algorithmic corrections
  initAlgorithmicProperties();

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output Workspace IvsQ.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceWavelength",
                                                   "", Direction::Output,
                                                   PropertyMode::Optional),
                  "Output Workspace IvsLam. Intermediate workspace.");
}

/** Validate inputs
*/
std::map<std::string, std::string>
ReflectometryReductionOne2::validateInputs() {

  std::map<std::string, std::string> results;

  const auto wavelength = validateWavelengthRanges();
  results.insert(wavelength.begin(), wavelength.end());

  const auto directBeam = validateDirectBeamProperties();
  results.insert(directBeam.begin(), directBeam.end());

  const auto transmission = validateTransmissionProperties();
  results.insert(transmission.begin(), transmission.end());

  return results;
}

/**
* Initialise class members for a particular run
*/
void ReflectometryReductionOne2::initRun() {
  m_runWS = getProperty("InputWorkspace");
  m_spectrumInfo = &m_runWS->spectrumInfo();

  const auto xUnitID = m_runWS->getAxis(0)->unit()->unitID();

  // Neither TOF or Lambda? Abort.
  if ((xUnitID != "Wavelength") && (xUnitID != "TOF"))
    throw std::invalid_argument(
        "InputWorkspace must have units of TOF or Wavelength");

  m_convertUnits = true;
  m_normalise = true;
  m_sum = true;

  if (xUnitID == "Wavelength") {
    // Assume already reduced (converted, normalised and summed)
    m_convertUnits = false;
    m_normalise = false;
    // sum = false;  // temp for testing - still do summation
  }

  findDetectorsOfInterest();

  if (summingInQ()) {
    // These values are only required for summation in Q
    findLambdaMinMax();
    findTwoThetaMinMax();
    findTheta0();
    findTwoThetaR();
  }
}

/** Execute the algorithm.
*/
void ReflectometryReductionOne2::exec() {
  // Set up member variables from inputs for this run
  initRun();

  // Create the output workspace in wavelength
  MatrixWorkspace_sptr IvsLam = makeIvsLam();

  // Convert to Q
  auto IvsQ = convertToQ(IvsLam);

  setProperty("OutputWorkspaceWavelength", IvsLam);
  setProperty("OutputWorkspace", IvsQ);
}

/**
* Creates the output 1D array in wavelength from an input 2D workspace in
* TOF. Summation is done over lambda or over lines of constant Q depending on
* the type of reduction. For the latter, the output is projected to "virtual
* lambda" at a reference angle twoThetaR.
*
* @return :: the output workspace in wavelength
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::makeIvsLam() {
  MatrixWorkspace_sptr result = m_runWS;

  if (summingInQ()) {
    // Convert to lambda
    if (m_convertUnits) {
      result = convertToWavelength(result);
    }
    // Normalise
    if (m_normalise) {
      result = directBeamCorrection(result);
      result = monitorCorrection(result);
      result = transOrAlgCorrection(result, false);
    }
    // Do the summation in Q
    if (m_sum) {
      result = sumInQ(result);
    }
  } else {
    // Do the summation in lambda
    if (m_sum) {
      result = makeDetectorWS(result, m_convertUnits);
    }
    // Normalise the 1D result
    if (m_normalise) {
      result = directBeamCorrection(result);
      result = monitorCorrection(result);
      result = transOrAlgCorrection(result, true);
    }
  }

  // Crop to wavelength limits
  result = cropWavelength(result);

  return result;
}

/**
* Normalize by monitors (only if I0MonitorIndex, MonitorBackgroundWavelengthMin
* and MonitorBackgroundWavelengthMax have been given)
*
* @param detectorWS :: the detector workspace to normalise, in lambda
* @param runWS :: the original run workspace in TOF
* @return :: the normalized workspace in lambda
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::monitorCorrection(MatrixWorkspace_sptr detectorWS) {
  MatrixWorkspace_sptr IvsLam;
  Property *monProperty = getProperty("I0MonitorIndex");
  Property *backgroundMinProperty =
      getProperty("MonitorBackgroundWavelengthMin");
  Property *backgroundMaxProperty =
      getProperty("MonitorBackgroundWavelengthMin");
  if (!monProperty->isDefault() && !backgroundMinProperty->isDefault() &&
      !backgroundMaxProperty->isDefault()) {
    const bool integratedMonitors =
        getProperty("NormalizeByIntegratedMonitors");
    const auto monitorWS = makeMonitorWS(m_runWS, integratedMonitors);
    if (!integratedMonitors)
      detectorWS = rebinDetectorsToMonitors(detectorWS, monitorWS);
    IvsLam = divide(detectorWS, monitorWS);
  } else {
    IvsLam = detectorWS;
  }

  return IvsLam;
}

/** Creates a direct beam workspace in wavelength from an input workspace in
* TOF. This method should only be called if RegionOfDirectBeam is provided.
*
* @param inputWS :: the input workspace in TOF
* @return :: the direct beam workspace in wavelength
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::makeDirectBeamWS(MatrixWorkspace_sptr inputWS) {

  std::vector<int> directBeamRegion = getProperty("RegionOfDirectBeam");
  // Sum over the direct beam.
  const std::string processingCommands = std::to_string(directBeamRegion[0]) +
                                         "-" +
                                         std::to_string(directBeamRegion[1]);

  auto groupDirectBeamAlg = this->createChildAlgorithm("GroupDetectors");
  groupDirectBeamAlg->initialize();
  groupDirectBeamAlg->setProperty("GroupingPattern", processingCommands);
  groupDirectBeamAlg->setProperty("InputWorkspace", inputWS);
  groupDirectBeamAlg->execute();
  MatrixWorkspace_sptr directBeamWS =
      groupDirectBeamAlg->getProperty("OutputWorkspace");

  directBeamWS = convertToWavelength(directBeamWS);

  return directBeamWS;
}

/**
* Normalize the workspace by the direct beam (optional)
*
* @param detectorWS : workspace in wavelength which is to be normalized
* @param runWS :: the original run workspace in TOF
* @return : corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::directBeamCorrection(
    MatrixWorkspace_sptr detectorWS) {

  MatrixWorkspace_sptr normalized = detectorWS;
  Property *directBeamProperty = getProperty("RegionOfDirectBeam");
  if (!directBeamProperty->isDefault()) {
    const auto directBeam = makeDirectBeamWS(m_runWS);
    normalized = divide(detectorWS, directBeam);
  }

  return normalized;
}

/**
* Perform either transmission or algorithmic correction according to the
* settings.
* @param detectorWS : workspace in wavelength which is to be normalized
* @return : corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::transOrAlgCorrection(
    MatrixWorkspace_sptr detectorWS, const bool detectorWSReduced) {

  MatrixWorkspace_sptr normalized;
  MatrixWorkspace_sptr transRun = getProperty("FirstTransmissionRun");
  if (transRun) {
    normalized = transmissionCorrection(detectorWS, detectorWSReduced);
  } else if (getPropertyValue("CorrectionAlgorithm") != "None") {
    normalized = algorithmicCorrection(detectorWS);
  } else {
    normalized = detectorWS;
  }

  return normalized;
}

/** Perform transmission correction by running 'CreateTransmissionWorkspace' on
* the input workspace
* @param detectorWS :: the input workspace
* @param detectorWSReduced:: the input detector workspace has been reduced
* @return :: the input workspace normalized by transmission
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::transmissionCorrection(
    MatrixWorkspace_sptr detectorWS, const bool detectorWSReduced) {

  const bool strictSpectrumChecking = getProperty("StrictSpectrumChecking");
  MatrixWorkspace_sptr transmissionWS = getProperty("FirstTransmissionRun");

  // Reduce the transmission workspace, if not already done (assume that if
  // the workspace is in wavelength then it has already been reduced)
  Unit_const_sptr xUnit = transmissionWS->getAxis(0)->unit();
  if (xUnit->unitID() == "TOF") {

    // Processing instructions for transmission workspace. If strict spectrum
    // checking is not enabled then just use the same processing instructions
    // that were passed in.
    std::string transmissionCommands = getProperty("ProcessingInstructions");
    if (strictSpectrumChecking) {
      // If we have strict spectrum checking, we should have the same
      // spectrum numbers in both workspaces, but not necessarily with the
      // same workspace indices. Therefore, map the processing instructions
      // from the original workspace to the correct indices in the
      // transmission workspace. Note that we use the run workspace here
      // because the detectorWS may already have been reduced and may not
      // contain the original spectra.
      transmissionCommands =
          createProcessingCommandsFromDetectorWS(m_runWS, transmissionWS);
    }

    MatrixWorkspace_sptr secondTransmissionWS =
        getProperty("SecondTransmissionRun");
    auto alg = this->createChildAlgorithm("CreateTransmissionWorkspace");
    alg->initialize();
    alg->setProperty("FirstTransmissionRun", transmissionWS);
    alg->setProperty("SecondTransmissionRun", secondTransmissionWS);
    alg->setPropertyValue("Params", getPropertyValue("Params"));
    alg->setPropertyValue("StartOverlap", getPropertyValue("StartOverlap"));
    alg->setPropertyValue("EndOverlap", getPropertyValue("EndOverlap"));
    alg->setPropertyValue("I0MonitorIndex", getPropertyValue("I0MonitorIndex"));
    alg->setPropertyValue("WavelengthMin", getPropertyValue("WavelengthMin"));
    alg->setPropertyValue("WavelengthMax", getPropertyValue("WavelengthMax"));
    alg->setPropertyValue("MonitorBackgroundWavelengthMin",
                          getPropertyValue("MonitorBackgroundWavelengthMin"));
    alg->setPropertyValue("MonitorBackgroundWavelengthMax",
                          getPropertyValue("MonitorBackgroundWavelengthMax"));
    alg->setPropertyValue("MonitorIntegrationWavelengthMin",
                          getPropertyValue("MonitorIntegrationWavelengthMin"));
    alg->setPropertyValue("MonitorIntegrationWavelengthMax",
                          getPropertyValue("MonitorIntegrationWavelengthMax"));
    alg->setProperty("ProcessingInstructions", transmissionCommands);
    alg->execute();
    transmissionWS = alg->getProperty("OutputWorkspace");
  }

  // Rebin the transmission run to be the same as the input.
  auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
  rebinToWorkspaceAlg->initialize();
  rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", detectorWS);
  rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", transmissionWS);
  rebinToWorkspaceAlg->execute();
  transmissionWS = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

  // If the detector workspace has been reduced then the spectrum maps
  // should match AFTER reducing the transmission workspace
  if (detectorWSReduced) {
    verifySpectrumMaps(detectorWS, transmissionWS, strictSpectrumChecking);
  }

  MatrixWorkspace_sptr normalized = divide(detectorWS, transmissionWS);
  return normalized;
}

/**
* Perform transmission correction using alternative correction algorithms
* @param detectorWS : workspace in wavelength which is to be normalized by the
* results of the transmission corrections.
* @return : corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::algorithmicCorrection(
    MatrixWorkspace_sptr detectorWS) {

  const std::string corrAlgName = getProperty("CorrectionAlgorithm");

  IAlgorithm_sptr corrAlg = createChildAlgorithm(corrAlgName);
  corrAlg->initialize();
  if (corrAlgName == "PolynomialCorrection") {
    corrAlg->setPropertyValue("Coefficients", getPropertyValue("Polynomial"));
  } else if (corrAlgName == "ExponentialCorrection") {
    corrAlg->setPropertyValue("C0", getPropertyValue("C0"));
    corrAlg->setPropertyValue("C1", getPropertyValue("C1"));
  } else {
    throw std::runtime_error("Unknown correction algorithm: " + corrAlgName);
  }

  corrAlg->setProperty("InputWorkspace", detectorWS);
  corrAlg->setProperty("Operation", "Divide");
  corrAlg->execute();

  return corrAlg->getProperty("OutputWorkspace");
}

/**
* The input workspace (in wavelength) to convert to Q
* @param inputWS : the input workspace to convert
* @return : output workspace in Q
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::convertToQ(MatrixWorkspace_sptr inputWS) {

  // Convert to Q
  auto convertUnits = this->createChildAlgorithm("ConvertUnits");
  convertUnits->initialize();
  convertUnits->setProperty("InputWorkspace", inputWS);
  convertUnits->setProperty("Target", "MomentumTransfer");
  convertUnits->setProperty("AlignBins", false);
  convertUnits->execute();
  MatrixWorkspace_sptr IvsQ = convertUnits->getProperty("OutputWorkspace");

  return IvsQ;
}

/**
* Determine whether the reduction should sum along lines of constant
* Q or in the default lambda.
*
* @return : true if the reduction should sum in Q; false otherwise
*/
bool ReflectometryReductionOne2::summingInQ() {
  bool result = false;
  const std::string reductionType = getProperty("ReductionType");

  if (reductionType == "DivergentBeam" || reductionType == "NonFlatSample") {
    result = true;
  }

  return result;
}

/**
* Find and cache the min/max lambda in the region of interest
*/
void ReflectometryReductionOne2::findLambdaMinMax() {
  m_lambdaMin = getProperty("WavelengthMin");
  m_lambdaMax = getProperty("WavelengthMax");
}

/**
* Find and cache the indicies of the detectors of interest
*/
void ReflectometryReductionOne2::findDetectorsOfInterest() {
  std::string instructions = getPropertyValue("ProcessingInstructions");

  // Get the min and max spectrum indicies from the processing instructions, if
  // given, or default to the overall min and max
  size_t minDetectorIdx = 0;
  size_t maxDetectorIdx = static_cast<int>(m_spectrumInfo->size()) - 1;
  bool done = false;

  if (!instructions.empty() &&
      !std::all_of(instructions.begin(), instructions.end(), isspace)) {
    try {
      // The processing instructions should be in the format <start>-<end> or
      // <a>+<b>+<c>... At the moment we only support a single range or set of
      // values but not combinations of both.

      /// todo Add support for the '+' operator in the GroupingPattern. Could
      /// we also support ':' and ',' operators? These would result in more
      /// than one row in the detector workspace.
      std::vector<std::string> matches;

      // Look for a single occurance of '-' (only support a single range for
      // now)
      boost::split(matches, instructions, boost::is_any_of("-"));

      if (matches.size() == 2) {
        minDetectorIdx = std::stoi(matches[0]);
        if (matches.size() > 1) {
          maxDetectorIdx = std::stoi(matches[1]);
        } else {
          maxDetectorIdx = minDetectorIdx;
        }
      } else {
        // Look for '+'
        boost::split(matches, instructions, boost::is_any_of("+"));
        if (matches.size() > 0) {
          for (auto match : matches) {
            m_detectors.push_back(std::stoi(match));
          }
          done = true;
        }
      }

      // Also set the reference detector index as the centre of the
      // region of interest
      m_twoThetaRDetectorIdx =
          minDetectorIdx + (maxDetectorIdx - minDetectorIdx) / 2;
    } catch (std::exception &ex) {
      std::ostringstream errMsg;
      errMsg << "Error reading processing instructions '" << instructions
             << "'; " << ex.what();
      throw std::runtime_error(errMsg.str());
    }
  }

  if (!done) {
    // Add each detector index in the range to the list
    for (size_t i = minDetectorIdx; i <= maxDetectorIdx; ++i) {
      m_detectors.push_back(i);
    }
  }
}

/**
* Find and cache the min/max twoTheta for the area of interest
*/
void ReflectometryReductionOne2::findTwoThetaMinMax() {

  m_twoThetaMin = std::min(m_spectrumInfo->twoTheta(m_detectors.front()),
                           m_spectrumInfo->twoTheta(m_detectors.back()));

  m_twoThetaMax = std::max(m_spectrumInfo->twoTheta(m_detectors.front()),
                           m_spectrumInfo->twoTheta(m_detectors.back()));
}

/**
* Find and cache the horizon angle theta0 for use for summation in Q.
*/
void ReflectometryReductionOne2::findTheta0() {
  const std::string reductionType = getProperty("ReductionType");

  m_theta0 = 0.0;

  if (reductionType == "DivergentBeam") {
    // theta0 is at the angle at the centre of the detector. This is the
    // angle the detector has been rotated around and should be defined in
    m_theta0 = getProperty("Theta0");
  }
}

/**
* Find and cache the (arbitrary) reference angle twoThetaR for use for summation
* in
* Q, and the detector pixel it corresponds to.
*
* @return : the angle twoThetaR
* @throws : if the angle could not be found
*/
void ReflectometryReductionOne2::findTwoThetaR() {
  m_twoThetaR = m_spectrumInfo->twoTheta(twoThetaRDetectorIdx());
}

/**
* Construct an "empty" output workspace in virtual-lambda for summation in Q.
* The workspace will have the same x values as the input workspace but the y
* values will all be zero.
*
* @return : a 1D workspace where y values are all zero
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::constructIvsLamWS(MatrixWorkspace_sptr detectorWS) {
  // Construct the new workspace from the centre pixel (where twoThetaR is
  // defined)
  auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", detectorWS);
  cropWorkspaceAlg->setProperty("StartWorkspaceIndex",
                                static_cast<int>(twoThetaRDetectorIdx()));
  cropWorkspaceAlg->setProperty("EndWorkspaceIndex",
                                static_cast<int>(twoThetaRDetectorIdx()));
  cropWorkspaceAlg->execute();
  MatrixWorkspace_sptr ws = cropWorkspaceAlg->getProperty("OutputWorkspace");

  // Get the max and min values of the projected (virtual) lambda range
  double lambdaVMin = 0.0;
  double lambdaVMax = 0.0;
  double dummy = 0.0;
  const int numBins = static_cast<int>(detectorWS->blocksize());
  const double bLambda = (lambdaMax() - lambdaMin()) / numBins;
  const double bTwoTheta =
      (twoThetaMax() - twoThetaMin()) / detectorWS->getNumberHistograms();
  getProjectedLambdaRange(lambdaMax(), twoThetaMin(), bLambda, bTwoTheta,
                          lambdaVMin, dummy);
  getProjectedLambdaRange(lambdaMin(), twoThetaMax(), bLambda, bTwoTheta, dummy,
                          lambdaVMax);

  if (lambdaVMin > lambdaVMax) {
    std::swap(lambdaVMin, lambdaVMax);
  }

  // Use the same number of bins as the input
  const double binWidth = (lambdaVMax - lambdaVMin) / numBins;
  std::stringstream params;
  params << lambdaMin() << "," << binWidth << "," << lambdaMax();

  auto rebinAlg = this->createChildAlgorithm("Rebin");
  rebinAlg->initialize();
  rebinAlg->setProperty("InputWorkspace", ws);
  rebinAlg->setProperty("PreserveEvents", false);
  rebinAlg->setProperty("Params", params.str());
  rebinAlg->execute();
  ws = rebinAlg->getProperty("OutputWorkspace");

  for (int i = 0; i < ws->blocksize(); ++i) {
    ws->dataY(0)[i] = 0.0;
  }

  return ws;
}

/**
* Sum counts from the input workspace in lambda along lines of constant Q by
* projecting to "virtual lambda" at a reference angle twoThetaR.
*
* @param detectorWS :: the input workspace in wavelength
* @return :: the output workspace in wavelength
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::sumInQ(MatrixWorkspace_sptr detectorWS) {
  // Construct the output array in virtual lambda
  MatrixWorkspace_sptr IvsLam = constructIvsLamWS(detectorWS);

  // Loop through each spectrum in the region of interest
  const auto &spectrumInfo = detectorWS->spectrumInfo();
  for (size_t spIdx : m_detectors) {
    // Get the angle of this detector and its size in twoTheta
    double twoTheta = 0.0;
    double bTwoTheta = 0.0;
    getDetectorDetails(spIdx, spectrumInfo, twoTheta, bTwoTheta);

    // Check X length is Y length + 1
    const auto &inputX = detectorWS->x(spIdx);
    const auto &inputY = detectorWS->y(spIdx);
    if (inputX.size() != inputY.size() + 1) {
      std::ostringstream errMsg;
      errMsg << "Expected input workspace to be histogram data (got X len="
             << inputX.size() << ", Y len=" << inputY.size() << ")";
      throw std::runtime_error(errMsg.str());
    }

    // Process each value in the spectrum
    for (int inputIdx = 0; inputIdx < inputY.size(); ++inputIdx) {
      sumInQProcessValue(inputIdx, twoTheta, bTwoTheta, inputX, inputY, IvsLam);
    }
  }

  return IvsLam;
}

/**
* Share counts from an input value onto the projected output in virtual-lambda
*
* @param inputIdx [in] :: the index into the input arrays
* @param twoTheta [in] :: the value of twotTheta for this spectrum
* @param bTwoTheta [in] :: the size of the pixel in twoTheta
* @param inputY [in] :: the input spectrum Y values
* @param inputX [in] :: the input spectrum Y values
* @param IvsLam [in,out] :: the output workspace
*/
void ReflectometryReductionOne2::sumInQProcessValue(
    const int inputIdx, const double twoTheta, const double bTwoTheta,
    const HistogramX &inputX, const HistogramY &inputY,
    MatrixWorkspace_sptr IvsLam) {

  // Get the bin width and the bin centre
  const double bLambda = inputX[inputIdx + 1] - inputX[inputIdx];
  const double lambda = inputX[inputIdx] + bLambda / 2.0;
  // Skip if outside area of interest
  if (lambda < lambdaMin() || lambda > lambdaMax()) {
    return;
  }
  // Project these coordinates onto the virtual-lambda output (at twoThetaR)
  double lambdaMin = 0.0;
  double lambdaMax = 0.0;
  getProjectedLambdaRange(lambda, twoTheta, bLambda, bTwoTheta, lambdaMin,
                          lambdaMax);
  // Share the input counts into the output array
  const double inputCounts = inputY[inputIdx];
  sumInQShareCounts(inputCounts, bLambda, lambdaMin, lambdaMax, IvsLam);
}

/**
* Share the given input counts into the output array bins proportionally
* according to how much the bins overlap the given lambda range.
* outputX.size() must equal outputY.size() + 1
*
* @param inputCounts [in] :: the input counts to share out
* @param bLambda [in] :: the bin width in lambda
* @param lambdaMin [in] :: the start of the range to share counts to
* @param lambdaMax [in] :: the end of the range to share counts to
* @param IvsLam [in,out] :: the output workspace
*/
void ReflectometryReductionOne2::sumInQShareCounts(
    const double inputCounts, const double bLambda, const double lambdaMin,
    const double lambdaMax, MatrixWorkspace_sptr IvsLam) {
  // Check that we have histogram data
  const auto &outputX = IvsLam->dataX(0);
  auto &outputY = IvsLam->dataY(0);
  if (outputX.size() != outputY.size() + 1) {
    std::ostringstream errMsg;
    errMsg << "Expected output array to be histogram data (got X len="
           << outputX.size() << ", Y len=" << outputY.size() << ")";
    throw std::runtime_error(errMsg.str());
  }

  const double totalWidth = lambdaMax - lambdaMin;

  // Get the first bin edge in the output X array that is within range.
  // There will probably be some overlap, so start from the bin edge before
  // this (unless we're already at the first bin edge).
  auto startIter = std::lower_bound(outputX.begin(), outputX.end(), lambdaMin);
  if (startIter != outputX.begin()) {
    --startIter;
  }

  // Loop through all overlapping output bins. Convert the iterator to an
  // index because we need to index both the X and Y arrays.
  for (auto outputIdx = startIter - outputX.begin();
       outputIdx < outputX.size() - 1; ++outputIdx) {
    const double binStart = outputX[outputIdx];
    const double binEnd = outputX[outputIdx + 1];
    if (binStart > lambdaMax) {
      // No longer in the overlap region so we're finished
      break;
    }
    // Add a share of the input counts to this bin based on the proportion of
    // overlap.
    const double overlapWidth =
        std::min({bLambda, lambdaMax - binStart, binEnd - lambdaMin});
    outputY[outputIdx] += (inputCounts * overlapWidth) / (totalWidth);
  }
}

/**
* Get the angle for this detector
*
* @param spIdx : the spectrum index
* @param spectrumInfo : the cached spectrum info
* @param twoTheta [out] : the twoTheta angle of the detector
* @param bTwoTheta [out] : the range twoTheta covered by this detector
*/
void ReflectometryReductionOne2::getDetectorDetails(
    const size_t spIdx, const SpectrumInfo &spectrumInfo, double &twoTheta,
    double &bTwoTheta) {

  twoTheta = spectrumInfo.twoTheta(spIdx);

  // Get the range of covered by this pixel (assume it's the diff between this
  // pixel's twoTheta and the prev/next pixel) / todo - check this
  if (spIdx > 0) {
    bTwoTheta = twoTheta - spectrumInfo.twoTheta(spIdx - 1);
  } else if (spIdx + 1 < spectrumInfo.size()) {
    bTwoTheta = spectrumInfo.twoTheta(spIdx + 1) - twoTheta;
  }
}

/**
* Get the projected wavelength range onto the line theataR from the given
* coordinates.
*
* @param lambda [in] :: the lambda coord to project
* @param twoTheta [in] :: the twoTheta coord to project
* @param bLambda [in] :: the pixel size in lambda
* @param bTwoTheta [in] :: the pixel size in twoTheta
* @param lambdaMin [out] :: the projected range start
* @param lambdaMax [out] :: the projected range end
*/
void ReflectometryReductionOne2::getProjectedLambdaRange(
    const double lambda, const double twoTheta, const double bLambda,
    const double bTwoTheta, double &lambdaMin, double &lambdaMax) {
  // Get the angle from twoThetaR to this detector
  const double gamma = twoTheta - twoThetaR();
  // Get the angle from the horizon to this detector
  const double theta = twoTheta - theta0();

  // Calculate the projected wavelength range
  try {
    const double lambdaTop = std::sin(theta) * (lambda + bLambda / 2) /
                             std::sin(theta + gamma - bTwoTheta / 2);
    const double lambdaBot = std::sin(theta) * (lambda - bLambda / 2) /
                             std::sin(theta + gamma + bTwoTheta / 2);

    lambdaMin = std::min(lambdaTop, lambdaBot);
    lambdaMax = std::max(lambdaTop, lambdaBot);
  } catch (std::exception &ex) {
    std::stringstream errMsg;
    errMsg << "Failed to project (lambda, twoTheta) = (" << lambda << ","
           << twoTheta << ") onto twoThetaR = " << twoThetaR() << ": "
           << ex.what();
    throw std::runtime_error(errMsg.str());
  }
}

/**
Check whether theif spectrum maps for the given workspaces are the same.

@param ws1 : First workspace to compare
@param ws2 : Second workspace to compare against
@param severe: True to indicate that failure to verify should result in an
exception. Otherwise a warning is generated.
*/
void ReflectometryReductionOne2::verifySpectrumMaps(
    MatrixWorkspace_const_sptr ws1, MatrixWorkspace_const_sptr ws2,
    const bool severe) {
  auto map1 = ws1->getSpectrumToWorkspaceIndexMap();
  auto map2 = ws2->getSpectrumToWorkspaceIndexMap();
  if (map1 != map2) {
    const std::string message =
        "Spectrum maps between workspaces do NOT match up.";
    if (severe) {
      throw std::invalid_argument(message);
    } else {
      g_log.warning(message);
    }
  }
}

/**
* Translate all the workspace indexes in an origin workspace into workspace
* indexes of a host end-point workspace. This is done using spectrum numbers as
* the intermediate.
*
* @param originWS : Origin workspace, which provides the original workspace
* index to spectrum number mapping.
* @param hostWS : Workspace onto which the resulting workspace indexes will be
* hosted
* @throws :: If the specId are not found to exist on the host end-point
*workspace.
* @return :: Remapped workspace indexes applicable for the host workspace,
*as comma separated string.
*/
std::string ReflectometryReductionOne2::createProcessingCommandsFromDetectorWS(
    MatrixWorkspace_const_sptr originWS, MatrixWorkspace_const_sptr hostWS) {
  auto map = originWS->getSpectrumToWorkspaceIndexMap();
  std::stringstream result;

  // Get the start and end of the range of detectors.
  /// todo Add support for different GroupingPatterns rather than just a single
  /// range?
  const int hostIdxStart =
      mapSpectrumIndexToWorkspace(map, m_detectors.front(), hostWS);
  const int hostIdxEnd =
      mapSpectrumIndexToWorkspace(map, m_detectors.back(), hostWS);
  result << hostIdxStart << "-" << hostIdxEnd;

  return result.str();
}

/**
* Map a spectrum index from the given map to the given workspace
* @param map : the original spectrum number to index map
* @param mapIdx : the index to look up in the map
* @param destWS : the destination workspace
*/
int ReflectometryReductionOne2::mapSpectrumIndexToWorkspace(
    const spec2index_map &map, const size_t mapIdx,
    MatrixWorkspace_const_sptr destWS) {

  // Get the spectrum numbers for these indices
  auto it = std::find_if(map.begin(), map.end(),
                         [mapIdx](auto wsIt) { return wsIt.second == mapIdx; });

  if (it == map.end()) {
    std::ostringstream errMsg;
    errMsg << "Workspace index " << mapIdx << " not found in run workspace ";
    throw std::runtime_error(errMsg.str());
  }
  specnum_t specId = it->first;

  // Get the workspace index in the destination workspace
  int wsIdx = static_cast<int>(destWS->getIndexFromSpectrumNumber(specId));

  return wsIdx;
}
} // namespace Algorithms
} // namespace Mantid
