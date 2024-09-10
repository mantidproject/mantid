// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometryWorkflowBase2.h"

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
#include "MantidKernel/Strings.h"
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
    throw std::runtime_error("Out of range value given for processing instructions");
  }
}

std::string convertToWorkspaceIndex(const std::string &spectrumNumber, const MatrixWorkspace_const_sptr &ws) {
  auto specNum = convertStringNumToInt(spectrumNumber);
  std::string wsIdx = std::to_string(ws->getIndexFromSpectrumNumber(static_cast<Mantid::specnum_t>(specNum)));
  return wsIdx;
}

std::string convertProcessingInstructionsToWorkspaceIndices(const std::string &instructions,
                                                            const MatrixWorkspace_const_sptr &ws) {
  std::string converted = "";
  std::string currentNumber = "";
  std::string ignoreThese = "-,:+";
  for (const char instruction : instructions) {
    if (std::find(ignoreThese.begin(), ignoreThese.end(), instruction) != ignoreThese.end()) {
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

/** Convert processing instructions given as spectrum numbers to a vector of
 * workspace indices
 * @param instructions : the processing instructions in spectrum numbers in the
 * grouping format used by GroupDetectors
 * @param workspace : the workspace the instructions apply to
 * @returns : a vector of indices of the requested spectra in the given
 * workspace
 */
std::vector<size_t> getProcessingInstructionsAsIndices(std::string const &instructions,
                                                       const MatrixWorkspace_sptr &workspace) {
  auto instructionsInWSIndex = convertProcessingInstructionsToWorkspaceIndices(instructions, workspace);
  auto groups = Mantid::Kernel::Strings::parseGroups<size_t>(instructionsInWSIndex);
  auto indices = std::vector<size_t>();
  std::for_each(groups.cbegin(), groups.cend(), [&indices](std::vector<size_t> const &groupIndices) -> void {
    indices.insert(indices.begin(), groupIndices.cbegin(), groupIndices.cend());
  });
  return indices;
}

/** Get a detector parameter from an instrument
 *
 * @param instrument : the instrument containing the parameters
 * @param param : the parameter name
 * @return : the parameter value converted to an integer, or std::nullopt if it was not found
 * @throw : if the detector index is invalid
 */
std::optional<size_t> getDetectorParamOrNone(const Instrument_const_sptr &instrument,
                                             const MatrixWorkspace_sptr &inputWS, const std::string &param) {
  const std::vector<double> value = instrument->getNumberParameter(param);
  if (value.empty()) {
    return std::nullopt;
  }
  // Check it's a valid workspace index
  if (value[0] < 0) {
    std::ostringstream msg;
    msg << "Parameter file value " << param << "=" << value[0] << " is invalid; it must be 0 or greater";
    throw std::out_of_range(msg.str());
  }

  auto const wsIndex = static_cast<size_t>(value[0]);
  if (wsIndex >= inputWS->getNumberHistograms()) {
    std::ostringstream msg;
    msg << "Parameter file value " << param << "=" << wsIndex
        << " is out of range; max workspace index= " << inputWS->getNumberHistograms() - 1 << ")";
    throw std::out_of_range(msg.str());
  }
  return wsIndex;
}
} // namespace

namespace Mantid::Reflectometry {

/** Initialize the analysis properties
 */
void ReflectometryWorkflowBase2::initAnalysisProperties() {
  const std::vector<std::string> analysisMode{"PointDetectorAnalysis", "MultiDetectorAnalysis"};
  auto analysisModeValidator = std::make_shared<StringListValidator>(analysisMode);
  declareProperty("AnalysisMode", analysisMode[0], analysisModeValidator,
                  "Analysis mode. This property is only used when "
                  "ProcessingInstructions is not set.",
                  Direction::Input);
}

/** Initialize properties related to the type of reduction
 */
void ReflectometryWorkflowBase2::initReductionProperties() {
  // Summation type
  std::vector<std::string> summationTypes = {"SumInLambda", "SumInQ"};
  declareProperty("SummationType", "SumInLambda", std::make_shared<StringListValidator>(summationTypes),
                  "The type of summation to perform.", Direction::Input);

  // Reduction type
  std::vector<std::string> reductionTypes = {"Normal", "DivergentBeam", "NonFlatSample"};
  declareProperty("ReductionType", "Normal", std::make_shared<StringListValidator>(reductionTypes),
                  "The type of reduction to perform when summing in Q.", Direction::Input);
  setPropertySettings("ReductionType",
                      std::make_unique<Kernel::EnabledWhenProperty>("SummationType", IS_EQUAL_TO, "SumInQ"));

  // Whether to crop out partial bins when projecting to virtual lambda for Q
  // summation
  declareProperty(std::make_unique<PropertyWithValue<bool>>("IncludePartialBins", false, Direction::Input),
                  "If true then partial bins at the beginning and end of the "
                  "output range are included");
  setPropertySettings("IncludePartialBins",
                      std::make_unique<Kernel::EnabledWhenProperty>("SummationType", IS_EQUAL_TO, "SumInQ"));
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
  declareProperty(std::make_unique<PropertyWithValue<int>>("I0MonitorIndex", Mantid::EMPTY_INT(), Direction::Input),
                  "I0 monitor workspace index");

  // Minimum wavelength for background subtraction
  declareProperty(std::make_unique<PropertyWithValue<double>>("MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
                                                              Direction::Input),
                  "Wavelength minimum for monitor background subtraction in angstroms.");
  // Maximum wavelength for background subtraction
  declareProperty(std::make_unique<PropertyWithValue<double>>("MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
                                                              Direction::Input),
                  "Wavelength maximum for monitor background subtraction in angstroms.");

  // Minimum wavelength for monitor integration
  declareProperty(std::make_unique<PropertyWithValue<double>>("MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
                                                              Direction::Input),
                  "Wavelength minimum for integration in angstroms.");
  // Maximum wavelength for monitor integration
  declareProperty(std::make_unique<PropertyWithValue<double>>("MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
                                                              Direction::Input),
                  "Wavelength maximum for integration in angstroms.");
  // Normalization by integrated monitors
  declareProperty("NormalizeByIntegratedMonitors", true, "Normalize by dividing by the integrated monitors.");
}

/** Initialize properties related to transmission normalization
 */
void ReflectometryWorkflowBase2::initBackgroundProperties() {

  declareProperty(std::make_unique<PropertyWithValue<bool>>("SubtractBackground", false, Direction::Input),
                  "If true then perform background subtraction");
  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("BackgroundProcessingInstructions", "", Direction::Input),
      "These processing instructions will be passed to the background "
      "subtraction algorithm");

  auto algBkg = AlgorithmManager::Instance().createUnmanaged("ReflectometryBackgroundSubtraction");
  algBkg->initialize();
  copyProperty(algBkg, "BackgroundCalculationMethod");
  copyProperty(algBkg, "DegreeOfPolynomial");
  copyProperty(algBkg, "CostFunction");

  setPropertySettings("BackgroundProcessingInstructions",
                      std::make_unique<Kernel::EnabledWhenProperty>("SubtractBackground", IS_EQUAL_TO, "1"));
  setPropertySettings("BackgroundCalculationMethod",
                      std::make_unique<Kernel::EnabledWhenProperty>("SubtractBackground", IS_EQUAL_TO, "1"));

  setPropertyGroup("SubtractBackground", "Background");
  setPropertyGroup("BackgroundProcessingInstructions", "Background");
  setPropertyGroup("BackgroundCalculationMethod", "Background");
  setPropertyGroup("DegreeOfPolynomial", "Background");
  setPropertyGroup("CostFunction", "Background");
}

/** Initialize properties related to transmission normalization
 */
void ReflectometryWorkflowBase2::initTransmissionProperties() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("FirstTransmissionRun", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "First transmission run, or the low wavelength transmission run if "
                  "SecondTransmissionRun is also provided.");

  auto inputValidator = std::make_shared<WorkspaceUnitValidator>("TOF");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("SecondTransmissionRun", "", Direction::Input,
                                                                       PropertyMode::Optional, inputValidator),
                  "Second, high wavelength transmission run. Optional. Causes "
                  "the FirstTransmissionRun to be treated as the low "
                  "wavelength transmission run.");

  initStitchProperties();

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>("TransmissionProcessingInstructions", "", Direction::Input),
      "These processing instructions will be passed to the transmission "
      "workspace algorithm");

  setPropertyGroup("FirstTransmissionRun", "Transmission");
  setPropertyGroup("SecondTransmissionRun", "Transmission");
  setPropertyGroup("Params", "Transmission");
  setPropertyGroup("StartOverlap", "Transmission");
  setPropertyGroup("EndOverlap", "Transmission");
  setPropertyGroup("ScaleRHSWorkspace", "Transmission");
  setPropertyGroup("TransmissionProcessingInstructions", "Transmission");
}

