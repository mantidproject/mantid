#include "MantidAlgorithms/ReflectometryReductionOne2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::HistogramData;

// unnamed namespace
namespace {
/** Get the twoTheta angle for the centre of the detector associated with the
* given spectrum
*
* @return : the twoTheta angle in degrees
*/
double getDetectorTwoTheta(const SpectrumInfo *spectrumInfo,
                           const size_t spectrumIdx) {
  double twoTheta = spectrumInfo->signedTwoTheta(spectrumIdx);
  return twoTheta * 180 / M_PI;
}

/** Get the twoTheta angle range for the top/bottom of the detector associated
* with the given spectrum
*
* @return : the twoTheta angle in degrees
*/
double getDetectorTwoThetaRange(const SpectrumInfo *spectrumInfo,
                                const size_t spectrumIdx) {
  // Assume the range covered by this pixel is the diff between this
  // pixel's twoTheta and the next/prev pixel)
  double twoTheta = getDetectorTwoTheta(spectrumInfo, spectrumIdx);
  double bTwoTheta = 0;

  if (spectrumIdx + 1 < spectrumInfo->size()) {
    bTwoTheta = getDetectorTwoTheta(spectrumInfo, spectrumIdx + 1) - twoTheta;
  }

  return bTwoTheta;
}

/** Get the start/end of the lambda range for the detector associated
* with the given spectrum
*
* @return : the lambda range
*/
double getLambdaRange(const HistogramX &xValues, const int idx) {
  // The lambda range is the bin width from the given index to the next.
  if (idx + 1 >= xValues.size()) {
    std::ostringstream errMsg;
    errMsg << "Error accessing X values out of range (index=" << idx + 1
           << ", size=" << xValues.size();
    throw std::runtime_error(errMsg.str());
  }

  double result = xValues[idx + 1] - xValues[idx];
  return result;
}

/** Get the start/end of the lambda range for the detector associated
* with the given spectrum
*
* @return : the lambda range
*/
double getLambdaRange(MatrixWorkspace_const_sptr ws, const size_t spectrumIdx) {
  return getLambdaRange(ws->x(spectrumIdx), static_cast<int>(spectrumIdx));
}

/** Get the lambda value at the centre of the detector associated
* with the given spectrum
*
* @return : the lambda range
*/
double getLambda(const HistogramX &xValues, const int idx) {
  if (idx >= xValues.size()) {
    std::ostringstream errMsg;
    errMsg << "Error accessing X values out of range (index=" << idx
           << ", size=" << xValues.size();
    throw std::runtime_error(errMsg.str());
  }

  // The centre of the bin is the lower bin edge plus half the width
  return xValues[idx] + getLambdaRange(xValues, idx) / 2.0;
}

/*
Get the value of theta from the logs
@param inputWs : the input workspace
@return : theta found in the logs
@throw: runtime_errror if 'stheta' was not found.
*/
double getThetaFromLogs(MatrixWorkspace_sptr inputWs) {

  double theta = -1.;
  const Mantid::API::Run &run = inputWs->run();
  try {
    Property *p = run.getLogData("stheta");
    auto incidentThetas = dynamic_cast<TimeSeriesProperty<double> *>(p);
    if (!incidentThetas) {
      throw std::runtime_error("stheta log not found");
    }
    theta =
        incidentThetas->valuesAsVector().back(); // Not quite sure what to do
                                                 // with the time series for
                                                 // stheta
  } catch (Exception::NotFoundError &) {
    return theta;
  }
  return theta;
}

/** @todo The following translate functions are duplicates of code in 
* GroupDetectors2.cpp. We should move them to a common location if possible */

/* The following functions are used to translate single operators into
* groups, just like the ones this algorithm loads from .map files.
*
* Each function takes a string, such as "3+4", or "6:10" and then adds
* the resulting groups of spectra to outGroups.
*/

// An add operation, i.e. "3+4" -> [3+4]
void translateAdd(const std::string &instructions,
                  std::vector<std::vector<size_t>> &outGroups) {
  std::vector<std::string> spectra;
  boost::split(spectra, instructions, boost::is_any_of("+"));

  std::vector<size_t> outSpectra;
  for (auto spectrum : spectra) {
    // remove leading/trailing whitespace
    boost::trim(spectrum);
    // add this spectrum to the group we're about to add
    outSpectra.push_back(boost::lexical_cast<size_t>(spectrum));
  }
  outGroups.push_back(outSpectra);
}

// A range summation, i.e. "3-6" -> [3+4+5+6]
void translateSumRange(const std::string &instructions,
                       std::vector<std::vector<size_t>> &outGroups) {
  // add a group with the sum of the spectra in the range
  std::vector<std::string> spectra;
  boost::split(spectra, instructions, boost::is_any_of("-"));
  if (spectra.size() != 2)
    throw std::runtime_error("Malformed range (-) operation.");
  // fetch the start and stop spectra
  size_t first = boost::lexical_cast<size_t>(spectra[0]);
  size_t last = boost::lexical_cast<size_t>(spectra[1]);
  // swap if they're back to front
  if (first > last)
    std::swap(first, last);

  // add all the spectra in the range to the output group
  std::vector<size_t> outSpectra;
  for (size_t i = first; i <= last; ++i)
    outSpectra.push_back(i);
  if (!outSpectra.empty())
    outGroups.push_back(outSpectra);
}

// A range insertion, i.e. "3:6" -> [3,4,5,6]
void translateRange(const std::string &instructions,
                    std::vector<std::vector<size_t>> &outGroups) {
  // add a group per spectra
  std::vector<std::string> spectra;
  boost::split(spectra, instructions, boost::is_any_of(":"));
  if (spectra.size() != 2)
    throw std::runtime_error("Malformed range (:) operation.");
  // fetch the start and stop spectra
  size_t first = boost::lexical_cast<size_t>(spectra[0]);
  size_t last = boost::lexical_cast<size_t>(spectra[1]);
  // swap if they're back to front
  if (first > last)
    std::swap(first, last);

  // add all the spectra in the range to separate output groups
  for (size_t i = first; i <= last; ++i) {
    // create group of size 1 with the spectrum in it
    std::vector<size_t> newGroup(1, i);
    // and add it to output
    outGroups.push_back(newGroup);
  }
}

/**
* Translate the processing instructions into a vector of groups of indices
*
* @param instructions : Instructions to translate
* @return : A vector of groups, each group being a vector of its 0-based
* spectrum indices
*/
std::vector<std::vector<size_t>>
translateInstructions(const std::string &instructions) {
  std::vector<std::vector<size_t>> outGroups;

  try {
    // split into comma separated groups, each group potentially containing
    // an operation (+-:) that produces even more groups.
    std::vector<std::string> groups;
    boost::split(groups, instructions, boost::is_any_of(","));

    for (auto groupStr : groups) {
      // remove leading/trailing whitespace
      boost::trim(groupStr);

      // Look for the various operators in the string. If one is found then
      // do the necessary translation into groupings.
      if (groupStr.find('+') != std::string::npos) {
        // add a group with the given spectra
        translateAdd(groupStr, outGroups);
      }
      else if (groupStr.find('-') != std::string::npos) {
        translateSumRange(groupStr, outGroups);
      }
      else if (groupStr.find(':') != std::string::npos) {
        translateRange(groupStr, outGroups);
      }
      else if (!groupStr.empty()) {
        // contains no instructions, just add this spectrum as a new group
        // create group of size 1 with the spectrum in it
        std::vector<size_t> newGroup(1, boost::lexical_cast<size_t>(groupStr));
        // and add it to output
        outGroups.push_back(newGroup);
      }
    }
  } catch (boost::bad_lexical_cast &ex) {
    throw std::runtime_error("Invalid processing instructions: " + instructions);
  }

  return outGroups;
}
}

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

  initReductionProperties();

  // Theta0
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "Theta0", Mantid::EMPTY_DBL(), Direction::Input),
                  "Horizon angle in degrees");

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

  const auto reduction = validateReductionProperties();
  results.insert(reduction.begin(), reduction.end());

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
    m_sum = false;
  }

  findDetectorsOfInterest();

  if (summingInQ()) {
    // These values are only required for summation in Q
    findLambdaMinMax();
    findTwoThetaMinMax(m_detectors[0]);
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
      result = sumInQ(result, m_detectors[0]);
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
    // Crop to wavelength limits
    result = cropWavelength(result);
  }

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
  const std::string summationType = getProperty("SummationType");

  if (summationType == "SumInQ") {
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

  m_detectors = translateInstructions(instructions);
  if (m_detectors.size() == 0) {
    throw std::runtime_error("Invalid processing instructions");
  } else if (m_detectors.size() > 1 && summingInQ()) {
    throw std::runtime_error(
        "Only a single range is supported for summing in Q");
  }

  // Also set the reference detector index as the centre of the
  // region of interest
  /// @todo Get correct centre pixel
  m_twoThetaRDetectorIdx = 403;
  //          minDetectorIdx + (maxDetectorIdx - minDetectorIdx) / 2;

  // Log the results
  g_log.debug() << "twoThetaR spectrum index: " << twoThetaRDetectorIdx()
                << std::endl;
}

