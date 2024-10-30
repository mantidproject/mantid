// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometryReductionOneAuto2.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RegexStrings.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace Mantid::Reflectometry {

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace { // anonymous
// Property names
namespace Prop {
static const std::string FLIPPERS{"Flippers"};
static const std::string POLARIZATION_ANALYSIS{"PolarizationAnalysis"};
} // namespace Prop

namespace CorrectionMethod {
static const std::string WILDES{"Wildes"};
static const std::string FREDRIKZE{"Fredrikze"};

// Map correction methods to which correction-option property name they use
static const std::map<std::string, std::string> OPTION_NAME{{CorrectionMethod::WILDES, Prop::FLIPPERS},
                                                            {CorrectionMethod::FREDRIKZE, Prop::POLARIZATION_ANALYSIS}};

void validate(const std::string &method) {
  if (!CorrectionMethod::OPTION_NAME.count(method))
    throw std::invalid_argument("Unsupported polarization correction method: " + method);
}
} // namespace CorrectionMethod

std::vector<std::string> workspaceNamesInGroup(std::string const &groupName) {
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(groupName);
  return group->getNames();
}

std::string vectorToString(std::vector<std::string> const &vec) {
  std::string result;
  for (auto item : vec) {
    if (!result.empty())
      result += ",";
    result += item;
  }
  return result;
}

void removeAllWorkspacesFromGroup(std::string const &groupName) {
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(groupName);
  group->removeAll();
}

void removeWorkspacesFromADS(std::vector<std::string> const &workspaceNames) {
  for (auto workspaceName : workspaceNames)
    AnalysisDataService::Instance().remove(workspaceName);
}
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryReductionOneAuto2)

namespace {

std::string const OUTPUT_WORKSPACE_BINNED_DEFAULT_PREFIX("IvsQ_binned");
std::string const OUTPUT_WORKSPACE_DEFAULT_PREFIX("IvsQ");
std::string const OUTPUT_WORKSPACE_WAVELENGTH_DEFAULT_PREFIX("IvsLam");
} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryReductionOneAuto2::name() const { return "ReflectometryReductionOneAuto"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryReductionOneAuto2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryReductionOneAuto2::category() const { return "Reflectometry\\ISIS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryReductionOneAuto2::summary() const {
  return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 "
         "workspace attempting to pick instrument parameters for missing "
         "properties";
}

/** Validate transmission runs
 *
 * @return :: result of the validation as a map
 */
std::map<std::string, std::string> ReflectometryReductionOneAuto2::validateInputs() {

  std::map<std::string, std::string> results;

  // Validate transmission runs only if our input workspace is a group
  if (!checkGroups())
    return results;

  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getPropertyValue("InputWorkspace"));
  if (!group)
    return results;

  // First transmission run
  const std::string firstStr = getPropertyValue("FirstTransmissionRun");
  if (!firstStr.empty()) {
    auto firstTransGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(firstStr);
    // If it is not a group, we don't need to validate its size
    if (!firstTransGroup)
      return results;

    const bool polarizationCorrections = getPropertyValue("PolarizationAnalysis") != "None";

    if (group->size() != firstTransGroup->size() && !polarizationCorrections) {
      // If they are not the same size then we cannot associate a transmission
      // group member with every input group member.
      results["FirstTransmissionRun"] = "FirstTransmissionRun group must be the "
                                        "same size as the InputWorkspace group "
                                        "when polarization analysis is 'None'.";
    }
  }

  // The same for the second transmission run
  const std::string secondStr = getPropertyValue("SecondTransmissionRun");
  if (!secondStr.empty()) {
    auto secondTransGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(secondStr);
    // If it is not a group, we don't need to validate its size
    if (!secondTransGroup)
      return results;
    const bool polarizationCorrections = getPropertyValue("PolarizationAnalysis") != "None";

    if (group->size() != secondTransGroup->size() && !polarizationCorrections) {
      results["SecondTransmissionRun"] = "SecondTransmissionRun group must be the "
                                         "same size as the InputWorkspace group "
                                         "when polarization analysis is 'None'.";
    }
  }

  return results;
}

std::string ReflectometryReductionOneAuto2::getRunNumberForWorkspaceGroup(const WorkspaceGroup_const_sptr &group) {
  // Return the run number for the first child workspace
  if (!group)
    throw std::runtime_error("Invalid workspace group type");

  if (group->getNumberOfEntries() < 1)
    throw std::runtime_error("Cannot run algorithm on empty group");

  auto childWs = group->getItem(0);
  auto childMatrixWs = std::dynamic_pointer_cast<MatrixWorkspace>(childWs);

  if (!childMatrixWs)
    throw std::runtime_error("Child workspace is not a MatrixWorkspace");

  return getRunNumber(*childMatrixWs);
}

// Get output workspace names from the user-specified properties, or default
// names if the properties were not specified
auto ReflectometryReductionOneAuto2::getOutputWorkspaceNames() -> WorkspaceNames {
  WorkspaceNames result;
  MatrixWorkspace_const_sptr matrixWs = getProperty("InputWorkspace");

  std::string runNumber;
  if (matrixWs) {
    runNumber = getRunNumber(*matrixWs);
  } else {
    // Casting to WorkspaceGroup doesn't work - I think because InputWorkspace
    // is declared as a MatrixWorkspace - so get it from the ADS instead
    auto groupWs = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getPropertyValue("InputWorkspace"));
    runNumber = getRunNumberForWorkspaceGroup(groupWs);
  }

  if (isDefault("OutputWorkspaceBinned"))
    result.iVsQBinned = OUTPUT_WORKSPACE_BINNED_DEFAULT_PREFIX + runNumber;
  else
    result.iVsQBinned = getPropertyValue("OutputWorkspaceBinned");

  if (isDefault("OutputWorkspace"))
    result.iVsQ = OUTPUT_WORKSPACE_DEFAULT_PREFIX + runNumber;
  else
    result.iVsQ = getPropertyValue("OutputWorkspace");

  if (isDefault("OutputWorkspaceWavelength"))
    result.iVsLam = OUTPUT_WORKSPACE_WAVELENGTH_DEFAULT_PREFIX + runNumber;
  else
    result.iVsLam = getPropertyValue("OutputWorkspaceWavelength");

  return result;
}