/** Initialize output properties related to transmission normalization
 */
void ReflectometryWorkflowBase2::initTransmissionOutputProperties() {
  // Add additional output workspace properties
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspaceTransmission", "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output transmissison workspace in wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspaceFirstTransmission", "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "First transmissison workspace in wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspaceSecondTransmission", "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Second transmissison workspace in wavelength");

  // Specify conditional output properties for when debug is on
  setPropertySettings("OutputWorkspaceFirstTransmission",
                      std::make_unique<Kernel::EnabledWhenProperty>("Debug", IS_EQUAL_TO, "1"));
  setPropertySettings("OutputWorkspaceSecondTransmission",
                      std::make_unique<Kernel::EnabledWhenProperty>("Debug", IS_EQUAL_TO, "1"));
}

/** Initialize properties used for stitching transmission runs
 */
void ReflectometryWorkflowBase2::initStitchProperties() {

  declareProperty(std::make_unique<ArrayProperty<double>>("Params", std::make_shared<RebinParamsValidator>(true)),
                  "A comma separated list of first bin boundary, width, last bin boundary. "
                  "These parameters are used for stitching together transmission runs. "
                  "Values are in wavelength (angstroms). This input is only needed if a "
                  "SecondTransmission run is provided.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start wavelength for stitching transmission runs together. "
                  "Only used if a second transmission run is provided.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "End wavelength (angstroms) for stitching transmission runs "
                  "together. Only used if a second transmission run is "
                  "provided.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("ScaleRHSWorkspace", true, Direction::Input),
                  "Scale the right-hand-side or left-hand-side workspace. "
                  "Only used if a second transmission run is provided.");
}