/**
* Find and cache the min/max twoTheta for the area of interest
*/
void ReflectometryReductionOne2::findTwoThetaMinMax(
    const std::vector<size_t> &detectors) {
  m_twoThetaMin = getDetectorTwoTheta(m_spectrumInfo, spectrumMin(detectors));
  m_twoThetaMax = getDetectorTwoTheta(m_spectrumInfo, spectrumMax(detectors));
  if (m_twoThetaMin > m_twoThetaMax) {
    std::swap(m_twoThetaMin, m_twoThetaMax);
  }
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
    Property *theta0Property = getProperty("Theta0");
    if (!theta0Property->isDefault()) {
      m_theta0 = getProperty("Theta0");
    } else {
      m_theta0 = getThetaFromLogs(m_runWS);
    }
  }

  g_log.debug() << "theta0: " << theta0() << std::endl;
}

/**
* Find and cache the (arbitrary) reference angle twoThetaR for use for summation
* in Q, and the detector pixel it corresponds to.
*
* @return : the angle twoThetaR
* @throws : if the angle could not be found
*/
void ReflectometryReductionOne2::findTwoThetaR() {
  m_twoThetaR = getDetectorTwoTheta(m_spectrumInfo, twoThetaRDetectorIdx());
  g_log.debug() << "twoThetaR: " << twoThetaR() << std::endl;
}

