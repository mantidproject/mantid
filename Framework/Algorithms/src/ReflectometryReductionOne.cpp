#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"
#include "MantidAlgorithms/ReflectometryReductionOne.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {

/*Anonomous namespace */
namespace {
/**
* Helper non-member function for translating all the workspace indexes in an
*origin workspace into workspace indexes
* of a host end-point workspace. This is done using spectrum numbers as the
*intermediate.
*
* This function will throw a runtime error if the specId are not found to exist
*on the host end-point workspace.
*
* @param originWS : Origin workspace, which provides the original workspace
*index to spectrum number mapping.
* @param hostWS : Workspace onto which the resulting workspace indexes will be
*hosted
* @return Remapped workspace indexes applicable for the host workspace. results
*as comma separated string.
*/
std::string createWorkspaceIndexListFromDetectorWorkspace(
    MatrixWorkspace_const_sptr originWS, MatrixWorkspace_const_sptr hostWS) {
  auto spectrumMap = originWS->getSpectrumToWorkspaceIndexMap();
  auto it = spectrumMap.begin();
  std::stringstream result;
  specnum_t specId = (*it).first;
  result << static_cast<int>(hostWS->getIndexFromSpectrumNumber(specId));
  ++it;
  for (; it != spectrumMap.end(); ++it) {
    specId = (*it).first;
    result << ","
           << static_cast<int>(hostWS->getIndexFromSpectrumNumber(specId));
  }
  return result.str();
}

const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
const std::string pointDetectorAnalysis = "PointDetectorAnalysis";
const std::string tofUnitId = "TOF";
const std::string wavelengthUnitId = "Wavelength";

/**
* Helper free function to get the ordered spectrum numbers from a workspace.
* @param ws
* @return
*/
std::vector<int> getSpectrumNumbers(MatrixWorkspace_sptr &ws) {
  auto specToWSIndexMap = ws->getSpectrumToWorkspaceIndexMap();
  std::vector<int> keys(specToWSIndexMap.size());
  size_t i = 0;
  for (auto it = specToWSIndexMap.begin(); it != specToWSIndexMap.end();
       ++it, ++i) {
    keys[i] = static_cast<int>(it->first);
  }
  std::sort(
      keys.begin(),
      keys.end()); // Sort the keys, as the order is not guaranteed in the map.

  return keys;
}

/**
* Helper free function to calculate MomentumTransfer from lambda and theta
* @param lambda : Value in wavelength
* @param theta  : Value in Degrees
* @return MomentumTransfer
* @
*/
double calculateQ(double lambda, double theta) {
  if (lambda == 0.0)
    throw std::runtime_error("Minimum/Maximum value of the IvsLambda Workspace "
                             "is 0. Cannot calculate Q");
  double thetaInRad = theta * M_PI / 180;
  return (4 * M_PI * sin(thetaInRad)) / lambda;
}
}
/* End of ananomous namespace */

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryReductionOne)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryReductionOne::name() const {
  return "ReflectometryReductionOne";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryReductionOne::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryReductionOne::category() const {
  return "Reflectometry";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void ReflectometryReductionOne::init() {

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Run to reduce.");

  std::vector<std::string> propOptions;
  propOptions.push_back(pointDetectorAnalysis);
  propOptions.push_back(multiDetectorAnalysis);

  declareProperty(
      "AnalysisMode", "PointDetectorAnalysis",
      boost::make_shared<StringListValidator>(propOptions),
      "The type of analysis to perform. Point detector or multi detector.");

  declareProperty(make_unique<ArrayProperty<int>>("RegionOfDirectBeam"),
                  "Indices of the spectra a pair (lower, upper) that mark the "
                  "ranges that correspond to the direct beam in multi-detector "
                  "mode.");

  this->initIndexInputs();
  this->initWavelengthInputs();

  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "DetectorComponentName", "", Direction::Input),
                  "Name of the detector component i.e. point-detector. If "
                  "these are not specified, the algorithm will attempt lookup "
                  "using a standard naming convention.");

  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "SampleComponentName", "", Direction::Input),
                  "Name of the sample component i.e. some-surface-holder. If "
                  "these are not specified, the algorithm will attempt lookup "
                  "using a standard naming convention.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output Workspace IvsQ.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceWavelength",
                                                   "", Direction::Output,
                                                   PropertyMode::Optional),
                  "Output Workspace IvsLam. Intermediate workspace.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ThetaIn", Mantid::EMPTY_DBL(), Direction::Input),
                  "Final theta value in degrees. Optional, this value will be "
                  "calculated internally and provided as ThetaOut if not "
                  "provided.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ThetaOut", Mantid::EMPTY_DBL(), Direction::Output),
                  "Calculated final theta in degrees.");

  declareProperty("NormalizeByIntegratedMonitors", true,
                  "Normalize by dividing by the integrated monitors.");

  declareProperty(make_unique<PropertyWithValue<bool>>(
                      "CorrectDetectorPositions", true, Direction::Input),
                  "Correct detector positions using ThetaIn (if given)");

  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "FirstTransmissionRun", "", Direction::Input, PropertyMode::Optional),
      "First transmission run, or the low wavelength transmission run if "
      "SecondTransmissionRun is also provided.");

  auto inputValidator = boost::make_shared<WorkspaceUnitValidator>(tofUnitId);
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional, inputValidator),
                  "Second, high wavelength transmission run. Optional. Causes "
                  "the FirstTransmissionRun to be treated as the low "
                  "wavelength transmission run.");

  this->initStitchingInputs();

  declareProperty(make_unique<PropertyWithValue<bool>>("StrictSpectrumChecking",
                                                       true, Direction::Input),
                  "Enforces spectrum number checking prior to normalization");

  std::vector<std::string> correctionAlgorithms = {
      "None", "PolynomialCorrection", "ExponentialCorrection"};
  declareProperty("CorrectionAlgorithm", "None",
                  boost::make_shared<StringListValidator>(correctionAlgorithms),
                  "The type of correction to perform.");

  declareProperty(make_unique<ArrayProperty<double>>("Polynomial"),
                  "Coefficients to be passed to the PolynomialCorrection"
                  " algorithm.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("C0", 0.0, Direction::Input),
      "C0 value to be passed to the ExponentialCorrection algorithm.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("C1", 0.0, Direction::Input),
      "C1 value to be passed to the ExponentialCorrection algorithm.");

  setPropertyGroup("CorrectionAlgorithm", "Polynomial Corrections");
  setPropertyGroup("Polynomial", "Polynomial Corrections");
  setPropertyGroup("C0", "Polynomial Corrections");
  setPropertyGroup("C1", "Polynomial Corrections");

  setPropertySettings(
      "Polynomial",
      Kernel::make_unique<Kernel::EnabledWhenProperty>(
          "CorrectionAlgorithm", IS_EQUAL_TO, "PolynomialCorrection"));
  setPropertySettings(
      "C0", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                "CorrectionAlgorithm", IS_EQUAL_TO, "ExponentialCorrection"));
  setPropertySettings(
      "C1", Kernel::make_unique<Kernel::EnabledWhenProperty>(
                "CorrectionAlgorithm", IS_EQUAL_TO, "ExponentialCorrection"));

  setPropertyGroup("FirstTransmissionRun", "Transmission");
  setPropertyGroup("SecondTransmissionRun", "Transmission");
  setPropertyGroup("Params", "Transmission");
  setPropertyGroup("StartOverlap", "Transmission");
  setPropertyGroup("EndOverlap", "Transmission");

  // Only ask for transmission parameters when a FirstTranmissionRun has been
  // provided
  setPropertySettings("SecondTransmissionRun",
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "FirstTransmissionRun", IS_NOT_DEFAULT));
  setPropertySettings("Params",
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "FirstTransmissionRun", IS_NOT_DEFAULT));
  setPropertySettings("StartOverlap",
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "FirstTransmissionRun", IS_NOT_DEFAULT));
  setPropertySettings("EndOverlap",
                      Kernel::make_unique<Kernel::EnabledWhenProperty>(
                          "FirstTransmissionRun", IS_NOT_DEFAULT));

  // Only use region of direct beam when in multi-detector analysis mode.
  setPropertySettings(
      "RegionOfDirectBeam",
      Kernel::make_unique<Kernel::EnabledWhenProperty>(
          "AnalysisMode", IS_EQUAL_TO, "MultiDetectorAnalysis"));
  declareProperty("ScaleFactor", Mantid::EMPTY_DBL(),
                  "Factor you wish to scale Q workspace by.", Direction::Input);
  declareProperty("MomentumTransferMinimum", Mantid::EMPTY_DBL(),
                  "Minimum Q value in IvsQ "
                  "Workspace. Used for Rebinning "
                  "the IvsQ Workspace",
                  Direction::Input);
  declareProperty("MomentumTransferStep", Mantid::EMPTY_DBL(),
                  "Resolution value in IvsQ Workspace. Used for Rebinning the "
                  "IvsQ Workspace. This value will be made minus to apply "
                  "logarithmic rebinning. If you wish to have linear "
                  "bin-widths then please provide a negative DQQ",
                  Direction::Input);
  declareProperty("MomentumTransferMaximum", Mantid::EMPTY_DBL(),
                  "Maximum Q value in IvsQ "
                  "Workspace. Used for Rebinning "
                  "the IvsQ Workspace",
                  Direction::Input);
}