// Set default names for output workspaces
void ReflectometryReductionOneAuto2::setDefaultOutputWorkspaceNames() {
  bool const isDebug = getProperty("Debug");
  auto outputNames = getOutputWorkspaceNames();

  if (isDefault("OutputWorkspaceBinned")) {
    setPropertyValue("OutputWorkspaceBinned", outputNames.iVsQBinned);
  }
  if (isDefault("OutputWorkspace")) {
    setPropertyValue("OutputWorkspace", outputNames.iVsQ);
  }
  if (isDebug && isDefault("OutputWorkspaceWavelength")) {
    setPropertyValue("OutputWorkspaceWavelength", outputNames.iVsLam);
  }
}

/** Initialize the algorithm's properties.
 */
void ReflectometryReductionOneAuto2::init() {

  // Input ws
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input,
                                                                       PropertyMode::Mandatory),
                  "Input run in TOF or wavelength");

  // Reduction type
  initReductionProperties();

  // Analysis mode
  initAnalysisProperties();

  // Processing instructions
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("ProcessingInstructions", "", Direction::Input),
                  "Grouping pattern of spectrum numbers to yield only the"
                  " detectors of interest. See GroupDetectors for syntax.");

  // Theta
  declareProperty("ThetaIn", Mantid::EMPTY_DBL(), "Angle in degrees", Direction::Input);

  // ThetaLogName
  declareProperty("ThetaLogName", "", "The name ThetaIn can be found in the run log as");

  // Whether to correct detectors
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CorrectDetectors", true, Direction::Input),
                  "Moves detectors to twoTheta if ThetaIn or ThetaLogName is given");

  // Detector position correction type
  const std::vector<std::string> correctionType{"VerticalShift", "RotateAroundSample"};
  auto correctionTypeValidator = std::make_shared<CompositeValidator>();
  correctionTypeValidator->add(std::make_shared<MandatoryValidator<std::string>>());
  correctionTypeValidator->add(std::make_shared<StringListValidator>(correctionType));
  declareProperty("DetectorCorrectionType", correctionType[0], correctionTypeValidator,
                  "When correcting detector positions, this determines whether detectors"
                  "should be shifted vertically or rotated around the sample position.",
                  Direction::Input);
  setPropertySettings("DetectorCorrectionType",
                      std::make_unique<Kernel::EnabledWhenProperty>("CorrectDetectors", IS_EQUAL_TO, "1"));

  // Wavelength limits
  declareProperty("WavelengthMin", Mantid::EMPTY_DBL(), "Wavelength Min in angstroms", Direction::Input);
  declareProperty("WavelengthMax", Mantid::EMPTY_DBL(), "Wavelength Max in angstroms", Direction::Input);

  // Monitor properties
  initMonitorProperties();

  // Init properties for transmission normalization
  initTransmissionProperties();

  // Init properties for algorithmic corrections
  initAlgorithmicProperties(true);

  // Momentum transfer properties
  initMomentumTransferProperties();

  // Polarization correction
  std::vector<std::string> propOptions = {"None", "PA", "PNR", "ParameterFile"};
  declareProperty("PolarizationAnalysis", "None", std::make_shared<StringListValidator>(propOptions),
                  "Polarization analysis mode.");
  declareProperty(std::make_unique<ArrayProperty<double>>("CPp", Direction::Input),
                  "Effective polarizing power of the polarizing system. "
                  "Expressed as a ratio 0 &lt; Pp &lt; 1");
  declareProperty(std::make_unique<ArrayProperty<double>>("CAp", Direction::Input),
                  "Effective polarizing power of the analyzing system. "
                  "Expressed as a ratio 0 &lt; Ap &lt; 1");
  declareProperty(std::make_unique<ArrayProperty<double>>("CRho", Direction::Input),
                  "Ratio of efficiencies of polarizer spin-down to polarizer "
                  "spin-up. This is characteristic of the polarizer flipper. "
                  "Values are constants for each term in a polynomial "
                  "expression.");
  declareProperty(std::make_unique<ArrayProperty<double>>("CAlpha", Direction::Input),
                  "Ratio of efficiencies of analyzer spin-down to analyzer "
                  "spin-up. This is characteristic of the analyzer flipper. "
                  "Values are factors for each term in a polynomial "
                  "expression.");
  setPropertyGroup("PolarizationAnalysis", "Polarization Corrections");
  setPropertyGroup("CPp", "Polarization Corrections");
  setPropertyGroup("CAp", "Polarization Corrections");
  setPropertyGroup("CRho", "Polarization Corrections");
  setPropertyGroup("CAlpha", "Polarization Corrections");

  // Flood correction
  propOptions = {"Workspace", "ParameterFile"};
  declareProperty("FloodCorrection", "Workspace", std::make_shared<StringListValidator>(propOptions),
                  "The way to apply flood correction: "
                  "Workspace - use FloodWorkspace property to get the flood "
                  "workspace, ParameterFile - use parameters in the parameter "
                  "file to construct and apply flood correction workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("FloodWorkspace", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "A flood workspace to apply; if empty and FloodCorrection is "
                  "'Workspace' then no correction is applied.");

  // Init properties for diagnostics
  initDebugProperties();

  // Output workspace in Q
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspaceBinned", "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Output workspace in Q (rebinned workspace)");

  // Output workspace in Q (unbinned)
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Output workspace in Q (native binning)");

  // Output workspace in wavelength
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspaceWavelength", "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "Output workspace in wavelength");
}