/**
* Get the minimum spectrum index in the area of interest
* @return : the spectrum index
*/
size_t
ReflectometryReductionOne2::spectrumMin(const std::vector<size_t> &detectors) {
  // The list of detector indices should be sorted so just return the first
  return detectors.front();
}

/**
* Get the maximum spectrum index in the area of interest
* @return : the spectrum index
*/
size_t
ReflectometryReductionOne2::spectrumMax(const std::vector<size_t> &detectors) {
  // The list of detector indices should be sorted so just return the last
  return detectors.back();
}

/**
* Construct an "empty" output workspace in virtual-lambda for summation in Q.
* The workspace will have the same x values as the input workspace but the y
* values will all be zero.
*
* @return : a 1D workspace where y values are all zero
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::constructIvsLamWS(
    MatrixWorkspace_sptr detectorWS, const std::vector<size_t> &detectors) {
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

  const double bLambdaMax = getLambdaRange(detectorWS, spectrumMax(detectors));
  const double bTwoThetaMin =
      getDetectorTwoThetaRange(m_spectrumInfo, spectrumMin(detectors));
  getProjectedLambdaRange(lambdaMax(), twoThetaMin(), bLambdaMax, bTwoThetaMin,
                          lambdaVMin, dummy);

  const double bLambdaMin = getLambdaRange(detectorWS, spectrumMin(detectors));
  const double bTwoThetaMax =
      getDetectorTwoThetaRange(m_spectrumInfo, spectrumMax(detectors));
  getProjectedLambdaRange(lambdaMin(), twoThetaMax(), bLambdaMin, bTwoThetaMax,
                          dummy, lambdaVMax);

  if (lambdaVMin > lambdaVMax) {
    std::swap(lambdaVMin, lambdaVMax);
  }

  // Use the same number of bins as the input
  const double binWidth = (lambdaVMax - lambdaVMin) / numBins;
  std::stringstream params;
  params << lambdaVMin << "," << binWidth << "," << lambdaVMax;

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
ReflectometryReductionOne2::sumInQ(MatrixWorkspace_sptr detectorWS,
                                   const std::vector<size_t> &detectors) {
  // Construct the output array in virtual lambda
  MatrixWorkspace_sptr IvsLam = constructIvsLamWS(detectorWS, detectors);

  // Loop through each spectrum in the region of interest
  for (size_t spIdx : detectors) {
    // Get the angle of this detector and its size in twoTheta
    double twoTheta = 0.0;
    double bTwoTheta = 0.0;
    getDetectorDetails(spIdx, twoTheta, bTwoTheta);

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
  const double bLambda = getLambdaRange(inputX, inputIdx);
  const double lambda = getLambda(inputX, inputIdx);
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
void ReflectometryReductionOne2::getDetectorDetails(const size_t spIdx,
                                                    double &twoTheta,
                                                    double &bTwoTheta) {
  twoTheta = getDetectorTwoTheta(m_spectrumInfo, spIdx);
  bTwoTheta = getDetectorTwoThetaRange(m_spectrumInfo, spIdx);
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
  // Get the angle from the horizon to the reference angle
  const double horizonThetaR = twoThetaR() - theta0();

  // Calculate the projected wavelength range
  try {
    const double lambdaTop = std::sin(horizonThetaR) * (lambda + bLambda / 2) /
                             std::sin(horizonThetaR + gamma - bTwoTheta / 2);
    const double lambdaBot = std::sin(horizonThetaR) * (lambda - bLambda / 2) /
                             std::sin(horizonThetaR + gamma + bTwoTheta / 2);

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

  // Map the original indices to the host workspace
  std::vector<std::vector<size_t>> hostGroups;
  for (auto group : m_detectors) {
    std::vector<size_t> hostDetectors;
    for (auto i : group) {
      const int hostIdx = mapSpectrumIndexToWorkspace(map, i, hostWS);
      hostDetectors.push_back(hostIdx);
    }
    hostGroups.push_back(hostDetectors);
  }

  // Add each group to the output, separated by ','
  /// @todo Add support to separate contiguous groups by ':' to avoid having
  /// long lists in the processing instructions
  for (auto &hostDetectors : hostGroups) {
    // Add each detector index to the output string separated by '+' to indicate
    // that all detectors in this group will be summed. We also check for
    // contiguous ranges so we output e.g. 3-5 instead of 3+4+5
    bool contiguous = false;
    size_t contiguousStart = 0;

    for (auto &it = hostDetectors.begin(); it != hostDetectors.end(); ++it) {
      // Check if the next iterator is a contiguous increment from this one
      auto &nextIt = it + 1;
      if (nextIt != hostDetectors.end() && *nextIt == *it + 1) {
        // If this is a start of a new contiguous region, remember the start
        // index
        if (!contiguous) {
          contiguousStart = *it;
          contiguous = true;
        }
        // Continue to find the end of the contiguous region
        continue;
      }

      if (contiguous) {
        // Output the contiguous range, then reset the flag
        result << contiguousStart << "-" << *it;
        contiguousStart = 0;
        contiguous = false;
      } else {
        // Just output the value
        result << *it;
      }

      // Add a separator ready for the next value/range
      if (nextIt != hostDetectors.end()) {
        result << "+";
      }
    }
  }

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