/**
* Correct the position of the detectors based on the input theta value.
* @param toCorrect : Workspace to correct detector positions on.
* @param thetaInDeg : Theta in degrees to use in correction calculations.
* @param isPointDetector : True if using point detector analysis
* @return Copy with positions corrected.
*/
MatrixWorkspace_sptr
ReflectometryReductionOne::correctPosition(API::MatrixWorkspace_sptr &toCorrect,
                                           const double &thetaInDeg,
                                           const bool isPointDetector) {
  g_log.debug("Correcting position using theta.");

  auto correctPosAlg = this->createChildAlgorithm(
      "SpecularReflectionPositionCorrect", -1, -1, true, 1);
  correctPosAlg->initialize();
  correctPosAlg->setProperty("InputWorkspace", toCorrect);

  const std::string analysisMode = this->getProperty("AnalysisMode");
  correctPosAlg->setProperty("AnalysisMode", analysisMode);
  auto instrument = toCorrect->getInstrument();
  IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);
  correctPosAlg->setProperty("SampleComponentName", sample->getName());
  correctPosAlg->setProperty("TwoThetaIn", thetaInDeg * 2);

  if (isPointDetector) {
    IComponent_const_sptr detector =
        this->getDetectorComponent(instrument, isPointDetector);
    correctPosAlg->setProperty("DetectorComponentName", detector->getName());
  } else {
    auto specNumbers = getSpectrumNumbers(toCorrect);
    correctPosAlg->setProperty("SpectrumNumbersOfDetectors", specNumbers);
    for (auto specNumber : specNumbers) {
      std::stringstream buffer;
      buffer << "Writing out: " << specNumber;
      g_log.notice(buffer.str());
    }
  }
  correctPosAlg->execute();
  MatrixWorkspace_sptr corrected =
      correctPosAlg->getProperty("OutputWorkspace");

  return corrected;
}
/**
* @param toConvert : workspace used to get instrument components
* @param thetaOut : angle between sample and detectors (in Degrees)
* @return Theta : the value by which we rotate the source (in Degrees)
*/
double ReflectometryReductionOne::getAngleForSourceRotation(
    MatrixWorkspace_sptr toConvert, double thetaOut) {
  auto instrument = toConvert->getInstrument();
  auto instrumentUpVector = instrument->getReferenceFrame()->vecPointingUp();
  // check to see if calculated theta is the same as theta from instrument setup
  auto instrumentBeamDirection = instrument->getBeamDirection();
  double currentThetaInFromInstrument =
      instrumentUpVector.angle(instrumentBeamDirection) * (180 / M_PI) - 90;
  bool isInThetaEqualToOutTheta =
      std::abs(currentThetaInFromInstrument - thetaOut) <
      Mantid::Kernel::Tolerance;
  // the angle by which we rotate the source
  double rotationTheta = 0.0;
  if (!isInThetaEqualToOutTheta /*source needs rotation*/) {
    rotationTheta = thetaOut - currentThetaInFromInstrument;
  }
  return rotationTheta;
}

