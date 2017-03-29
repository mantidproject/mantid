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

namespace Mantid {
namespace Algorithms {

/*Anonomous namespace */
namespace {

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
* @return :: Remapped workspace indexes applicable for the host workspace.
*results
*as comma separated string.
*/
std::string
createProcessingCommandsFromDetectorWS(MatrixWorkspace_const_sptr originWS,
                                       MatrixWorkspace_const_sptr hostWS,
                                       std::vector<size_t> detectors) {
  auto spectrumMap = originWS->getSpectrumToWorkspaceIndexMap();
  std::stringstream result;

  bool first = true;
  for (auto it = spectrumMap.begin(); it != spectrumMap.end(); ++it) {
    // Skip this spectrum if it is not in the detectors of interest
    if (std::find(detectors.begin(), detectors.end(), it->second) ==
        detectors.end()) {
      continue;
    }

    specnum_t specId = (*it).first;
    if (!first) {
      result << ",";
    }

    result << static_cast<int>(hostWS->getIndexFromSpectrumNumber(specId));
    first = false;
  }

  return result.str();
}
}

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

  if (summingInQ()) {
    // These values are only required for summation in Q
    findLambdaMinMax();
    findDetectorsOfInterest();
    findThetaMinMax();
    findTheta0();
    findThetaR();
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
* lambda" at a reference angle thetaR.
*
* @param inputWS :: the input workspace in TOF
* @return :: the output workspace in wavelength
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::makeIvsLam() {
  MatrixWorkspace_sptr IvsLam = m_runWS;

  if (summingInQ()) {
    // Get the normalised detector workspace
    MatrixWorkspace_sptr detectorWS = m_runWS;
    if (m_convertUnits) {
      // Convert the 2D workspace to wavelength
      detectorWS = convertToWavelength(detectorWS);
    }
    if (m_normalise) {
      detectorWS = monitorCorrection(detectorWS);
      detectorWS = transOrAlgCorrection(detectorWS, false);
    }

    // Do the summation in Q
    if (m_sum) {
      // Construct the output array in virtual lambda
      IvsLam = constructIvsLamWS(detectorWS);

      // Loop through each spectrum in the region of interest
      const auto &spectrumInfo = detectorWS->spectrumInfo();
      for (size_t spIdx : m_detectors) {
        // Get the angle of this detector and its size in twoTheta
        double theta = 0.0;
        double bTwoTheta = 0.0;
        getDetectorDetails(spIdx, spectrumInfo, theta, bTwoTheta);

        // Loop through each value in this spectrum
        const auto &inputX = detectorWS->x(spIdx);
        const auto &inputY = detectorWS->y(spIdx);
        if (inputX.size() != inputY.size() + 1) {
          throw std::runtime_error("Expected input data to be binned");
        }

        // Convenience points to the output array X and Y values
        const auto &outputX = IvsLam->x(0);
        auto &outputY = IvsLam->dataY(0);
        if (outputX.size() != outputY.size() + 1) {
          throw std::runtime_error("Expected output data to be binned");
        }

        for (int inputIdx = 0; inputIdx < inputY.size(); ++inputIdx) {
          // Get the bin width and the bin centre
          const double bLambda = inputX[inputIdx + 1] - inputX[inputIdx];
          const double lambda = inputX[inputIdx] + bLambda / 2.0;

          // Skip if outside area of interest
          if (lambda < lambdaMin() || lambda > lambdaMax()) {
            continue;
          }

          // Project these coordinates onto the virtual-lambda output (at
          // thetaR)
          double lambdaMin = 0.0;
          double lambdaMax = 0.0;
          getProjectedLambdaRange(lambda, theta, bLambda, bTwoTheta, lambdaMin,
                                  lambdaMax);

          // Share the counts from the detector into the output bins that
          // overlap
          // this range
          const double inputCounts = inputY[inputIdx];
          const double totalWidth = lambdaMax - lambdaMin;

          // Loop through all output bins that overlap the projected range
          for (int outputIdx = 0; outputIdx < outputY.size(); ++outputIdx) {
            const double binStart = outputX[outputIdx];
            const double binEnd = outputX[outputIdx + 1];
            if (binEnd < lambdaMin) {
              continue; // doesn't overlap
            } else if (binStart > lambdaMax) {
              break; // finished
            }
            // Add a share of the input counts to this bin based on the
            // proportion
            // of overlap.
            const double overlapWidth =
                std::min({bLambda, lambdaMax - binStart, binEnd - lambdaMin});
            outputY[outputIdx] += (inputCounts * overlapWidth) / (totalWidth);
          }
        }
      }
    }
  } else {
    // Simple summation in lambda
    if (m_sum) {
      // Get 1D workspace of detectors, summed, and converted to lambda if
      // applicable
      IvsLam = makeDetectorWS(IvsLam, m_convertUnits);
    }

    if (m_normalise) {
      IvsLam = directBeamCorrection(IvsLam);
      IvsLam = monitorCorrection(IvsLam);
      IvsLam = transOrAlgCorrection(IvsLam, true);
    }
  }

  // Crop to wavelength limits
  IvsLam = cropWavelength(IvsLam);

  return IvsLam;
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

      // Processing instructions for transmission workspace
      std::string transmissionCommands = getProperty("ProcessingInstructions");
      if (strictSpectrumChecking) {
        // If we have strict spectrum checking, the processing commands need to
        // be
        // made from the
        // numerator workspace AND the transmission workspace based on matching
        // spectrum numbers.
        transmissionCommands = createProcessingCommandsFromDetectorWS(
            detectorWS, transmissionWS, m_detectors);
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
      alg->setPropertyValue("I0MonitorIndex",
                            getPropertyValue("I0MonitorIndex"));
      alg->setPropertyValue("WavelengthMin", getPropertyValue("WavelengthMin"));
      alg->setPropertyValue("WavelengthMax", getPropertyValue("WavelengthMax"));
      alg->setPropertyValue("MonitorBackgroundWavelengthMin",
                            getPropertyValue("MonitorBackgroundWavelengthMin"));
      alg->setPropertyValue("MonitorBackgroundWavelengthMax",
                            getPropertyValue("MonitorBackgroundWavelengthMax"));
      alg->setPropertyValue(
          "MonitorIntegrationWavelengthMin",
          getPropertyValue("MonitorIntegrationWavelengthMin"));
      alg->setPropertyValue(
          "MonitorIntegrationWavelengthMax",
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
  // Get the min and max spectrum indicies from the processing instructions, if
  // given, or default to the overall min and max
  size_t minDetectorIdx = 0;
  size_t maxDetectorIdx = static_cast<int>(m_spectrumInfo->size()) - 1;

  std::string instructions = getPropertyValue("ProcessingInstructions");
  if (!instructions.empty() &&
      !std::all_of(instructions.begin(), instructions.end(), isspace)) {
    // The processing instructions should be in the format <start>-<end>.
    /// todo Add support for the '+' operator in the GroupingPattern. Could we
    /// also support ':' and ',' operators? These would result in more than one
    /// row in the detector workspace.
    std::vector<std::string> matches;
    boost::split(matches, instructions, boost::is_any_of("-"));

    if (matches.size() != 2) {
      std::ostringstream errMsg;
      errMsg << "Error reading processing instructions; expected a range in "
                "the format 'start-end' or 'start:end' but got "
             << instructions;
      throw std::runtime_error(errMsg.str());
    }

    try {
      boost::trim(matches[0]);
      boost::trim(matches[1]);
      minDetectorIdx = boost::lexical_cast<int>(matches[0]);
      maxDetectorIdx = boost::lexical_cast<int>(matches[1]);
    } catch (boost::bad_lexical_cast &ex) {
      std::ostringstream errMsg;
      errMsg << "Error reading processing instructions '" << instructions
             << "'; " << ex.what();
      throw std::runtime_error(errMsg.str());
    }
  }

  // Add each detector index in the range to the list
  for (size_t i = minDetectorIdx; i <= maxDetectorIdx; ++i) {
    m_detectors.push_back(i);
  }

  // Also set the centre detector index, which is the detector where thetaR is
  // defined. Use the centre of the region of interest.
  /// todo: check this - should it be the centre pixel of the detector
  /// (regardless of region of interest?)
  m_centreDetectorIdx = minDetectorIdx + (maxDetectorIdx - minDetectorIdx) / 2;
}

/**
* Find and cache the min/max theta for the area of interest
*/
void ReflectometryReductionOne2::findThetaMinMax() {

  m_thetaMin = std::min(m_spectrumInfo->twoTheta(m_detectors.front()),
                        m_spectrumInfo->twoTheta(m_detectors.back()));

  m_thetaMax = std::max(m_spectrumInfo->twoTheta(m_detectors.front()),
                        m_spectrumInfo->twoTheta(m_detectors.back()));
}

/**
* Find and cache the horizon angle theta0 for use for summation in Q.
*/
void ReflectometryReductionOne2::findTheta0() {
  const std::string reductionType = getProperty("ReductionType");

  m_theta0 = 0.0;

  if (reductionType == "DivergentBeam") {
    // theta0 is at the centre detector pixel
    /// todo: check this is true for all instruments
    m_theta0 = m_spectrumInfo->twoTheta(centreDetectorIdx()) / 2.0;
  }
}

/**
* Find and cache the (arbitrary) reference angle thetaR for use for summation in
* Q.
*
* @return : the angle thetaR
* @throws : if the angle could not be found
*/
void ReflectometryReductionOne2::findThetaR() {
  m_thetaR = thetaMin() + ((thetaMax() - thetaMin()) / 2.0);
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
  // Construct the new workspace from the centre pixel (where thetaR is defined)
  auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", detectorWS);
  cropWorkspaceAlg->setProperty("StartWorkspaceIndex", centreDetectorIdx());
  cropWorkspaceAlg->setProperty("EndWorkspaceIndex", centreDetectorIdx());
  cropWorkspaceAlg->execute();
  MatrixWorkspace_sptr ws = cropWorkspaceAlg->getProperty("OutputWorkspace");

  // Get the max and min values of the projected (virtual) lambda range
  double lambdaVMin = 0.0;
  double lambdaVMax = 0.0;
  double dummy = 0.0;
  const int numBins = static_cast<int>(detectorWS->blocksize());
  const double bLambda = (lambdaMax() - lambdaMin()) / numBins;
  const double bTwoTheta =
      (thetaMax() - thetaMin()) / detectorWS->getNumberHistograms();
  getProjectedLambdaRange(lambdaMax(), thetaMin(), bLambda, bTwoTheta,
                          lambdaVMin, dummy);
  getProjectedLambdaRange(lambdaMin(), thetaMax(), bLambda, bTwoTheta, dummy,
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
* Get the angle for this detector
*
* @param spIdx : the spectrum index
* @param spectrumInfo : the cached spectrum info
* @param theta [out] : the angle of the detector
* @param bTwoTheta [out] : the range twoTheta covered by this detector
*/
void ReflectometryReductionOne2::getDetectorDetails(
    const size_t spIdx, const SpectrumInfo &spectrumInfo, double &theta,
    double &bTwoTheta) {

  const double twoTheta = spectrumInfo.twoTheta(spIdx);
  theta = twoTheta / 2.0;

  // Get the range of covered by this pixel (assume it's the diff between this
  // pixel's theta and the prev/next pixel) / todo - check this
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
* @returns : the projected wavelength in virtual-lambda
*/
void ReflectometryReductionOne2::getProjectedLambdaRange(
    const double lambda, const double theta, const double bLambda,
    const double bTwoTheta, double &lambdaMin, double &lambdaMax) {
  // Convenince values for the calculation
  const double gamma = theta - thetaR();
  const double bTwoThetaOver2 = bTwoTheta;
  const double twoThetaR = thetaR() * 2.0;
  const double thetaR_theta0 = twoThetaR - theta0();

  // Calculate the projected wavelength range
  try {
    const double lambdaTop = std::sin(thetaR_theta0) * (lambda + bLambda / 2) /
                             std::sin(thetaR_theta0 + gamma - bTwoTheta / 2);
    const double lambdaBot = std::sin(thetaR_theta0) * (lambda - bLambda / 2) /
                             std::sin(thetaR_theta0 + gamma + bTwoTheta / 2);

    lambdaMin = std::min(lambdaTop, lambdaBot);
    lambdaMax = std::max(lambdaTop, lambdaBot);
  } catch (std::exception &ex) {
    std::stringstream errMsg;
    errMsg << "Failed to project (lambda, theta) = (" << lambda << "," << theta
           << ") onto thetaR = " << thetaR() << ": " << ex.what();
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
void ReflectometryReductionOne2::verifySpectrumMaps(MatrixWorkspace_const_sptr ws1,
                                                    MatrixWorkspace_const_sptr ws2,
                                                    const bool severe) {
  auto map1 = ws1->getSpectrumToWorkspaceIndexMap();
  auto map2 = ws2->getSpectrumToWorkspaceIndexMap();
  if (map1 != map2) {
    const std::string message =
      "Spectrum maps between workspaces do NOT match up.";
    if (severe) {
      throw std::invalid_argument(message);
    }
    else {
      g_log.warning(message);
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