/** Execute the algorithm.
 */
void ReflectometryReductionOneAuto2::exec() {

  applyFloodCorrections();
  setDefaultOutputWorkspaceNames();

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto instrument = inputWS->getInstrument();

  auto alg = createChildAlgorithm("ReflectometryReductionOne");
  alg->initialize();
  // Mandatory properties
  alg->setProperty("SummationType", getPropertyValue("SummationType"));
  alg->setProperty("ReductionType", getPropertyValue("ReductionType"));
  alg->setProperty("IncludePartialBins", getPropertyValue("IncludePartialBins"));
  alg->setProperty("Diagnostics", getPropertyValue("Diagnostics"));
  alg->setProperty("Debug", getPropertyValue("Debug"));
  auto wavMin = checkForMandatoryInstrumentDefault<double>(this, "WavelengthMin", instrument, "LambdaMin");
  alg->setProperty("WavelengthMin", wavMin);
  auto wavMax = checkForMandatoryInstrumentDefault<double>(this, "WavelengthMax", instrument, "LambdaMax");
  alg->setProperty("WavelengthMax", wavMax);

  convertProcessingInstructions(instrument, inputWS);
  alg->setProperty("ProcessingInstructions", m_processingInstructions);
  // Now that we know the detectors of interest, we can move them if
  // necessary (i.e. if theta is given). If not, we calculate theta from the
  // current detector positions
  bool correctDetectors = getProperty("CorrectDetectors");
  double theta;
  if (!getPointerToProperty("ThetaIn")->isDefault()) {
    theta = getProperty("ThetaIn");
  } else if (!getPropertyValue("ThetaLogName").empty()) {
    theta = getThetaFromLogs(inputWS, getPropertyValue("ThetaLogName"));
  } else {
    // Calculate theta from detector positions
    theta = calculateTheta(inputWS);
    // Never correct detector positions if ThetaIn or ThetaLogName is not
    // specified
    correctDetectors = false;
  }

  // Pass theta to the child algorithm
  alg->setProperty("ThetaIn", theta);

  if (correctDetectors) {
    inputWS = correctDetectorPositions(inputWS, 2 * theta);
  }

  // Optional properties

  alg->setPropertyValue("TransmissionProcessingInstructions", getPropertyValue("TransmissionProcessingInstructions"));
  populateMonitorProperties(alg, instrument);
  alg->setPropertyValue("NormalizeByIntegratedMonitors", getPropertyValue("NormalizeByIntegratedMonitors"));
  bool transRunsFound = populateTransmissionProperties(alg);
  if (!transRunsFound)
    populateAlgorithmicCorrectionProperties(alg, instrument);

  alg->setProperty("InputWorkspace", inputWS);
  alg->execute();

  // Set the unbinned output workspace in Q, cropped to the min/max q
  MatrixWorkspace_sptr IvsQ = alg->getProperty("OutputWorkspace");
  auto const params = getRebinParams(IvsQ, theta);
  auto IvsQC = IvsQ;
  if (!(params.qMinIsDefault() && params.qMaxIsDefault()))
    IvsQC = cropQ(IvsQ, params);
  setProperty("OutputWorkspace", IvsQC);

  // Set the binned output workspace in Q
  if (params.hasQStep()) {
    MatrixWorkspace_sptr IvsQB = rebinAndScale(IvsQ, params);
    setProperty("OutputWorkspaceBinned", IvsQB);
  } else {
    g_log.error("NRCalculateSlitResolution failed. Workspace in Q will not be "
                "rebinned. Please provide dQ/Q.");
    setProperty("OutputWorkspaceBinned", IvsQC);
  }

  // Set the output workspace in wavelength, if debug outputs are enabled
  bool const isDebug = getProperty("Debug");
  if (isDebug || isChild()) {
    MatrixWorkspace_sptr IvsLam = alg->getProperty("OutputWorkspaceWavelength");
    setProperty("OutputWorkspaceWavelength", IvsLam);
  }

  // Set other properties so they can be updated in the Reflectometry interface
  setProperty("ThetaIn", theta);
  setProperty("MomentumTransferMin", params.qMin());
  setProperty("MomentumTransferMax", params.qMax());
  if (params.hasQStep())
    setProperty("MomentumTransferStep", -params.qStep());
  if (getPointerToProperty("ScaleFactor")->isDefault())
    setProperty("ScaleFactor", 1.0);
}

/** Returns the detectors of interest, specified via processing instructions.
 * Note that this returns the names of the parent detectors of the first and
 * last spectrum indices in the processing instructions. It is assumed that all
 * the interim detectors have the same parent.
 *
 * @param inputWS :: the input workspace
 * @return :: the names of the detectors of interest
 */
std::vector<std::string> ReflectometryReductionOneAuto2::getDetectorNames(const MatrixWorkspace_sptr &inputWS) {

  std::vector<std::string> wsIndices;
  boost::split(wsIndices, m_processingInstructionsWorkspaceIndex, boost::is_any_of(":,-+"));
  // vector of comopnents
  std::vector<std::string> detectors;

  try {
    for (const auto &wsIndex : wsIndices) {

      auto index = boost::lexical_cast<size_t>(wsIndex);

      auto detector = inputWS->getDetector(index);
      auto parent = detector->getParent();

      if (parent) {
        auto parentType = parent->type();
        auto detectorName = (parentType == "Instrument") ? detector->getName() : parent->getName();
        detectors.emplace_back(detectorName);
      }
    }
  } catch (boost::bad_lexical_cast &) {
    throw std::runtime_error("Invalid processing instructions: " + m_processingInstructionsWorkspaceIndex);
  }

  return detectors;
}

