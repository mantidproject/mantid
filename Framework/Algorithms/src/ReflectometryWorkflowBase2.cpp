// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {
int convertStringNumToInt(const std::string &string) {
  try {
    auto returnValue = std::stoi(string.c_str());
    return returnValue;
  } catch (std::invalid_argument &) {
    throw std::runtime_error("Invalid argument for processing instructions");
  } catch (std::out_of_range &) {
    throw std::runtime_error(
        "Out of range value given for processing instructions");
  }
}
} // namespace

namespace Mantid {
namespace Algorithms {

/** Initialize properties related to the type of reduction
 */
void ReflectometryWorkflowBase2::initReductionProperties() {
  // Summation type
  std::vector<std::string> summationTypes = {"SumInLambda", "SumInQ"};
  declareProperty("SummationType", "SumInLambda",
                  boost::make_shared<StringListValidator>(summationTypes),
                  "The type of summation to perform.", Direction::Input);

  // Reduction type
  std::vector<std::string> reductionTypes = {"Normal", "DivergentBeam",
                                             "NonFlatSample"};
  declareProperty("ReductionType", "Normal",
                  boost::make_shared<StringListValidator>(reductionTypes),
                  "The type of reduction to perform when summing in Q.",
                  Direction::Input);
  setPropertySettings("ReductionType",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SummationType", IS_EQUAL_TO, "SumInQ"));

  // Whether to crop out partial bins when projecting to virtual lambda for Q
  // summation
  declareProperty(std::make_unique<PropertyWithValue<bool>>(
                      "IncludePartialBins", false, Direction::Input),
                  "If true then partial bins at the beginning and end of the "
                  "output range are included");
  setPropertySettings("IncludePartialBins",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SummationType", IS_EQUAL_TO, "SumInQ"));
}

/** Initialize properties related to direct beam normalization
 */
void ReflectometryWorkflowBase2::initDirectBeamProperties() {

  declareProperty(std::make_unique<ArrayProperty<int>>("RegionOfDirectBeam"),
                  "Indices of the spectra a pair (lower, upper) that mark the "
                  "ranges that correspond to the direct beam in multi-detector "
                  "mode.");
}

/** Initialize properties related to monitors
 */
void ReflectometryWorkflowBase2::initMonitorProperties() {

  // Monitor workspace index
  declareProperty(std::make_unique<PropertyWithValue<int>>(
                      "I0MonitorIndex", Mantid::EMPTY_INT(), Direction::Input),
                  "I0 monitor workspace index");

  // Minimum wavelength for background subtraction
  declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
          Direction::Input),
      "Wavelength minimum for monitor background subtraction in angstroms.");
  // Maximum wavelength for background subtraction
  declareProperty(
      std::make_unique<PropertyWithValue<double>>(
          "MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
          Direction::Input),
      "Wavelength maximum for monitor background subtraction in angstroms.");

  // Minimum wavelength for monitor integration
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength minimum for integration in angstroms.");
  // Maximum wavelength for monitor integration
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength maximum for integration in angstroms.");
  // Normalization by integrated monitors
  declareProperty("NormalizeByIntegratedMonitors", true,
                  "Normalize by dividing by the integrated monitors.");
}

/** Initialize properties related to transmission normalization
 */
void ReflectometryWorkflowBase2::initTransmissionProperties() {

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "FirstTransmissionRun", "", Direction::Input, PropertyMode::Optional),
      "First transmission run, or the low wavelength transmission run if "
      "SecondTransmissionRun is also provided.");

  auto inputValidator = boost::make_shared<WorkspaceUnitValidator>("TOF");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional, inputValidator),
                  "Second, high wavelength transmission run. Optional. Causes "
                  "the FirstTransmissionRun to be treated as the low "
                  "wavelength transmission run.");

  initStitchProperties();

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>(
          "TransmissionProcessingInstructions", "", Direction::Input),
      "These processing instructions will be passed to the transmission "
      "workspace algorithm");

  setPropertyGroup("FirstTransmissionRun", "Transmission");
  setPropertyGroup("SecondTransmissionRun", "Transmission");
  setPropertyGroup("Params", "Transmission");
  setPropertyGroup("StartOverlap", "Transmission");
  setPropertyGroup("EndOverlap", "Transmission");
  setPropertyGroup("ScaleRHSWorkspace", "Transmission");
}