/**
* Convert an input workspace into an IvsQ workspace.
*
* @param toConvert : Workspace to convert
* @param bCorrectPosition : Flag to indicate that detector positions should be
*corrected based on the input theta values.
* @param thetaInDeg : Theta in Degrees. Used for correction.
* @param isPointDetector: Is point detector analysis
* @return
*/
Mantid::API::MatrixWorkspace_sptr ReflectometryReductionOne::toIvsQ(
    API::MatrixWorkspace_sptr &toConvert, const bool bCorrectPosition,
    OptionalDouble &thetaInDeg, const bool isPointDetector) {
  /*
  * Can either calculate a missing theta value for the purposes of reporting,
  * or correct positions based on a theta value,
  * but not both. The processing is effectively circular if both are applied.
  */
  if (!thetaInDeg.is_initialized()) {
    g_log.debug("Calculating final theta.");

    auto correctThetaAlg =
        this->createChildAlgorithm("SpecularReflectionCalculateTheta");
    correctThetaAlg->initialize();
    correctThetaAlg->setProperty("InputWorkspace", toConvert);
    const std::string analysisMode = this->getProperty("AnalysisMode");
    correctThetaAlg->setProperty("AnalysisMode", analysisMode);
    const std::string sampleComponentName =
        this->getProperty("SampleComponentName");
    correctThetaAlg->setProperty("SampleComponentName", sampleComponentName);
    if (isPointDetector) {
      const std::string detectorComponentName =
          this->getPropertyValue("DetectorComponentName");
      correctThetaAlg->setProperty("DetectorComponentName",
                                   detectorComponentName);
    } else {
      std::vector<int> spectrumNumbers = getSpectrumNumbers(toConvert);
      correctThetaAlg->setProperty("SpectrumNumbersOfDetectors",
                                   spectrumNumbers);
    }
    correctThetaAlg->execute();
    const double twoTheta = correctThetaAlg->getProperty("TwoTheta");

    thetaInDeg = twoTheta / 2;

  } else if (bCorrectPosition) {
    toConvert = correctPosition(toConvert, thetaInDeg.get(), isPointDetector);
  }

  // Rotate the source (needed before ConvertUnits)
  double rotationTheta = getAngleForSourceRotation(toConvert, thetaInDeg.get());
  if (rotationTheta != 0.0) {
    auto rotateSource = this->createChildAlgorithm("RotateSource");
    rotateSource->setChild(true);
    rotateSource->initialize();
    rotateSource->setProperty("Workspace", toConvert);
    rotateSource->setProperty("Angle", rotationTheta);
    rotateSource->execute();
  }

  // Always convert units.
  auto convertUnits = this->createChildAlgorithm("ConvertUnits");
  convertUnits->initialize();
  convertUnits->setProperty("InputWorkspace", toConvert);
  convertUnits->setProperty("Target", "MomentumTransfer");
  convertUnits->execute();
  MatrixWorkspace_sptr inQ = convertUnits->getProperty("OutputWorkspace");

  // Rotate the source back to its original position
  if (rotationTheta != 0.0) {
    // for IvsLam Workspace
    auto rotateSource = this->createChildAlgorithm("RotateSource");
    rotateSource->setChild(true);
    rotateSource->initialize();
    rotateSource->setProperty("Workspace", toConvert);
    rotateSource->setProperty("Angle", -rotationTheta);
    rotateSource->execute();
    // for IvsQ Workspace
    rotateSource->setProperty("Workspace", inQ);
    rotateSource->setProperty("Angle", -rotationTheta);
    rotateSource->execute();
  }

  return inQ;
}