/** Correct an instrument component by shifting it vertically or
 * rotating it around the sample.
 *
 * @param inputWS :: the input workspace
 * @param twoTheta :: the angle to move detectors to
 * @return :: the corrected workspace
 */
MatrixWorkspace_sptr ReflectometryReductionOneAuto2::correctDetectorPositions(MatrixWorkspace_sptr inputWS,
                                                                              const double twoTheta) {

  auto detectorsOfInterest = getDetectorNames(inputWS);

  // Detectors of interest may be empty. This happens for instance when we input
  // a workspace that was previously reduced using this algorithm. In this case,
  // we shouldn't correct the detector positions
  if (detectorsOfInterest.empty())
    return inputWS;

  const std::set<std::string> detectorSet(detectorsOfInterest.begin(), detectorsOfInterest.end());

  const std::string correctionType = getProperty("DetectorCorrectionType");

  MatrixWorkspace_sptr corrected = inputWS;

  for (const auto &detector : detectorSet) {
    auto alg = createChildAlgorithm("SpecularReflectionPositionCorrect");
    alg->setProperty("InputWorkspace", corrected);
    alg->setProperty("TwoTheta", twoTheta);
    alg->setProperty("DetectorCorrectionType", correctionType);
    alg->setProperty("DetectorComponentName", detector);
    alg->execute();
    corrected = alg->getProperty("OutputWorkspace");
  }

  return corrected;
}

/** Calculate the theta value of the detector of interest specified via
 * processing instructions
 *
 * @param inputWS :: the input workspace
 * @return :: the angle of the detector (only the first detector is considered)
 */
double ReflectometryReductionOneAuto2::calculateTheta(const MatrixWorkspace_sptr &inputWS) {

  const auto detectorsOfInterest = getDetectorNames(inputWS);

  // Detectors of interest may be empty. This happens for instance when we input
  // a workspace that was previously reduced using this algorithm. In this case,
  // we can't calculate theta
  if (detectorsOfInterest.empty())
    return 0.0;

  auto alg = createChildAlgorithm("SpecularReflectionCalculateTheta");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("DetectorComponentName", detectorsOfInterest[0]);
  alg->execute();
  const double theta = alg->getProperty("TwoTheta");
  // Take a factor of 0.5 of the detector position, which is expected to be at
  // 2 * theta
  return theta * 0.5;
}

/** Set algorithmic correction properties
 *
 * @param alg :: ReflectometryReductionOne algorithm
 * @param instrument :: The instrument attached to the workspace
 */
void ReflectometryReductionOneAuto2::populateAlgorithmicCorrectionProperties(const IAlgorithm_sptr &alg,
                                                                             const Instrument_const_sptr &instrument) {

  // With algorithmic corrections, monitors should not be integrated, see below

  const std::string correctionAlgorithm = getProperty("CorrectionAlgorithm");

  if (correctionAlgorithm == "PolynomialCorrection") {
    alg->setProperty("NormalizeByIntegratedMonitors", false);
    alg->setProperty("CorrectionAlgorithm", "PolynomialCorrection");
    alg->setPropertyValue("Polynomial", getPropertyValue("Polynomial"));

  } else if (correctionAlgorithm == "ExponentialCorrection") {
    alg->setProperty("NormalizeByIntegratedMonitors", false);
    alg->setProperty("CorrectionAlgorithm", "ExponentialCorrection");
    alg->setProperty("C0", getPropertyValue("C0"));
    alg->setProperty("C1", getPropertyValue("C1"));

  } else if (correctionAlgorithm == "AutoDetect") {
    // Figure out what to do from the instrument
    try {
      const auto corrVec = instrument->getStringParameter("correction");
      if (corrVec.empty()) {
        throw std::runtime_error("Could not find parameter 'correction' in "
                                 "parameter file. Cannot auto detect the type of "
                                 "correction.");
      }

      const std::string correctionStr = corrVec[0];

      if (correctionStr == "polynomial") {
        const auto polyVec = instrument->getStringParameter("polystring");
        if (polyVec.empty())
          throw std::runtime_error("Could not find parameter 'polystring' in "
                                   "parameter file. Cannot apply polynomial "
                                   "correction.");
        alg->setProperty("CorrectionAlgorithm", "PolynomialCorrection");
        alg->setProperty("Polynomial", polyVec[0]);
      } else if (correctionStr == "exponential") {
        const auto c0Vec = instrument->getStringParameter("C0");
        if (c0Vec.empty())
          throw std::runtime_error("Could not find parameter 'C0' in parameter "
                                   "file. Cannot apply exponential correction.");
        const auto c1Vec = instrument->getStringParameter("C1");
        if (c1Vec.empty())
          throw std::runtime_error("Could not find parameter 'C1' in parameter "
                                   "file. Cannot apply exponential correction.");
        alg->setProperty("C0", c0Vec[0]);
        alg->setProperty("C1", c1Vec[0]);
      }
      alg->setProperty("NormalizeByIntegratedMonitors", false);
    } catch (std::runtime_error &e) {
      g_log.error() << e.what() << ". Polynomial correction will not be performed.";
      alg->setProperty("CorrectionAlgorithm", "None");
    }
  } else {
    alg->setProperty("CorrectionAlgorithm", "None");
  }
}