/** Initialize properties used for stitching transmission runs
 */
void ReflectometryWorkflowBase2::initStitchProperties() {

  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "These parameters are used for stitching together transmission runs. "
      "Values are in wavelength (angstroms). This input is only needed if a "
      "SecondTransmission run is provided.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start wavelength for stitching transmission runs together. "
                  "Only used if a second transmission run is provided.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "End wavelength (angstroms) for stitching transmission runs "
                  "together. Only used if a second transmission run is "
                  "provided.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>(
                      "ScaleRHSWorkspace", true, Direction::Input),
                  "Scale the right-hand-side or left-hand-side workspace. "
                  "Only used if a second transmission run is provided.");
}

/** Initialize algorithmic correction properties
 *
 * @param autoDetect :: True to include 'AutoDetect' option. False otherwise.
 */
void ReflectometryWorkflowBase2::initAlgorithmicProperties(bool autoDetect) {

  std::vector<std::string> correctionAlgorithms = {
      "None", "PolynomialCorrection", "ExponentialCorrection"};
  std::string defaultCorrection = "None";

  if (autoDetect) {
    correctionAlgorithms.insert(correctionAlgorithms.begin() + 1, "AutoDetect");
    defaultCorrection = "AutoDetect";
  }

  declareProperty("CorrectionAlgorithm", defaultCorrection,
                  boost::make_shared<StringListValidator>(correctionAlgorithms),
                  "The type of correction to perform.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Polynomial"),
                  "Coefficients to be passed to the PolynomialCorrection"
                  " algorithm.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("C0", 0.0, Direction::Input),
      "C0 value to be passed to the ExponentialCorrection algorithm.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("C1", 0.0, Direction::Input),
      "C1 value to be passed to the ExponentialCorrection algorithm.");

  setPropertyGroup("CorrectionAlgorithm", "Polynomial Corrections");
  setPropertyGroup("Polynomial", "Polynomial Corrections");
  setPropertyGroup("C0", "Polynomial Corrections");
  setPropertyGroup("C1", "Polynomial Corrections");
}

/** Initialize momentum transfer properties
 */
void ReflectometryWorkflowBase2::initMomentumTransferProperties() {

  declareProperty("MomentumTransferMin", Mantid::EMPTY_DBL(),
                  "Minimum Q value in IvsQ "
                  "Workspace. Used for Rebinning "
                  "the IvsQ Workspace",
                  Direction::Input);
  declareProperty("MomentumTransferStep", Mantid::EMPTY_DBL(),
                  "Resolution value in IvsQ Workspace. Used for Rebinning the "
                  "IvsQ Workspace. This value will be made minus to apply "
                  "logarithmic rebinning. If you wish to have linear "
                  "bin-widths then please provide a negative value.",
                  Direction::Input);
  declareProperty("MomentumTransferMax", Mantid::EMPTY_DBL(),
                  "Maximum Q value in IvsQ "
                  "Workspace. Used for Rebinning "
                  "the IvsQ Workspace",
                  Direction::Input);
  declareProperty("ScaleFactor", Mantid::EMPTY_DBL(),
                  "Factor you wish to scale Q workspace by.", Direction::Input);
}

/** Initialize properties for diagnostics
 */
void ReflectometryWorkflowBase2::initDebugProperties() {
  // Diagnostics
  declareProperty("Diagnostics", false,
                  "Whether to enable the output of "
                  "interim workspaces for debugging "
                  "purposes.");
  declareProperty("Debug", false,
                  "Whether to enable the output of extra workspaces.");
}