/** Initialize algorithmic correction properties
 *
 * @param autoDetect :: True to include 'AutoDetect' option. False otherwise.
 */
void ReflectometryWorkflowBase2::initAlgorithmicProperties(bool autoDetect) {

  std::vector<std::string> correctionAlgorithms = {"None", "PolynomialCorrection", "ExponentialCorrection"};
  std::string defaultCorrection = "None";

  if (autoDetect) {
    correctionAlgorithms.insert(correctionAlgorithms.begin() + 1, "AutoDetect");
    defaultCorrection = "AutoDetect";
  }

  declareProperty("CorrectionAlgorithm", defaultCorrection, std::make_shared<StringListValidator>(correctionAlgorithms),
                  "The type of correction to perform.");

  declareProperty(std::make_unique<ArrayProperty<double>>("Polynomial"),
                  "Coefficients to be passed to the PolynomialCorrection"
                  " algorithm.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("C0", 0.0, Direction::Input),
                  "C0 value to be passed to the ExponentialCorrection algorithm.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("C1", 0.0, Direction::Input),
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
  declareProperty("ScaleFactor", Mantid::EMPTY_DBL(), "Factor you wish to scale Q workspace by.", Direction::Input);
}

/** Initialize properties for diagnostics
 */
void ReflectometryWorkflowBase2::initDebugProperties() {
  declareProperty("Debug", false, "Whether to enable the output of extra workspaces.");
  declareProperty("Diagnostics", false,
                  "Whether to enable the output of "
                  "interim workspaces for debugging "
                  "purposes.");
}

/** Validate background properties, if given
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string> ReflectometryWorkflowBase2::validateBackgroundProperties() const {

  std::map<std::string, std::string> results;
  return results;
}

/** Validate reduction properties, if given
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string> ReflectometryWorkflowBase2::validateReductionProperties() const {

  std::map<std::string, std::string> results;

  // If summing in Q, then reduction type must be given
  const std::string summationType = getProperty("SummationType");
  const std::string reductionType = getProperty("ReductionType");
  if (summationType == "SumInQ") {
    if (reductionType == "Normal") {
      results["ReductionType"] = "ReductionType must be set if SummationType is SumInQ";
    }
  } else {
    if (reductionType != "Normal") {
      results["ReductionType"] = "ReductionType should not be set unless SummationType is SumInQ";
    }
  }

  return results;
}

/** Validate direct beam if given
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string> ReflectometryWorkflowBase2::validateDirectBeamProperties() const {

  std::map<std::string, std::string> results;

  // Validate direct beam if given
  Property *directBeamProperty = getProperty("RegionOfDirectBeam");
  if (!directBeamProperty->isDefault()) {

    const std::vector<int> directBeamRegion = getProperty("RegionOfDirectBeam");
    if (directBeamRegion.size() != 2) {
      results["RegionOfDirectBeam"] = "RegionOfDirect beam requires a lower and upper boundary";
    }
    if (directBeamRegion[0] > directBeamRegion[1]) {
      results["RegionOfDirectBeam"] = "First index must be less or equal than max index";
    }
  }

  return results;
}

/** Validate transmission runs if given
 *
 * @return :: A map with results of validation
 */
std::map<std::string, std::string> ReflectometryWorkflowBase2::validateTransmissionProperties() const {

  std::map<std::string, std::string> results;

  MatrixWorkspace_sptr firstTransmissionRun = getProperty("FirstTransmissionRun");
  if (firstTransmissionRun) {
    const auto xUnitFirst = firstTransmissionRun->getAxis(0)->unit()->unitID();
    if (xUnitFirst != "TOF" && xUnitFirst != "Wavelength") {
      results["FirstTransmissionRun"] = "First transmission run must be in TOF or wavelength";
    }
    MatrixWorkspace_sptr secondTransmissionRun = getProperty("SecondTransmissionRun");
    if (secondTransmissionRun) {
      const auto xUnitSecond = secondTransmissionRun->getAxis(0)->unit()->unitID();
      if (xUnitSecond != "TOF")
        results["SecondTransmissionRun"] = "Second transmission run must be in TOF";
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
std::map<std::string, std::string> ReflectometryWorkflowBase2::validateWavelengthRanges() const {

  std::map<std::string, std::string> results;

  // Validate wavelength range
  double wavMin = getProperty("WavelengthMin");
  double wavMax = getProperty("WavelengthMax");
  if (wavMin > wavMax)
    results["WavelengthMin"] = "WavelengthMax must be greater than WavelengthMin";

  // Validate monitor background range
  double monMin = getProperty("MonitorBackgroundWavelengthMin");
  double monMax = getProperty("MonitorBackgroundWavelengthMax");
  if (monMin > monMax)
    results["MonitorBackgroundWavelengthMin"] = "MonitorBackgroundWavelengthMax must be greater than "
                                                "MonitorBackgroundWavelengthMin";

  // Validate monitor integration range
  double monIntMin = getProperty("MonitorIntegrationWavelengthMin");
  double monIntMax = getProperty("MonitorIntegrationWavelengthMax");
  if (monIntMin > monIntMax)
    results["MonitorIntegrationWavelengthMax"] = "MonitorIntegrationWavelengthMax must be greater than "
                                                 "MonitorIntegrationWavelengthMin";

  return results;
}

/** Converts an input workspace in TOF to wavelength
 * @param inputWS :: the workspace to convert
 * @return :: the workspace in wavelength
 */
MatrixWorkspace_sptr ReflectometryWorkflowBase2::convertToWavelength(const MatrixWorkspace_sptr &inputWS) {

  auto convertUnitsAlg = createChildAlgorithm("ConvertUnits");
  convertUnitsAlg->initialize();
  convertUnitsAlg->setProperty("InputWorkspace", inputWS);
  convertUnitsAlg->setProperty("Target", "Wavelength");
  convertUnitsAlg->setProperty("AlignBins", true);
  convertUnitsAlg->execute();
  MatrixWorkspace_sptr outputWS = convertUnitsAlg->getProperty("OutputWorkspace");

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
MatrixWorkspace_sptr ReflectometryWorkflowBase2::cropWavelength(const MatrixWorkspace_sptr &inputWS, const bool useArgs,
                                                                const double argMin, const double argMax) {

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
    MatrixWorkspace_sptr outputWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

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
 * @param sum :: whether the detectors should be summed into a single spectrum
 * @return :: the detector workspace in wavelength
 */
MatrixWorkspace_sptr ReflectometryWorkflowBase2::makeDetectorWS(MatrixWorkspace_sptr inputWS, const bool convert,
                                                                const bool sum) {
  auto detectorWS = std::move(inputWS);

  if (sum) {
    // Use GroupDetectors to extract and sum the detectors of interest
    auto groupAlg = createChildAlgorithm("GroupDetectors");
    groupAlg->initialize();
    groupAlg->setProperty("GroupingPattern", m_processingInstructionsWorkspaceIndex);
    groupAlg->setProperty("InputWorkspace", detectorWS);
    groupAlg->execute();
    detectorWS = groupAlg->getProperty("OutputWorkspace");
  } else if (!isDefault("BackgroundProcessingInstructions")) {
    // Extract the detectors for the ROI and background. Note that if background
    // instructions are not set then we require the whole workspace so there is
    // nothing to do.
    auto indices = getProcessingInstructionsAsIndices(m_processingInstructions, detectorWS);
    auto bkgIndices =
        getProcessingInstructionsAsIndices(getPropertyValue("BackgroundProcessingInstructions"), detectorWS);
    indices.insert(indices.end(), bkgIndices.cbegin(), bkgIndices.cend());
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    auto extractAlg = createChildAlgorithm("ExtractSpectra");
    extractAlg->initialize();
    extractAlg->setProperty("InputWorkspace", detectorWS);
    extractAlg->setProperty("WorkspaceIndexList", indices);
    extractAlg->execute();
    detectorWS = extractAlg->getProperty("OutputWorkspace");
    // Update the workspace indicies to match the new workspace
    m_processingInstructionsWorkspaceIndex =
        convertProcessingInstructionsToWorkspaceIndices(m_processingInstructions, detectorWS);
  }

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
MatrixWorkspace_sptr ReflectometryWorkflowBase2::makeMonitorWS(const MatrixWorkspace_sptr &inputWS,
                                                               const bool integratedMonitors) {

  // Extract the monitor workspace
  const int monitorIndex = getProperty("I0MonitorIndex");
  auto cropWorkspaceAlg = createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", inputWS);
  cropWorkspaceAlg->setProperty("StartWorkspaceIndex", monitorIndex);
  cropWorkspaceAlg->setProperty("EndWorkspaceIndex", monitorIndex);
  cropWorkspaceAlg->execute();
  MatrixWorkspace_sptr monitorWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

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

  Property *integrationMinProperty = getProperty("MonitorIntegrationWavelengthMin");
  if (!integrationMinProperty->isDefault()) {
    integrationAlg->setProperty("RangeLower", integrationMinProperty->value());
  }

  Property *integrationMaxProperty = getProperty("MonitorIntegrationWavelengthMax");
  if (!integrationMaxProperty->isDefault()) {
    integrationAlg->setProperty("RangeUpper", integrationMaxProperty->value());
  }
  integrationAlg->execute();
  MatrixWorkspace_sptr integratedMonitor = integrationAlg->getProperty("OutputWorkspace");

  return integratedMonitor;
}

/** Rebin a detector workspace in wavelength to a given monitor workspace in
 * wavelength.
 * @param detectorWS :: the detector workspace in wavelength
 * @param monitorWS :: the monitor workspace in wavelength
 * @return :: the rebinned detector workspace
 */
MatrixWorkspace_sptr ReflectometryWorkflowBase2::rebinDetectorsToMonitors(const MatrixWorkspace_sptr &detectorWS,
                                                                          const MatrixWorkspace_sptr &monitorWS) {

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
void ReflectometryWorkflowBase2::populateMonitorProperties(const IAlgorithm_sptr &alg,
                                                           const Instrument_const_sptr &instrument) {

  const auto startOverlap =
      checkForOptionalInstrumentDefault<double>(this, "StartOverlap", instrument, "TransRunStartOverlap");
  if (startOverlap.has_value())
    alg->setProperty("StartOverlap", startOverlap.value());

  const auto endOverlap =
      checkForOptionalInstrumentDefault<double>(this, "EndOverlap", instrument, "TransRunEndOverlap");
  if (endOverlap.has_value())
    alg->setProperty("EndOverlap", endOverlap.value());

  const auto monitorIndex =
      checkForOptionalInstrumentDefault<int>(this, "I0MonitorIndex", instrument, "I0MonitorIndex");
  if (monitorIndex.has_value())
    alg->setProperty("I0MonitorIndex", monitorIndex.value());

  const auto backgroundMin = checkForOptionalInstrumentDefault<double>(this, "MonitorBackgroundWavelengthMin",
                                                                       instrument, "MonitorBackgroundMin");
  if (backgroundMin.has_value())
    alg->setProperty("MonitorBackgroundWavelengthMin", backgroundMin.value());

  const auto backgroundMax = checkForOptionalInstrumentDefault<double>(this, "MonitorBackgroundWavelengthMax",
                                                                       instrument, "MonitorBackgroundMax");
  if (backgroundMax.has_value())
    alg->setProperty("MonitorBackgroundWavelengthMax", backgroundMax.value());

  const auto integrationMin = checkForOptionalInstrumentDefault<double>(this, "MonitorIntegrationWavelengthMin",
                                                                        instrument, "MonitorIntegralMin");
  if (integrationMin.has_value())
    alg->setProperty("MonitorIntegrationWavelengthMin", integrationMin.value());

  const auto integrationMax = checkForOptionalInstrumentDefault<double>(this, "MonitorIntegrationWavelengthMax",
                                                                        instrument, "MonitorIntegralMax");
  if (integrationMax.has_value())
    alg->setProperty("MonitorIntegrationWavelengthMax", integrationMax.value());

  const auto integrationBool = checkForOptionalInstrumentDefault<bool>(this, "NormalizeByIntegratedMonitors",
                                                                       instrument, "NormalizeByIntegratedMonitors");
  if (integrationBool.has_value())
    alg->setProperty("NormalizeByIntegratedMonitors", integrationBool.value());
}

/** Finding processing instructions from the parameters file
 * @param instrument :: the instrument attached to the workspace
 * @param inputWS :: the input workspace
 * @return :: processing instructions as a string
 */
std::string ReflectometryWorkflowBase2::findProcessingInstructions(const Instrument_const_sptr &instrument,
                                                                   const MatrixWorkspace_sptr &inputWS) const {
  assert(instrument && inputWS && inputWS->getNumberHistograms() > 0);
  const std::string analysisMode = getProperty("AnalysisMode");

  std::optional<size_t> maybeStart;
  std::optional<size_t> maybeStop;
  if (analysisMode == "PointDetectorAnalysis") {
    maybeStart = getDetectorParamOrNone(instrument, inputWS, "PointDetectorStart");
    maybeStop = getDetectorParamOrNone(instrument, inputWS, "PointDetectorStop");
    if (!maybeStart || !maybeStop) {
      throw std::runtime_error(
          "Could not find 'PointDetectorStart' and/or 'PointDetectorStop' in parameter file. Please provide processing "
          "instructions manually or set analysis mode to 'MultiDetectorAnalysis'.");
    }
  } else {
    maybeStart = getDetectorParamOrNone(instrument, inputWS, "MultiDetectorStart");
    maybeStop = getDetectorParamOrNone(instrument, inputWS, "MultiDetectorStop");
    if (!maybeStart) {
      throw std::runtime_error("Could not find 'MultiDetectorStart' in parameter file. Please provide processing "
                               "instructions manually or set analysis mode to 'PointDetectorAnalysis'.");
    }
    if (!maybeStop) {
      // Default to the last workspace index if stop is not given
      maybeStop = inputWS->getNumberHistograms() - 1;
    }
  }
  // We always have start; stop is optional
  auto instructions = std::to_string(*maybeStart);
  if (maybeStop && *maybeStart != *maybeStop) {
    instructions += "-" + std::to_string(*maybeStop);
  }
  return instructions;
}

/** Set transmission properties
 *
 * @param alg :: The algorithm to populate parameters for
 * @return Boolean, whether or not any transmission runs were found
 */
bool ReflectometryWorkflowBase2::populateTransmissionProperties(const IAlgorithm_sptr &alg) const {

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
      alg->setProperty("ScaleRHSWorkspace", getPropertyValue("ScaleRHSWorkspace"));
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
double ReflectometryWorkflowBase2::getThetaFromLogs(const MatrixWorkspace_sptr &inputWs,
                                                    const std::string &logName) const {
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
std::string ReflectometryWorkflowBase2::getRunNumber(MatrixWorkspace const &ws) const {
  auto const &run = ws.run();
  if (run.hasProperty("run_number")) {
    return "_" + run.getPropertyValueAsType<std::string>("run_number");
  }
  return "";
}

std::string ReflectometryWorkflowBase2::convertProcessingInstructionsToSpectrumNumbers(
    const std::string &instructions, const Mantid::API::MatrixWorkspace_const_sptr &ws) const {
  std::string converted = "";
  std::string currentNumber = "";
  std::string ignoreThese = "-,:+";
  for (const char instruction : instructions) {
    if (std::find(ignoreThese.begin(), ignoreThese.end(), instruction) != ignoreThese.end()) {
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
std::string
ReflectometryWorkflowBase2::convertToSpectrumNumber(const std::string &workspaceIndex,
                                                    const Mantid::API::MatrixWorkspace_const_sptr &ws) const {
  auto wsIndx = convertStringNumToInt(workspaceIndex);
  std::string specId = std::to_string(static_cast<int32_t>(ws->indexInfo().spectrumNumber(wsIndx)));
  return specId;
}

void ReflectometryWorkflowBase2::convertProcessingInstructions(const Instrument_const_sptr &instrument,
                                                               const MatrixWorkspace_sptr &inputWS) {
  m_processingInstructions = getPropertyValue("ProcessingInstructions");
  if (!getPointerToProperty("ProcessingInstructions")->isDefault()) {
    m_processingInstructionsWorkspaceIndex =
        convertProcessingInstructionsToWorkspaceIndices(m_processingInstructions, inputWS);
  } else {
    m_processingInstructionsWorkspaceIndex = findProcessingInstructions(instrument, inputWS);
    m_processingInstructions =
        convertProcessingInstructionsToSpectrumNumbers(m_processingInstructionsWorkspaceIndex, inputWS);
  }
}

void ReflectometryWorkflowBase2::convertProcessingInstructions(const MatrixWorkspace_sptr &inputWS) {
  m_processingInstructions = getPropertyValue("ProcessingInstructions");
  m_processingInstructionsWorkspaceIndex =
      convertProcessingInstructionsToWorkspaceIndices(m_processingInstructions, inputWS);
}

// Create an on-the-fly property to set an output workspace from a child
// algorithm, if the child has that output value set
void ReflectometryWorkflowBase2::setWorkspacePropertyFromChild(const Algorithm_sptr &alg,
                                                               std::string const &propertyName) {
  if (alg->isDefault(propertyName))
    return;

  if (isDefault(propertyName)) {
    std::string const workspaceName = alg->getPropertyValue(propertyName);
    setPropertyValue(propertyName, workspaceName);
  }
  MatrixWorkspace_sptr workspace = alg->getProperty(propertyName);
  setProperty(propertyName, workspace);
}
} // namespace Mantid::Reflectometry