auto ReflectometryReductionOneAuto2::getRebinParams(const MatrixWorkspace_sptr &inputWS, const double theta)
    -> RebinParams {
  bool qMinIsDefault = true, qMaxIsDefault = true;
  auto const qMin = getPropertyOrDefault("MomentumTransferMin", inputWS->x(0).front(), qMinIsDefault);
  auto const qMax = getPropertyOrDefault("MomentumTransferMax", inputWS->x(0).back(), qMaxIsDefault);
  return RebinParams(qMin, qMinIsDefault, qMax, qMaxIsDefault, getQStep(inputWS, theta));
}

/** Get the binning step the final output workspace in Q
 *
 * @param inputWS :: the workspace in Q
 * @param theta :: the angle of this run
 * @return :: the rebin step in Q, or none if it could not be found
 */
std::optional<double> ReflectometryReductionOneAuto2::getQStep(const MatrixWorkspace_sptr &inputWS,
                                                               const double theta) {
  Property *qStepProp = getProperty("MomentumTransferStep");
  double qstep;
  if (!qStepProp->isDefault()) {
    qstep = getProperty("MomentumTransferStep");
    qstep = -qstep;
  } else {
    if (theta == 0.0) {
      throw std::runtime_error("Theta determined from the detector positions is "
                               "0.0. Please provide a value for theta manually "
                               "or correct the detector position before running "
                               "this algorithm.");
    }

    auto calcRes = createChildAlgorithm("NRCalculateSlitResolution");
    calcRes->setProperty("Workspace", inputWS);
    calcRes->setProperty("TwoTheta", 2 * theta);
    calcRes->execute();

    if (!calcRes->isExecuted()) {
      return std::nullopt;
    }
    qstep = calcRes->getProperty("Resolution");
    qstep = -qstep;
  }
  return qstep;
}

/** Rebin and scale a workspace in Q.
 *
 * @param inputWS :: the workspace in Q
 * @param params :: A vector containing the three rebin parameters (min, step
 * and max)
 * @return :: the output workspace
 */
MatrixWorkspace_sptr ReflectometryReductionOneAuto2::rebinAndScale(const MatrixWorkspace_sptr &inputWS,
                                                                   RebinParams const &params) {
  // Rebin
  auto algRebin = createChildAlgorithm("Rebin");
  algRebin->initialize();
  algRebin->setProperty("InputWorkspace", inputWS);
  algRebin->setProperty("OutputWorkspace", inputWS);
  algRebin->setProperty("Params", params.asVector());
  algRebin->execute();
  MatrixWorkspace_sptr IvsQ = algRebin->getProperty("OutputWorkspace");

  // Scale (optional)
  Property *scaleProp = getProperty("ScaleFactor");
  if (!scaleProp->isDefault()) {
    double scaleFactor = getProperty("ScaleFactor");
    auto algScale = createChildAlgorithm("Scale");
    algScale->initialize();
    algScale->setProperty("InputWorkspace", IvsQ);
    algScale->setProperty("OutputWorkspace", IvsQ);
    algScale->setProperty("Factor", 1.0 / scaleFactor);
    algScale->execute();
    IvsQ = algScale->getProperty("OutputWorkspace");
  }

  return IvsQ;
}

MatrixWorkspace_sptr ReflectometryReductionOneAuto2::cropQ(const MatrixWorkspace_sptr &inputWS,
                                                           RebinParams const &params) {
  auto algCrop = createChildAlgorithm("CropWorkspace");
  algCrop->initialize();
  algCrop->setProperty("InputWorkspace", inputWS);
  algCrop->setProperty("OutputWorkspace", inputWS);
  if (!params.qMinIsDefault())
    algCrop->setProperty("XMin", params.qMin());
  if (!params.qMaxIsDefault())
    algCrop->setProperty("XMax", params.qMax());
  algCrop->execute();
  MatrixWorkspace_sptr IvsQ = algCrop->getProperty("OutputWorkspace");
  return IvsQ;
}

/**
 * @brief Get the Property Or return a default given value
 *
 * @param propertyName : the name of the property to get
 * @param defaultValue : the default value to use if the property is not set
 * @param isDefault [out] : true if the default value was used
 */
double ReflectometryReductionOneAuto2::getPropertyOrDefault(const std::string &propertyName, const double defaultValue,
                                                            bool &isDefault) {
  Property *property = getProperty(propertyName);
  isDefault = property->isDefault();
  if (isDefault)
    return defaultValue;
  else
    return getProperty(propertyName);
}

/** Check if input workspace is a group
 */
bool ReflectometryReductionOneAuto2::checkGroups() {

  const std::string wsName = getPropertyValue("InputWorkspace");

  try {
    auto ws = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    if (ws)
      return true;
  } catch (...) {
  }
  return false;
}

void ReflectometryReductionOneAuto2::setOutputWorkspaces(WorkspaceNames const &outputGroupNames,
                                                         std::vector<std::string> &IvsLamGroup,
                                                         std::vector<std::string> &IvsQBinnedGroup,
                                                         std::vector<std::string> &IvsQGroup) {
  // Group the IvsQ and IvsLam workspaces
  Algorithm_sptr groupAlg = createChildAlgorithm("GroupWorkspaces");
  groupAlg->setChild(false);
  groupAlg->setRethrows(true);
  if (!IvsLamGroup.empty()) {
    groupAlg->setProperty("InputWorkspaces", IvsLamGroup);
    groupAlg->setProperty("OutputWorkspace", outputGroupNames.iVsLam);
    groupAlg->execute();
  }
  groupAlg->setProperty("InputWorkspaces", IvsQBinnedGroup);
  groupAlg->setProperty("OutputWorkspace", outputGroupNames.iVsQBinned);
  groupAlg->execute();
  groupAlg->setProperty("InputWorkspaces", IvsQGroup);
  groupAlg->setProperty("OutputWorkspace", outputGroupNames.iVsQ);
  groupAlg->execute();

  setPropertyValue("OutputWorkspace", outputGroupNames.iVsQ);
  setPropertyValue("OutputWorkspaceBinned", outputGroupNames.iVsQBinned);
  setPropertyValue("OutputWorkspaceWavelength", outputGroupNames.iVsLam);
}