/** Validate reduction properties, if given
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string>
ReflectometryWorkflowBase2::validateReductionProperties() const {

  std::map<std::string, std::string> results;

  // If summing in Q, then reduction type must be given
  const std::string summationType = getProperty("SummationType");
  const std::string reductionType = getProperty("ReductionType");
  if (summationType == "SumInQ") {
    if (reductionType == "Normal") {
      results["ReductionType"] =
          "ReductionType must be set if SummationType is SumInQ";
    }
  } else {
    if (reductionType != "Normal") {
      results["ReductionType"] =
          "ReductionType should not be set unless SummationType is SumInQ";
    }
  }

  return results;
}

/** Validate direct beam if given
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string>
ReflectometryWorkflowBase2::validateDirectBeamProperties() const {

  std::map<std::string, std::string> results;

  // Validate direct beam if given
  Property *directBeamProperty = getProperty("RegionOfDirectBeam");
  if (!directBeamProperty->isDefault()) {

    const std::vector<int> directBeamRegion = getProperty("RegionOfDirectBeam");
    if (directBeamRegion.size() != 2) {
      results["RegionOfDirectBeam"] =
          "RegionOfDirect beam requires a lower and upper boundary";
    }
    if (directBeamRegion[0] > directBeamRegion[1]) {
      results["RegionOfDirectBeam"] =
          "First index must be less or equal than max index";
    }
  }

  return results;
}

/** Validate transmission runs if given
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string>
ReflectometryWorkflowBase2::validateTransmissionProperties() const {

  std::map<std::string, std::string> results;

  MatrixWorkspace_sptr firstTransmissionRun =
      getProperty("FirstTransmissionRun");
  if (firstTransmissionRun) {
    const auto xUnitFirst = firstTransmissionRun->getAxis(0)->unit()->unitID();
    if (xUnitFirst != "TOF" && xUnitFirst != "Wavelength") {
      results["FirstTransmissionRun"] =
          "First transmission run must be in TOF or wavelength";
    }
    MatrixWorkspace_sptr secondTransmissionRun =
        getProperty("SecondTransmissionRun");
    if (secondTransmissionRun) {
      const auto xUnitSecond =
          secondTransmissionRun->getAxis(0)->unit()->unitID();
      if (xUnitSecond != "TOF")
        results["SecondTransmissionRun"] =
            "Second transmission run must be in TOF";
      if (xUnitFirst != "TOF")
        results["FirstTransmissionRun"] = "When a second transmission run is "
                                          "given, first transmission run must "
                                          "be in TOF";
    }
  }

  return results;
}

/** Validate various wavelength ranges
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string>
ReflectometryWorkflowBase2::validateWavelengthRanges() const {

  std::map<std::string, std::string> results;

  // Validate wavelength range
  double wavMin = getProperty("WavelengthMin");
  double wavMax = getProperty("WavelengthMax");
  if (wavMin > wavMax)
    results["WavelengthMin"] =
        "WavelengthMax must be greater than WavelengthMin";

  // Validate monitor background range
  double monMin = getProperty("MonitorBackgroundWavelengthMin");
  double monMax = getProperty("MonitorBackgroundWavelengthMax");
  if (monMin > monMax)
    results["MonitorBackgroundWavelengthMin"] =
        "MonitorBackgroundWavelengthMax must be greater than "
        "MonitorBackgroundWavelengthMin";

  // Validate monitor integration range
  double monIntMin = getProperty("MonitorIntegrationWavelengthMin");
  double monIntMax = getProperty("MonitorIntegrationWavelengthMax");
  if (monIntMin > monIntMax)
    results["MonitorIntegrationWavelengthMax"] =
        "MonitorIntegrationWavelengthMax must be greater than "
        "MonitorIntegrationWavelengthMin";

  return results;
}

/** Converts an input workspace in TOF to wavelength
 * @param inputWS :: the workspace to convert
 * @return :: the workspace in wavelength
 */
MatrixWorkspace_sptr
ReflectometryWorkflowBase2::convertToWavelength(MatrixWorkspace_sptr inputWS) {

  auto convertUnitsAlg = createChildAlgorithm("ConvertUnits");
  convertUnitsAlg->initialize();
  convertUnitsAlg->setProperty("InputWorkspace", inputWS);
  convertUnitsAlg->setProperty("Target", "Wavelength");
  convertUnitsAlg->setProperty("AlignBins", true);
  convertUnitsAlg->execute();
  MatrixWorkspace_sptr outputWS =
      convertUnitsAlg->getProperty("OutputWorkspace");

  return outputWS;
}