/**
* Get the sample component. Use the name provided as a property as the basis
*for the lookup as a priority.
*
* Throws if the name is invalid.
* @param inst : Instrument to search through
* @return : The component : The component object found.
*/
Mantid::Geometry::IComponent_const_sptr
ReflectometryReductionOne::getSurfaceSampleComponent(
    Mantid::Geometry::Instrument_const_sptr inst) {
  std::string sampleComponent = "some-surface-holder";
  if (!isPropertyDefault("SampleComponentName")) {
    sampleComponent = this->getPropertyValue("SampleComponentName");
  }
  auto searchResult = inst->getComponentByName(sampleComponent);
  if (searchResult == nullptr) {
    throw std::invalid_argument(sampleComponent +
                                " does not exist. Check input properties.");
  }
  return searchResult;
}

/**
* Get the detector component. Use the name provided as a property as the basis
*for the lookup as a priority.
*
* Throws if the name is invalid.
* @param inst : Instrument to search through.
* @param isPointDetector : True if this is a point detector. Used to guess a
*name.
* @return The component : The component object found.
*/
boost::shared_ptr<const Mantid::Geometry::IComponent>
ReflectometryReductionOne::getDetectorComponent(
    Mantid::Geometry::Instrument_const_sptr inst, const bool isPointDetector) {
  std::string componentToCorrect =
      isPointDetector ? "point-detector" : "line-detector";
  if (!isPropertyDefault("DetectorComponentName")) {
    componentToCorrect = this->getPropertyValue("DetectorComponentName");
  }
  boost::shared_ptr<const IComponent> searchResult =
      inst->getComponentByName(componentToCorrect);
  if (searchResult == nullptr) {
    throw std::invalid_argument(componentToCorrect +
                                " does not exist. Check input properties.");
  }
  return searchResult;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void ReflectometryReductionOne::exec() {
  MatrixWorkspace_sptr runWS = getProperty("InputWorkspace");

  OptionalMatrixWorkspace_sptr firstTransmissionRun;
  OptionalMatrixWorkspace_sptr secondTransmissionRun;
  OptionalDouble stitchingStart;
  OptionalDouble stitchingDelta;
  OptionalDouble stitchingEnd;
  OptionalDouble stitchingStartOverlap;
  OptionalDouble stitchingEndOverlap;

  getTransmissionRunInfo(firstTransmissionRun, secondTransmissionRun,
                         stitchingStart, stitchingDelta, stitchingEnd,
                         stitchingStartOverlap, stitchingEndOverlap);

  OptionalDouble theta;
  if (!isPropertyDefault("ThetaIn")) {
    double temp = this->getProperty("ThetaIn");
    theta = temp;
  }
  const std::string strAnalysisMode = getProperty("AnalysisMode");
  const bool isPointDetector = (pointDetectorAnalysis == strAnalysisMode);
  const bool isMultiDetector = (multiDetectorAnalysis == strAnalysisMode);

  const MinMax wavelengthInterval =
      this->getMinMax("WavelengthMin", "WavelengthMax");

  const std::string processingCommands = getWorkspaceIndexList();

  OptionalWorkspaceIndexes directBeam;
  fetchOptionalLowerUpperPropertyValue("RegionOfDirectBeam", isPointDetector,
                                       directBeam);

  auto instrument = runWS->getInstrument();

  const OptionalInteger i0MonitorIndex = checkForOptionalInstrumentDefault<int>(
      this, "I0MonitorIndex", instrument, "I0MonitorIndex");

  const OptionalMinMax monitorBackgroundWavelengthInterval = getOptionalMinMax(
      this, "MonitorBackgroundWavelengthMin", "MonitorBackgroundWavelengthMax",
      instrument, "MonitorBackgroundWavelengthMin",
      "MonitorBackgroundWavelengthMax");
  const OptionalMinMax monitorIntegrationWavelengthInterval = getOptionalMinMax(
      this, "MonitorIntegrationWavelengthMin",
      "MonitorIntegrationWavelengthMax", instrument,
      "MonitorIntegrationWavelengthMin", "MonitorIntegrationWavelengthMax");

  const bool correctDetectorPositions = getProperty("CorrectDetectorPositions");

  MatrixWorkspace_sptr IvsLam; // Output workspace
  MatrixWorkspace_sptr IvsQ;   // Output workspace

  auto xUnitID = runWS->getAxis(0)->unit()->unitID();

  if (xUnitID == "Wavelength") {
    // If the input workspace is in lambda, we don't need to do any corrections,
    // just use it as is.
    g_log.information("Input workspace already in unit 'Wavelength'. Skipping "
                      "lambda conversions.");
    IvsLam = runWS;
  } else if (xUnitID == "TOF") {
    // If the input workspace is in TOF, do some corrections and generate IvsLam
    // from it.
    DetectorMonitorWorkspacePair inLam =
        toLam(runWS, processingCommands, i0MonitorIndex, wavelengthInterval,
              monitorBackgroundWavelengthInterval);
    auto detectorWS = inLam.get<0>();
    auto monitorWS = inLam.get<1>();

    if (isMultiDetector) {
      if (directBeam.is_initialized()) {
        // Sum over the direct beam.
        WorkspaceIndexList db = directBeam.get();
        std::stringstream buffer;
        buffer << db.front() << "-" << db.back();
        MatrixWorkspace_sptr regionOfDirectBeamWS =
            this->toLamDetector(buffer.str(), runWS, wavelengthInterval);

        // Rebin to the detector workspace
        auto rebinToWorkspaceAlg =
            this->createChildAlgorithm("RebinToWorkspace");
        rebinToWorkspaceAlg->initialize();
        rebinToWorkspaceAlg->setProperty("WorkspaceToRebin",
                                         regionOfDirectBeamWS);
        rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", detectorWS);
        rebinToWorkspaceAlg->execute();
        regionOfDirectBeamWS =
            rebinToWorkspaceAlg->getProperty("OutputWorkspace");

        // Normalize by the direct beam.
        detectorWS = divide(detectorWS, regionOfDirectBeamWS);
      }
    }

    const bool normalizeByIntMon = getProperty("NormalizeByIntegratedMonitors");
    if (normalizeByIntMon) {
      auto integrationAlg = this->createChildAlgorithm("Integration");
      integrationAlg->initialize();
      integrationAlg->setProperty("InputWorkspace", monitorWS);
      if (monitorIntegrationWavelengthInterval.is_initialized()) {
        integrationAlg->setProperty(
            "RangeLower", monitorIntegrationWavelengthInterval.get().get<0>());
        integrationAlg->setProperty(
            "RangeUpper", monitorIntegrationWavelengthInterval.get().get<1>());
      }
      integrationAlg->execute();
      MatrixWorkspace_sptr integratedMonitor =
          integrationAlg->getProperty("OutputWorkspace");

      IvsLam = divide(
          detectorWS,
          integratedMonitor); // Normalize by the integrated monitor counts.
    } else {
      IvsLam = divide(detectorWS, monitorWS);
    }
  } else {
    // Neither TOF or Lambda? Abort.
    throw std::invalid_argument(
        "InputWorkspace must have units of TOF or Wavelength");
  }

  if (firstTransmissionRun.is_initialized()) {
    // Perform transmission correction.
    IvsLam = this->transmissonCorrection(
        IvsLam, wavelengthInterval, monitorBackgroundWavelengthInterval,
        monitorIntegrationWavelengthInterval, i0MonitorIndex,
        firstTransmissionRun.get(), secondTransmissionRun, stitchingStart,
        stitchingDelta, stitchingEnd, stitchingStartOverlap,
        stitchingEndOverlap, processingCommands);
  } else if (getPropertyValue("CorrectionAlgorithm") != "None") {
    IvsLam = algorithmicCorrection(IvsLam);
  } else {
    g_log.warning("No transmission correction will be applied.");
  }

  IvsQ = this->toIvsQ(IvsLam, correctDetectorPositions, theta, isPointDetector);
  double momentumTransferMinimum = getProperty("MomentumTransferMinimum");
  double momentumTransferStep = getProperty("MomentumTransferStep");
  double momentumTransferMaximum = getProperty("MomentumTransferMaximum");
  MantidVec QParams;
  if (isDefault("MomentumTransferMinimum"))
    momentumTransferMinimum = calculateQ(IvsLam->x(0).back(), theta.get());
  if (isDefault("MomentumTransferMaximum"))
    momentumTransferMaximum = calculateQ(IvsLam->x(0).front(), theta.get());
  if (isDefault("MomentumTransferStep")) {
    // if the DQQ is not given for this run.
    // we will use CalculateResoltion to produce this value
    // for us.
    IAlgorithm_sptr calcResAlg = createChildAlgorithm("CalculateResolution");
    calcResAlg->setProperty("Workspace", runWS);
    calcResAlg->setProperty("TwoTheta", theta.get());
    calcResAlg->execute();
    if (!calcResAlg->isExecuted())
      throw std::runtime_error("CalculateResolution failed. Please manually "
                               "enter a value for MomentumTransferStep.");
    momentumTransferStep = calcResAlg->getProperty("Resolution");
  }
  if (momentumTransferMinimum > momentumTransferMaximum)
    throw std::invalid_argument("MomentumTransferMinimum must be less than "
                                "MomentumTransferMaximum. Please check your "
                                "inputs for these Properties.");
  QParams.push_back(momentumTransferMinimum);
  QParams.push_back(-momentumTransferStep);
  QParams.push_back(momentumTransferMaximum);
  IAlgorithm_sptr algRebin = this->createChildAlgorithm("Rebin");
  algRebin->initialize();
  algRebin->setProperty("InputWorkspace", IvsQ);
  algRebin->setProperty("OutputWorkspace", IvsQ);
  algRebin->setProperty("Params", QParams);
  algRebin->execute();
  if (!algRebin->isExecuted())
    throw std::runtime_error("Failed to run Rebin algorithm");
  IvsQ = algRebin->getProperty("OutputWorkspace");
  double scaleFactor = getProperty("ScaleFactor");
  if (!isDefault("ScaleFactor")) {
    IAlgorithm_sptr algScale = this->createChildAlgorithm("Scale");
    algScale->initialize();
    algScale->setProperty("InputWorkspace", IvsQ);
    algScale->setProperty("OutputWorkspace", IvsQ);
    algScale->setProperty("Factor", 1.0 / scaleFactor);
    algScale->execute();
    if (!algScale->isExecuted())
      throw std::runtime_error("Failed to run Scale algorithm");
    IvsQ = algScale->getProperty("OutputWorkspace");
  }
  setProperty("ThetaOut", theta.get());
  setProperty("OutputWorkspaceWavelength", IvsLam);
  setProperty("OutputWorkspace", IvsQ);
  // setting these values so the Interface can retrieve them from
  // ReflectometryReductionOneAuto.
  setProperty("MomentumTransferMinimum", momentumTransferMinimum);
  setProperty("MomentumTransferStep", momentumTransferStep);
  setProperty("MomentumTransferMaximum", momentumTransferMaximum);
}

/**
* Perform Transmission Corrections.
* @param IvsLam : Run workspace which is to be normalized by the results of the
* transmission corrections.
* @param wavelengthInterval : Wavelength interval for the run workspace.
* @param wavelengthMonitorBackgroundInterval : Wavelength interval for the
* monitor background
* @param wavelengthMonitorIntegrationInterval : Wavelength interval for the
* monitor integration
* @param i0MonitorIndex : Monitor index for the I0 monitor
* @param firstTransmissionRun : The first transmission run
* @param secondTransmissionRun : The second transmission run (optional)
* @param stitchingStart : Stitching start in wavelength (optional but dependent
* on secondTransmissionRun)
* @param stitchingDelta : Stitching delta in wavelength (optional but dependent
* on secondTransmissionRun)
* @param stitchingEnd : Stitching end in wavelength (optional but dependent on
* secondTransmissionRun)
* @param stitchingStartOverlap : Stitching start wavelength overlap (optional
* but dependent on secondTransmissionRun)
* @param stitchingEndOverlap : Stitching end wavelength overlap (optional but
* dependent on secondTransmissionRun)
* @param numeratorProcessingCommands: Processing commands used on detector
* workspace.
* @return Normalized run workspace by the transmission workspace, which have
* themselves been converted to Lam, normalized by monitors and possibly
* stitched together.
*/
MatrixWorkspace_sptr ReflectometryReductionOne::transmissonCorrection(
    MatrixWorkspace_sptr IvsLam, const MinMax &wavelengthInterval,
    const OptionalMinMax &wavelengthMonitorBackgroundInterval,
    const OptionalMinMax &wavelengthMonitorIntegrationInterval,
    const OptionalInteger &i0MonitorIndex,
    MatrixWorkspace_sptr firstTransmissionRun,
    OptionalMatrixWorkspace_sptr secondTransmissionRun,
    const OptionalDouble &stitchingStart, const OptionalDouble &stitchingDelta,
    const OptionalDouble &stitchingEnd,
    const OptionalDouble &stitchingStartOverlap,
    const OptionalDouble &stitchingEndOverlap,
    const std::string &numeratorProcessingCommands) {
  g_log.debug(
      "Extracting first transmission run workspace indexes from spectra");

  const bool strictSpectrumChecking = getProperty("StrictSpectrumChecking");

  MatrixWorkspace_sptr denominator = firstTransmissionRun;
  Unit_const_sptr xUnit = firstTransmissionRun->getAxis(0)->unit();
  if (xUnit->unitID() == tofUnitId) {
    std::string spectrumProcessingCommands = numeratorProcessingCommands;
    /*
    If we have strict spectrum checking, the processing commands need to be
    made from the
    numerator workspace AND the transmission workspace based on matching
    spectrum numbers.
    */
    if (strictSpectrumChecking) {
      spectrumProcessingCommands =
          createWorkspaceIndexListFromDetectorWorkspace(IvsLam,
                                                        firstTransmissionRun);
    }

    // Make the transmission run.
    auto alg = this->createChildAlgorithm("CreateTransmissionWorkspace", -1, -1,
                                          true, 1);
    alg->initialize();
    alg->setProperty("FirstTransmissionRun", firstTransmissionRun);
    if (secondTransmissionRun.is_initialized()) {
      alg->setProperty("SecondTransmissionRun", secondTransmissionRun.get());

      if (stitchingStart.is_initialized() && stitchingEnd.is_initialized() &&
          stitchingDelta.is_initialized()) {
        const std::vector<double> params = {
            stitchingStart.get(), stitchingDelta.get(), stitchingEnd.get()};
        alg->setProperty("Params", params);
      } else if (stitchingDelta.is_initialized()) {
        alg->setProperty("Params",
                         std::vector<double>(1, stitchingDelta.get()));
      }
      if (stitchingStartOverlap.is_initialized()) {
        alg->setProperty("StartOverlap", stitchingStartOverlap.get());
      }
      if (stitchingEndOverlap.is_initialized()) {
        alg->setProperty("EndOverlap", stitchingEndOverlap.get());
      }
    }
    alg->setProperty("ProcessingInstructions", spectrumProcessingCommands);
    if (i0MonitorIndex.is_initialized()) {
      alg->setProperty("I0MonitorIndex", i0MonitorIndex.get());
    }
    alg->setProperty("WavelengthMin", wavelengthInterval.get<0>());
    alg->setProperty("WavelengthMax", wavelengthInterval.get<1>());
    if (wavelengthMonitorBackgroundInterval.is_initialized()) {
      alg->setProperty("MonitorBackgroundWavelengthMin",
                       wavelengthMonitorBackgroundInterval.get().get<0>());
      alg->setProperty("MonitorBackgroundWavelengthMax",
                       wavelengthMonitorBackgroundInterval.get().get<1>());
    }
    if (wavelengthMonitorIntegrationInterval.is_initialized()) {
      alg->setProperty("MonitorIntegrationWavelengthMin",
                       wavelengthMonitorIntegrationInterval.get().get<0>());
      alg->setProperty("MonitorIntegrationWavelengthMax",
                       wavelengthMonitorIntegrationInterval.get().get<1>());
    }
    alg->execute();
    denominator = alg->getProperty("OutputWorkspace");
  }

  // Rebin the transmission run to be the same as the input.
  auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
  rebinToWorkspaceAlg->initialize();
  rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", IvsLam);
  rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", denominator);
  rebinToWorkspaceAlg->execute();
  denominator = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

  verifySpectrumMaps(IvsLam, denominator, strictSpectrumChecking);

  // Do normalization.
  MatrixWorkspace_sptr normalizedIvsLam = divide(IvsLam, denominator);
  return normalizedIvsLam;
}

/**
* Perform transmission correction using alternative correction algorithms.
* @param IvsLam : Run workspace which is to be normalized by the results of the
* transmission corrections.
* @return Corrected workspace
*/
MatrixWorkspace_sptr
ReflectometryReductionOne::algorithmicCorrection(MatrixWorkspace_sptr IvsLam) {

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

  corrAlg->setProperty("InputWorkspace", IvsLam);
  corrAlg->setProperty("Operation", "Divide");
  corrAlg->execute();

  return corrAlg->getProperty("OutputWorkspace");
}

/**
@param ws1 : First workspace to compare
@param ws2 : Second workspace to compare against
@param severe: True to indicate that failure to verify should result in an
exception. Otherwise a warning is generated.
*/
void ReflectometryReductionOne::verifySpectrumMaps(
    MatrixWorkspace_const_sptr ws1, MatrixWorkspace_const_sptr ws2,
    const bool severe) {
  auto map1 = ws1->getSpectrumToWorkspaceIndexMap();
  auto map2 = ws2->getSpectrumToWorkspaceIndexMap();
  if (map1 != map2) {
    std::string message = "Spectrum maps between workspaces do NOT match up.";
    if (severe) {
      throw std::invalid_argument(message);
    } else {
      this->g_log.warning(message);
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