/** Process groups. Groups are processed differently depending on transmission
 * runs and polarization analysis. If transmission run is a matrix workspace,
 * it will be applied to each of the members in the input workspace group. If
 * transmission run is a workspace group, the behaviour is different depending
 * on polarization analysis. If polarization analysis is off (i.e.
 * 'PolarizationAnalysis' is set to 'None') each item in the transmission
 * group is associated with the corresponding item in the input workspace
 * group. If polarization analysis is on (i.e. 'PolarizationAnalysis' is 'PA'
 * or 'PNR') items in the transmission group will be summed to produce a
 * matrix workspace that will be applied to each of the items in the input
 * workspace group. See documentation of this algorithm for more details.
 */
bool ReflectometryReductionOneAuto2::processGroups() {
  // this algorithm effectively behaves as MultiPeriodGroupAlgorithm
  m_usingBaseProcessGroups = true;

  // Get our input workspace group
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getPropertyValue("InputWorkspace"));
  // Get the output workspace names
  auto const output = getOutputWorkspaceNames();

  // Create a copy of ourselves
  Algorithm_sptr alg = createChildAlgorithm(name(), -1, -1, isLogging(), version());
  alg->setChild(false);
  alg->setRethrows(true);

  // Copy all the non-workspace properties over
  const std::vector<Property *> props = getProperties();
  for (auto &prop : props) {
    if (prop) {
      auto *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
      if (!wsProp)
        alg->setPropertyValue(prop->name(), prop->value());
    }
  }

  const bool polarizationAnalysisOn = getPropertyValue("PolarizationAnalysis") != "None";

  // Check if the transmission runs are groups or not

  const std::string firstTrans = getPropertyValue("FirstTransmissionRun");
  WorkspaceGroup_sptr firstTransG;
  MatrixWorkspace_sptr firstTransSum;
  if (!firstTrans.empty()) {
    auto firstTransWS = AnalysisDataService::Instance().retrieveWS<Workspace>(firstTrans);
    firstTransG = std::dynamic_pointer_cast<WorkspaceGroup>(firstTransWS);
    if (!firstTransG) {
      alg->setProperty("FirstTransmissionRun", firstTrans);
    } else {
      g_log.information("A group has been passed as the first transmission run "
                        "so the first run only is being used");
      alg->setProperty("FirstTransmissionRun", firstTransG->getItem(0));
    }
  }
  const std::string secondTrans = getPropertyValue("SecondTransmissionRun");
  WorkspaceGroup_sptr secondTransG;
  MatrixWorkspace_sptr secondTransSum;
  if (!secondTrans.empty()) {
    auto secondTransWS = AnalysisDataService::Instance().retrieveWS<Workspace>(secondTrans);
    secondTransG = std::dynamic_pointer_cast<WorkspaceGroup>(secondTransWS);
    if (!secondTransG) {
      alg->setProperty("SecondTransmissionRun", secondTrans);
    } else {
      g_log.information("A group has been passed as the second transmission "
                        "run so the first run only is being used");
      alg->setProperty("secondTransmissionRun", secondTransG->getItem(0));
    }
  }

  std::vector<std::string> IvsQBinnedGroup, IvsQGroup, IvsLamGroup;
  std::string runNumber = getRunNumberForWorkspaceGroup(group);

  // Execute algorithm over each group member
  for (size_t i = 0; i < group->size(); ++i) {
    auto inputName = group->getItem(i)->getName();
    auto outputNames = getOutputNamesForGroups(inputName, runNumber, i);

    const std::string IvsQName = outputNames.iVsQ;
    const std::string IvsQBinnedName = outputNames.iVsQBinned;
    const std::string IvsLamName = outputNames.iVsLam;

    alg->setProperty("InputWorkspace", inputName);
    alg->setProperty("Debug", true);
    alg->setProperty("OutputWorkspace", IvsQName);
    alg->setProperty("OutputWorkspaceBinned", IvsQBinnedName);
    alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
    if (!isDefault("FloodWorkspace")) {
      MatrixWorkspace_sptr flood = getProperty("FloodWorkspace");
      alg->setProperty("FloodWorkspace", flood);
    }
    alg->execute();

    IvsQGroup.emplace_back(IvsQName);
    IvsQBinnedGroup.emplace_back(IvsQBinnedName);
    if (AnalysisDataService::Instance().doesExist(IvsLamName)) {
      IvsLamGroup.emplace_back(IvsLamName);
    }
  }

  // Group the IvsQ and IvsLam workspaces
  Algorithm_sptr groupAlg = createChildAlgorithm("GroupWorkspaces");
  groupAlg->setChild(false);
  groupAlg->setRethrows(true);
  if (!IvsLamGroup.empty()) {
    groupAlg->setProperty("InputWorkspaces", IvsLamGroup);
    groupAlg->setProperty("OutputWorkspace", output.iVsLam);
    groupAlg->execute();
  }
  groupAlg->setProperty("InputWorkspaces", IvsQBinnedGroup);
  groupAlg->setProperty("OutputWorkspace", output.iVsQBinned);
  groupAlg->execute();
  groupAlg->setProperty("InputWorkspaces", IvsQGroup);
  groupAlg->setProperty("OutputWorkspace", output.iVsQ);
  groupAlg->execute();

  // Set other properties so they can be updated in the Reflectometry interface
  setPropertyValue("ThetaIn", alg->getPropertyValue("ThetaIn"));
  setPropertyValue("MomentumTransferMin", alg->getPropertyValue("MomentumTransferMin"));
  setPropertyValue("MomentumTransferMax", alg->getPropertyValue("MomentumTransferMax"));
  setPropertyValue("MomentumTransferStep", alg->getPropertyValue("MomentumTransferStep"));
  setPropertyValue("ScaleFactor", alg->getPropertyValue("ScaleFactor"));

  setOutputWorkspaces(output, IvsLamGroup, IvsQBinnedGroup, IvsQGroup);

  if (!polarizationAnalysisOn) {
    // No polarization analysis. Reduction stops here
    return true;
  }

  applyPolarizationCorrection(output.iVsLam);

  // Polarization correction may have changed the number of workspaces in the
  // groups
  IvsLamGroup.clear();
  IvsQBinnedGroup.clear();
  IvsQGroup.clear();

  // Now we've overwritten the IvsLam workspaces, we'll need to recalculate
  // the IvsQ ones
  alg->setProperty("FirstTransmissionRun", "");
  alg->setProperty("SecondTransmissionRun", "");
  alg->setProperty("CorrectionAlgorithm", "None");

  auto outputIvsLamNames = workspaceNamesInGroup(output.iVsLam);
  for (size_t i = 0; i < outputIvsLamNames.size(); ++i) {
    auto inputName = group->getItem(i)->getName();
    auto outputNames = getOutputNamesForGroups(inputName, runNumber, i);

    const std::string IvsQName = outputNames.iVsQ;
    const std::string IvsQBinnedName = outputNames.iVsQBinned;
    const std::string IvsLamName = outputIvsLamNames[i];

    // Find the spectrum processing instructions for ws index 0
    auto currentWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputIvsLamNames[i]));
    auto newProcInst = convertToSpectrumNumber("0", currentWorkspace);
    alg->setProperty("ProcessingInstructions", newProcInst);
    alg->setProperty("InputWorkspace", IvsLamName);
    alg->setProperty("OutputWorkspace", IvsQName);
    alg->setProperty("OutputWorkspaceBinned", IvsQBinnedName);
    alg->setProperty("OutputWorkspaceWavelength", IvsLamName);
    alg->execute();
    IvsQBinnedGroup.emplace_back(IvsQBinnedName);
    IvsQGroup.emplace_back(IvsQName);
    if (AnalysisDataService::Instance().doesExist(IvsLamName)) {
      IvsLamGroup.emplace_back(IvsLamName);
    }
  }

  setOutputWorkspaces(output, IvsLamGroup, IvsQBinnedGroup, IvsQGroup);

  return true;
}