/** Crops a workspace in wavelength to specified limits
 * @param inputWS :: the workspace to crop
 * @param useArgs :: if true, use the given args as the min and max;
 * otherwise, use the input properties to the algorithm
 * @param argMin :: the minimum wavelength to crop to if useArgs is true
 * @param argMax :: the maximum wavelength to crop to if useArgs is true
 * @return :: the cropped workspace
 */
MatrixWorkspace_sptr ReflectometryWorkflowBase2::cropWavelength(
    MatrixWorkspace_sptr inputWS, const bool useArgs, const double argMin,
    const double argMax) {

  double wavelengthMin = 0.0;
  double wavelengthMax = 0.0;

  if (useArgs) {
    // Use the given args
    wavelengthMin = argMin;
    wavelengthMax = argMax;
  } else {
    // Use the input properties to the algorithm
    wavelengthMin = getProperty("WavelengthMin");
    wavelengthMax = getProperty("WavelengthMax");
  }
  try {
    auto cropWorkspaceAlg = createChildAlgorithm("CropWorkspace");
    cropWorkspaceAlg->initialize();
    cropWorkspaceAlg->setProperty("InputWorkspace", inputWS);
    cropWorkspaceAlg->setProperty("XMin", wavelengthMin);
    cropWorkspaceAlg->setProperty("XMax", wavelengthMax);
    cropWorkspaceAlg->execute();
    MatrixWorkspace_sptr outputWS =
        cropWorkspaceAlg->getProperty("OutputWorkspace");

    return outputWS;
  } catch (std::out_of_range &e) {
    throw std::runtime_error("The processing instruction(s) are likely out of "
                             "bounds on the workspace, actual error: " +
                             std::string(e.what()));
  }
}

/** Process an input workspace in TOF according to specified processing commands
 * to get a detector workspace in wavelength.
 * @param inputWS :: the input workspace in TOF
 * @param convert :: whether the result should be converted to wavelength
 * @return :: the detector workspace in wavelength
 */
MatrixWorkspace_sptr
ReflectometryWorkflowBase2::makeDetectorWS(MatrixWorkspace_sptr inputWS,
                                           const bool convert) {
  auto groupAlg = createChildAlgorithm("GroupDetectors");
  groupAlg->initialize();
  groupAlg->setProperty("GroupingPattern",
                        m_processingInstructionsWorkspaceIndex);
  groupAlg->setProperty("InputWorkspace", inputWS);
  groupAlg->execute();
  MatrixWorkspace_sptr detectorWS = groupAlg->getProperty("OutputWorkspace");

  if (convert) {
    detectorWS = convertToWavelength(detectorWS);
  }

  return detectorWS;
}

/** Creates a monitor workspace in wavelength from an input workspace in TOF.
 * This method should only be called if IOMonitorIndex has been specified and
 * MonitorBackgroundWavelengthMin and MonitorBackgroundWavelengthMax have been
 * given.
 * @param inputWS :: the input workspace in TOF
 * @param integratedMonitors :: boolean to indicate if monitors should be
 * integrated
 * @return :: the monitor workspace in wavelength
 */
MatrixWorkspace_sptr
ReflectometryWorkflowBase2::makeMonitorWS(MatrixWorkspace_sptr inputWS,
                                          const bool integratedMonitors) {

  // Extract the monitor workspace
  const int monitorIndex = getProperty("I0MonitorIndex");
  auto cropWorkspaceAlg = createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", inputWS);
  cropWorkspaceAlg->setProperty("StartWorkspaceIndex", monitorIndex);
  cropWorkspaceAlg->setProperty("EndWorkspaceIndex", monitorIndex);
  cropWorkspaceAlg->execute();
  MatrixWorkspace_sptr monitorWS =
      cropWorkspaceAlg->getProperty("OutputWorkspace");

  monitorWS = convertToWavelength(monitorWS);

  // Flat background correction
  const double backgroundMin = getProperty("MonitorBackgroundWavelengthMin");
  const double backgroundMax = getProperty("MonitorBackgroundWavelengthMax");
  auto correctMonitorsAlg = createChildAlgorithm("CalculateFlatBackground");
  correctMonitorsAlg->initialize();
  correctMonitorsAlg->setProperty("InputWorkspace", monitorWS);
  correctMonitorsAlg->setProperty("StartX", backgroundMin);
  correctMonitorsAlg->setProperty("EndX", backgroundMax);
  correctMonitorsAlg->setProperty("SkipMonitors", false);
  correctMonitorsAlg->execute();
  monitorWS = correctMonitorsAlg->getProperty("OutputWorkspace");

  // Normalization by integrated monitors ?
  if (!integratedMonitors) {
    return monitorWS;
  }

  auto integrationAlg = createChildAlgorithm("Integration");
  integrationAlg->initialize();
  integrationAlg->setProperty("InputWorkspace", monitorWS);

  Property *integrationMinProperty =
      getProperty("MonitorIntegrationWavelengthMin");
  if (!integrationMinProperty->isDefault()) {
    integrationAlg->setProperty("RangeLower", integrationMinProperty->value());
  }

  Property *integrationMaxProperty =
      getProperty("MonitorIntegrationWavelengthMax");
  if (!integrationMaxProperty->isDefault()) {
    integrationAlg->setProperty("RangeUpper", integrationMaxProperty->value());
  }
  integrationAlg->execute();
  MatrixWorkspace_sptr integratedMonitor =
      integrationAlg->getProperty("OutputWorkspace");

  return integratedMonitor;
}

/** Rebin a detector workspace in wavelength to a given monitor workspace in
 * wavelength.
 * @param detectorWS :: the detector workspace in wavelength
 * @param monitorWS :: the monitor workspace in wavelength
 * @return :: the rebinned detector workspace
 */
MatrixWorkspace_sptr ReflectometryWorkflowBase2::rebinDetectorsToMonitors(
    MatrixWorkspace_sptr detectorWS, MatrixWorkspace_sptr monitorWS) {

  auto rebin = createChildAlgorithm("RebinToWorkspace");
  rebin->initialize();
  rebin->setProperty("WorkspaceToRebin", detectorWS);
  rebin->setProperty("WorkspaceToMatch", monitorWS);
  rebin->execute();
  MatrixWorkspace_sptr rebinnedWS = rebin->getProperty("OutputWorkspace");

  return rebinnedWS;
}

/** Set monitor properties
 *
 * @param alg :: ReflectometryReductionOne algorithm
 * @param instrument :: the instrument attached to the workspace
 */
void ReflectometryWorkflowBase2::populateMonitorProperties(
    IAlgorithm_sptr alg, Instrument_const_sptr instrument) {

  const auto startOverlap = checkForOptionalInstrumentDefault<double>(
      this, "StartOverlap", instrument, "TransRunStartOverlap");
  if (startOverlap.is_initialized())
    alg->setProperty("StartOverlap", startOverlap.get());

  const auto endOverlap = checkForOptionalInstrumentDefault<double>(
      this, "EndOverlap", instrument, "TransRunEndOverlap");
  if (endOverlap.is_initialized())
    alg->setProperty("EndOverlap", endOverlap.get());

  const auto monitorIndex = checkForOptionalInstrumentDefault<int>(
      this, "I0MonitorIndex", instrument, "I0MonitorIndex");
  if (monitorIndex.is_initialized())
    alg->setProperty("I0MonitorIndex", monitorIndex.get());

  const auto backgroundMin = checkForOptionalInstrumentDefault<double>(
      this, "MonitorBackgroundWavelengthMin", instrument,
      "MonitorBackgroundMin");
  if (backgroundMin.is_initialized())
    alg->setProperty("MonitorBackgroundWavelengthMin", backgroundMin.get());

  const auto backgroundMax = checkForOptionalInstrumentDefault<double>(
      this, "MonitorBackgroundWavelengthMax", instrument,
      "MonitorBackgroundMax");
  if (backgroundMax.is_initialized())
    alg->setProperty("MonitorBackgroundWavelengthMax", backgroundMax.get());

  const auto integrationMin = checkForOptionalInstrumentDefault<double>(
      this, "MonitorIntegrationWavelengthMin", instrument,
      "MonitorIntegralMin");
  if (integrationMin.is_initialized())
    alg->setProperty("MonitorIntegrationWavelengthMin", integrationMin.get());

  const auto integrationMax = checkForOptionalInstrumentDefault<double>(
      this, "MonitorIntegrationWavelengthMax", instrument,
      "MonitorIntegralMax");
  if (integrationMax.is_initialized())
    alg->setProperty("MonitorIntegrationWavelengthMax", integrationMax.get());

  const auto integrationBool = checkForOptionalInstrumentDefault<bool>(
      this, "NormalizeByIntegratedMonitors", instrument,
      "NormalizeByIntegratedMonitors");
  if (integrationBool.is_initialized())
    alg->setProperty("NormalizeByIntegratedMonitors", integrationBool.get());
}