/** Get the output workspace names for a workspace in a group.
 * If an input workspace has been passed with the format
 * TOF_<runNumber>_<otherInfo> then the output workspaces will be of the same
 * format otherwise they are numbered according to the wsGroupNumber
 */
auto ReflectometryReductionOneAuto2::getOutputNamesForGroups(const std::string &inputName, const std::string &runNumber,
                                                             const size_t wsGroupNumber) -> WorkspaceNames {
  auto const output = getOutputWorkspaceNames();
  std::string informativeName = "TOF" + runNumber + "_";

  WorkspaceNames outputNames;
  if ((informativeName.length() < inputName.length()) &&
      (equal(informativeName.begin(), informativeName.end(), inputName.begin()))) {
    auto informativeTest = inputName.substr(informativeName.length());
    outputNames.iVsQ = output.iVsQ + "_" + informativeTest;
    outputNames.iVsQBinned = output.iVsQBinned + "_" + informativeTest;
    outputNames.iVsLam = output.iVsLam + "_" + informativeTest;
  } else {
    outputNames.iVsQ = output.iVsQ + "_" + std::to_string(wsGroupNumber + 1);
    outputNames.iVsQBinned = output.iVsQBinned + "_" + std::to_string(wsGroupNumber + 1);
    outputNames.iVsLam = output.iVsLam + "_" + std::to_string(wsGroupNumber + 1);
  }

  return outputNames;
}

/** Construct a polarization efficiencies workspace based on values of input
 * properties.
 */
std::tuple<API::MatrixWorkspace_sptr, std::string, std::string>
ReflectometryReductionOneAuto2::getPolarizationEfficiencies() {
  auto groupIvsLam =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getPropertyValue("OutputWorkspaceWavelength"));

  std::string const paMethod = getPropertyValue("PolarizationAnalysis");
  Workspace_sptr workspace = groupIvsLam->getItem(0);
  MatrixWorkspace_sptr efficiencies;
  std::string correctionMethod;
  std::string correctionOption;

  if (paMethod == "ParameterFile") {
    auto effAlg = createChildAlgorithm("ExtractPolarizationEfficiencies");
    effAlg->setProperty("InputWorkspace", workspace);
    effAlg->execute();
    efficiencies = effAlg->getProperty("OutputWorkspace");
    correctionMethod = effAlg->getPropertyValue("CorrectionMethod");
    correctionOption = effAlg->getPropertyValue("CorrectionOption");
  } else {
    auto effAlg = createChildAlgorithm("CreatePolarizationEfficiencies");
    effAlg->setProperty("InputWorkspace", workspace);
    if (!isDefault("CPp")) {
      effAlg->setProperty("Pp", getPropertyValue("CPp"));
    }
    if (!isDefault("CRho")) {
      effAlg->setProperty("Rho", getPropertyValue("CRho"));
    }
    if (!isDefault("CAp")) {
      effAlg->setProperty("Ap", getPropertyValue("CAp"));
    }
    if (!isDefault("CAlpha")) {
      effAlg->setProperty("Alpha", getPropertyValue("CAlpha"));
    }
    effAlg->execute();
    efficiencies = effAlg->getProperty("OutputWorkspace");
    correctionMethod = "Fredrikze";
    correctionOption = paMethod;
  }
  return std::make_tuple(efficiencies, correctionMethod, correctionOption);
}

/**
 * Apply a polarization correction to workspaces in lambda.
 * @param outputIvsLam :: Name of a workspace group to apply the correction
 * to.
 */
void ReflectometryReductionOneAuto2::applyPolarizationCorrection(std::string const &outputIvsLam) {
  MatrixWorkspace_sptr efficiencies;
  std::string correctionMethod;
  std::string correctionOption;
  std::tie(efficiencies, correctionMethod, correctionOption) = getPolarizationEfficiencies();
  CorrectionMethod::validate(correctionMethod);

  Algorithm_sptr polAlg = createChildAlgorithm("PolarizationEfficiencyCor");
  polAlg->setChild(false);
  polAlg->setRethrows(true);
  polAlg->setProperty("OutputWorkspace", outputIvsLam);
  polAlg->setProperty("Efficiencies", efficiencies);
  polAlg->setProperty("CorrectionMethod", correctionMethod);
  polAlg->setProperty(CorrectionMethod::OPTION_NAME.at(correctionMethod), correctionOption);

  if (correctionMethod == "Fredrikze") {
    polAlg->setProperty("InputWorkspaceGroup", outputIvsLam);
    polAlg->execute();
  } else {
    // The Wildes algorithm doesn't handle things well if the input workspaces
    // are in the same group that you specify as the output group, so move the
    // input workspaces out of the group first and delete them when finished
    auto inputNames = workspaceNamesInGroup(outputIvsLam);
    auto inputNamesString = vectorToString(inputNames);
    removeAllWorkspacesFromGroup(outputIvsLam);

    polAlg->setProperty("InputWorkspaces", inputNamesString);
    polAlg->execute();

    removeWorkspacesFromADS(inputNames);
  }
}

/**
 * Get the flood workspace for flood correction. If it is provided via the
 * FloodWorkspace property return it. Otherwise create it using parameters
 * in the instrument parameter file.
 */
MatrixWorkspace_sptr ReflectometryReductionOneAuto2::getFloodWorkspace() {
  std::string const method = getProperty("FloodCorrection");
  if (method == "Workspace" && !isDefault("FloodWorkspace")) {
    return getProperty("FloodWorkspace");
  } else if (method == "ParameterFile") {
    if (!isDefault("FloodWorkspace")) {
      g_log.warning() << "Flood correction is performed using data in the "
                         "Parameter File. Value of FloodWorkspace property is "
                         "ignored."
                      << std::endl;
    }
    MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    auto const instrument = inputWS->getInstrument();
    auto const floodRunParam = instrument->getParameterAsString("Flood_Run");
    if (floodRunParam.empty()) {
      throw std::invalid_argument("Instrument parameter file doesn't have the Flood_Run parameter.");
    }
    boost::regex separator("\\s*,\\s*|\\s+");
    auto const parts = Strings::StrParts(floodRunParam, separator);
    if (!parts.empty()) {
      std::string fileName = floodRunParam;
      try {
        // If the first part is a number treat all parts as run numbers
        boost::lexical_cast<size_t>(parts.front());
        fileName = instrument->getName() + Strings::toString(parts);
      } catch (const boost::bad_lexical_cast &) {
        // Do nothing fileName == floodRunParam
      }
      auto alg = createChildAlgorithm("CreateFloodWorkspace");
      alg->initialize();
      alg->setProperty("Filename", fileName);
      std::string const prefix("Flood_");
      for (auto const prop : {"StartSpectrum", "EndSpectrum", "ExcludeSpectra", "Background", "CentralPixelSpectrum",
                              "RangeLower", "RangeUpper"}) {
        auto const param = instrument->getParameterAsString(prefix + prop);
        if (!param.empty()) {
          alg->setPropertyValue(prop, param);
        }
      }
      alg->execute();
      MatrixWorkspace_sptr out = alg->getProperty("OutputWorkspace");
      return out;
    }
  }
  return MatrixWorkspace_sptr();
}

/**
 * Apply flood correction to a single data workspace.
 * @param flood :: The flood workspace.
 * @param propertyName :: Name of an input property containing a workspace
 *   that should be corrected. The corrected workspace replaces the old
 *   value of this property.
 */
void ReflectometryReductionOneAuto2::applyFloodCorrection(MatrixWorkspace_sptr const &flood,
                                                          const std::string &propertyName) {
  MatrixWorkspace_sptr ws = getProperty(propertyName);
  auto alg = createChildAlgorithm("ApplyFloodWorkspace");
  alg->initialize();
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("FloodWorkspace", flood);
  alg->execute();
  MatrixWorkspace_sptr out = alg->getProperty("OutputWOrkspace");
  setProperty(propertyName, out);
}

/**
 * Apply flood correction to all workspaces that need to be corrected:
 * the input data and the transmission runs.
 */
void ReflectometryReductionOneAuto2::applyFloodCorrections() {
  MatrixWorkspace_sptr flood = getFloodWorkspace();
  if (flood) {
    applyFloodCorrection(flood, "InputWorkspace");
    if (!isDefault("FirstTransmissionRun")) {
      applyFloodCorrection(flood, "FirstTransmissionRun");
    }
    if (!isDefault("SecondTransmissionRun")) {
      applyFloodCorrection(flood, "SecondTransmissionRun");
    }
  }
}

} // namespace Mantid::Reflectometry