/** Finding processing instructions from the parameters file
 * @param instrument :: the instrument attached to the workspace
 * @param inputWS :: the input workspace
 * @return :: processing instructions as a string
 */
std::string ReflectometryWorkflowBase2::findProcessingInstructions(
    Instrument_const_sptr instrument, MatrixWorkspace_sptr inputWS) const {

  const std::string analysisMode = getProperty("AnalysisMode");

  if (analysisMode == "PointDetectorAnalysis") {
    const std::vector<double> pointStart =
        instrument->getNumberParameter("PointDetectorStart");
    const std::vector<double> pointStop =
        instrument->getNumberParameter("PointDetectorStop");

    if (pointStart.empty() || pointStop.empty())
      throw std::runtime_error("Could not find 'PointDetectorStart' and/or "
                               "'PointDetectorStop' in parameter file. Please "
                               "provide processing instructions manually or "
                               "set analysis mode to 'MultiDetectorAnalysis'.");

    const int detStart = static_cast<int>(pointStart[0]);
    const int detStop = static_cast<int>(pointStop[0]);

    auto instructions = std::to_string(detStart);
    if (detStart != detStop)
      instructions += ":" + std::to_string(detStop);
    return instructions;
  }

  else {
    const std::vector<double> multiStart =
        instrument->getNumberParameter("MultiDetectorStart");
    if (multiStart.empty())
      throw std::runtime_error("Could not find 'MultiDetectorStart in "
                               "parameter file. Please provide processing "
                               "instructions manually or set analysis mode to "
                               "'PointDetectorAnalysis'.");

    auto instructions = std::to_string(static_cast<int>(multiStart[0])) + ":" +
                        std::to_string(inputWS->getNumberHistograms() - 1);
    return instructions;
  }
}

/** Set transmission properties
 *
 * @param alg :: The algorithm to populate parameters for
 * @return Boolean, whether or not any transmission runs were found
 */
bool ReflectometryWorkflowBase2::populateTransmissionProperties(
    IAlgorithm_sptr alg) const {

  bool transRunsExist = false;

  MatrixWorkspace_sptr firstWS = getProperty("FirstTransmissionRun");
  if (firstWS) {
    transRunsExist = true;
    alg->setProperty("FirstTransmissionRun", firstWS);
    MatrixWorkspace_sptr secondWS = getProperty("SecondTransmissionRun");
    if (secondWS) {
      alg->setProperty("SecondTransmissionRun", secondWS);
      alg->setPropertyValue("StartOverlap", getPropertyValue("StartOverlap"));
      alg->setPropertyValue("EndOverlap", getPropertyValue("EndOverlap"));
      alg->setPropertyValue("Params", getPropertyValue("Params"));
      alg->setProperty("ScaleRHSWorkspace",
                       getPropertyValue("ScaleRHSWorkspace"));
    }
  }

  return transRunsExist;
}

/**
 * Get the value of theta from a named log value
 *
 * @param inputWs :: the input workspace
 * @param logName :: the name of the log value to use
 * @return :: the value of theta found from the logs
 * @throw :: NotFoundError if the log value was not found
 */
double
ReflectometryWorkflowBase2::getThetaFromLogs(MatrixWorkspace_sptr inputWs,
                                             const std::string &logName) {
  double theta = -1.;
  const Mantid::API::Run &run = inputWs->run();
  Property *logData = run.getLogData(logName);
  auto logPWV = dynamic_cast<const PropertyWithValue<double> *>(logData);
  auto logTSP = dynamic_cast<const TimeSeriesProperty<double> *>(logData);

  if (logPWV) {
    theta = *logPWV;
  } else if (logTSP && logTSP->realSize() > 0) {
    theta = logTSP->lastValue();
  } else {
    throw Exception::NotFoundError("Theta", "Log value not found");
  }
  return theta;
}

/**
 * Retrieve the run number from the logs of the input workspace.
 *
 * @param ws :: A workspace to get the run number from.
 * @return :: A string containing the run number prefixed with the underscore
 *"_".
 *   if the workspace doesn't have the run number an empty string is returned.
 */
std::string
ReflectometryWorkflowBase2::getRunNumber(MatrixWorkspace const &ws) const {
  auto const &run = ws.run();
  if (run.hasProperty("run_number")) {
    return "_" + run.getPropertyValueAsType<std::string>("run_number");
  }
  return "";
}

std::string
ReflectometryWorkflowBase2::convertProcessingInstructionsToWorkspaceIndices(
    const std::string &instructions, MatrixWorkspace_const_sptr ws) const {
  std::string converted = "";
  std::string currentNumber = "";
  std::string ignoreThese = "-,:+";
  for (const char instruction : instructions) {
    if (std::find(ignoreThese.begin(), ignoreThese.end(), instruction) !=
        ignoreThese.end()) {
      // Found a spacer so add currentNumber to converted followed by separator
      converted.append(convertToWorkspaceIndex(currentNumber, ws));
      converted.push_back(instruction);
      currentNumber = "";
    } else {
      currentNumber.push_back(instruction);
    }
  }
  // Add currentNumber onto converted
  converted.append(convertToWorkspaceIndex(currentNumber, ws));
  return converted;
}

std::string ReflectometryWorkflowBase2::convertToWorkspaceIndex(
    const std::string &spectrumNumber, MatrixWorkspace_const_sptr ws) const {
  auto specNum = convertStringNumToInt(spectrumNumber);
  std::string wsIdx = std::to_string(
      ws->getIndexFromSpectrumNumber(static_cast<specnum_t>(specNum)));
  return wsIdx;
}

std::string
ReflectometryWorkflowBase2::convertProcessingInstructionsToSpectrumNumbers(
    const std::string &instructions,
    Mantid::API::MatrixWorkspace_const_sptr ws) const {
  std::string converted = "";
  std::string currentNumber = "";
  std::string ignoreThese = "-,:+";
  for (const char instruction : instructions) {
    if (std::find(ignoreThese.begin(), ignoreThese.end(), instruction) !=
        ignoreThese.end()) {
      // Found a spacer so add currentNumber to converted after seperator
      converted.append(convertToSpectrumNumber(currentNumber, ws));
      converted.push_back(instruction);
      currentNumber = "";
    } else {
      currentNumber.push_back(instruction);
    }
  }
  // Add currentNumber onto converted
  converted.append(convertToSpectrumNumber(currentNumber, ws));
  return converted;
}
std::string ReflectometryWorkflowBase2::convertToSpectrumNumber(
    const std::string &workspaceIndex,
    Mantid::API::MatrixWorkspace_const_sptr ws) const {
  auto wsIndx = convertStringNumToInt(workspaceIndex);
  std::string specId = std::to_string(
      static_cast<int32_t>(ws->indexInfo().spectrumNumber(wsIndx)));
  return specId;
}

void ReflectometryWorkflowBase2::convertProcessingInstructions(
    Instrument_const_sptr instrument, MatrixWorkspace_sptr inputWS) {
  m_processingInstructions = getPropertyValue("ProcessingInstructions");
  if (!getPointerToProperty("ProcessingInstructions")->isDefault()) {
    m_processingInstructionsWorkspaceIndex =
        convertProcessingInstructionsToWorkspaceIndices(
            m_processingInstructions, inputWS);
  } else {
    m_processingInstructionsWorkspaceIndex =
        findProcessingInstructions(instrument, inputWS);
    m_processingInstructions = convertProcessingInstructionsToSpectrumNumbers(
        m_processingInstructionsWorkspaceIndex, inputWS);
  }
}

void ReflectometryWorkflowBase2::convertProcessingInstructions(
    MatrixWorkspace_sptr inputWS) {
  m_processingInstructions = getPropertyValue("ProcessingInstructions");
  m_processingInstructionsWorkspaceIndex =
      convertProcessingInstructionsToWorkspaceIndices(m_processingInstructions,
                                                      inputWS);
}
} // namespace Algorithms
} // namespace Mantid
