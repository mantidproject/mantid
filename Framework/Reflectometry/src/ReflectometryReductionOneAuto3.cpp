// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometryReductionOneAuto3.h"
#include "MantidAPI/BoostOptionalToAlgorithmProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RegexStrings.h"
#include "MantidKernel/Strings.h"

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

static const std::vector<std::string> WILDES_AXES = {"P1", "P2", "F1", "F2"};
static const std::vector<std::string> FREDRIKZE_AXES = {"Pp", "Ap", "Rho", "Alpha"};

// Map correction methods to which correction-option property name they use
static const std::map<std::string, std::string> OPTION_NAME{{CorrectionMethod::WILDES, Prop::FLIPPERS},
                                                            {CorrectionMethod::FREDRIKZE, Prop::POLARIZATION_ANALYSIS}};

void validate(const std::string &method) {
  if (!CorrectionMethod::OPTION_NAME.count(method))
    throw std::invalid_argument("Unsupported polarization correction method: " + method);
}
} // namespace CorrectionMethod

namespace CorrectionOption {
static const std::string PNR{"PNR"};
static const std::string PA{"PA"};
static const std::string FLIPPERS_NO_ANALYSER{"0, 1"};
static const std::string FLIPPERS_FULL{"00, 01, 10, 11"};
} // namespace CorrectionOption

std::vector<std::string> getGroupMemberNames(const std::string &groupName) {
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(groupName);
  return group->getNames();
}

std::string vectorToString(const std::vector<std::string> &vec) {
  std::string result;
  for (const auto &item : vec) {
    if (!result.empty())
      result += ",";
    result += item;
  }
  return result;
}

void removeAllWorkspacesFromGroup(const std::string &groupName) {
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(groupName);
  group->removeAll();
}

void removeWorkspacesFromADS(const std::vector<std::string> &workspaceNames) {
  for (const auto &workspaceName : workspaceNames)
    AnalysisDataService::Instance().remove(workspaceName);
}

bool anyWorkspaceInListExists(std::vector<std::string> const &names) {
  return std::any_of(names.cbegin(), names.cend(),
                     [](std::string const &name) { return AnalysisDataService::Instance().doesExist(name); });
}
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryReductionOneAuto3)

namespace {

const std::string TOF_WORKSPACE_PREFIX("TOF");
const std::string TRANS_WORKSPACE_PREFIX("TRANS");
const std::string SUMMED_WORKSPACE_SUFFIX("_summed_segment");
const std::string OUTPUT_WORKSPACE_BINNED_DEFAULT_PREFIX("IvsQ_binned");
const std::string OUTPUT_WORKSPACE_DEFAULT_PREFIX("IvsQ");
const std::string OUTPUT_WORKSPACE_WAVELENGTH_DEFAULT_PREFIX("IvsLam");
} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string ReflectometryReductionOneAuto3::name() const { return "ReflectometryReductionOneAuto"; }

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryReductionOneAuto3::version() const { return 3; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryReductionOneAuto3::category() const { return "Reflectometry\\ISIS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryReductionOneAuto3::summary() const {
  return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 "
         "workspace attempting to pick instrument parameters for missing "
         "properties";
}

/** Validate individual transmission runs
 *
 * @return :: void
 */
void ReflectometryReductionOneAuto3::getTransmissionRun(std::map<std::string, std::string> &results,
                                                        WorkspaceGroup_sptr &workspaceGroup,
                                                        const std::string &transmissionRun) {
  const std::string str = getPropertyValue(transmissionRun);
  if (!str.empty()) {
    auto transmissionGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(str);
    // If it is not a group, we don't need to validate its size
    if (!transmissionGroup)
      return;

    const bool polarizationCorrections = getProperty("PolarizationAnalysis");

    if (workspaceGroup->size() != transmissionGroup->size() && !polarizationCorrections) {
      // If they are not the same size then we cannot associate a transmission
      // group member with every input group member.
      results[transmissionRun] = transmissionRun + " group must be the "
                                                   "same size as the InputWorkspace group "
                                                   "when polarization analysis is false.";
    }
  }
}

/** Validate transmission runs
 *
 * @return :: result of the validation as a map
 */
std::map<std::string, std::string> ReflectometryReductionOneAuto3::validateInputs() {
  std::map<std::string, std::string> results;

  // Validate transmission runs only if our input workspace is a group
  if (!checkGroups())
    return results;

  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getPropertyValue("InputWorkspace"));
  if (!group)
    return results;

  // First and second transmission runs
  getTransmissionRun(results, group, "FirstTransmissionRun");
  getTransmissionRun(results, group, "SecondTransmissionRun");

  return results;
}

/** Workspace groups do not have a run number but we need to supply one to the
 * reduction. Get the run number of the first member workspace in the group
 */
std::string ReflectometryReductionOneAuto3::getRunNumberForWorkspaceGroup(std::string const &wsName) {
  auto group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
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
auto ReflectometryReductionOneAuto3::getOutputWorkspaceNames() -> WorkspaceNames {
  WorkspaceNames result;
  MatrixWorkspace_const_sptr matrixWs = getProperty("InputWorkspace");

  std::string runNumber;
  if (matrixWs) {
    runNumber = getRunNumber(*matrixWs);
  } else {
    // Casting to WorkspaceGroup doesn't work - I think because InputWorkspace
    // is declared as a MatrixWorkspace - so pass the name and get it from the
    // ADS instead
    runNumber = getRunNumberForWorkspaceGroup(getPropertyValue("InputWorkspace"));
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
void ReflectometryReductionOneAuto3::setDefaultOutputWorkspaceNames() {
  const bool isDebug = getProperty("Debug");
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
void ReflectometryReductionOneAuto3::init() {
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

  initMonitorProperties();
  initBackgroundProperties();
  initTransmissionProperties();
  initAlgorithmicProperties(true);
  initMomentumTransferProperties();

  // Polarization correction
  declareProperty(std::make_unique<PropertyWithValue<bool>>("PolarizationAnalysis", false, Direction::Input),
                  "Apply polarization corrections");

  // Flood correction
  std::vector<std::string> propOptions = {"Workspace", "ParameterFile", "None"};
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
  setPropertySettings("OutputWorkspaceWavelength",
                      std::make_unique<Kernel::EnabledWhenProperty>("Debug", IS_EQUAL_TO, "1"));

  initTransmissionOutputProperties();

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("PolarizationEfficiencies", "", Direction::Input,
                                                                       PropertyMode::Optional),
                  "A workspace to be used for polarization analysis that contains the efficiency factors as "
                  "histograms: P1, P2, F1 and F2 in the Wildes method and Pp, Ap, Rho and Alpha for Fredrikze.");

  // Sum banks
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("ROIDetectorIDs", "", Direction::Input),
                  "When detector IDs are provided, the algorithm will attempt to sum counts across each row of a "
                  "RectangularDetector after the flood correction step. "
                  "Detectors not included in the given range will be masked before summing. "
                  "This will only work correctly when the instrument definition file(IDF) contains a single "
                  "RectangularDetector panel.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("HideSummedWorkspaces", false, Direction::Input),
                  "Whether to hide the workspaces created from the sum banks step, if performed.");
}

/** Execute the algorithm.
 */
void ReflectometryReductionOneAuto3::exec() {
  applyFloodCorrections();
  sumBanks();
  setDefaultOutputWorkspaceNames();

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto instrument = inputWS->getInstrument();
  bool const isDebug = getProperty("Debug");

  Algorithm_sptr alg = createChildAlgorithm("ReflectometryReductionOne");
  alg->initialize();
  // Mandatory properties
  alg->setProperty("SummationType", getPropertyValue("SummationType"));
  alg->setProperty("ReductionType", getPropertyValue("ReductionType"));
  alg->setProperty("IncludePartialBins", getPropertyValue("IncludePartialBins"));
  alg->setProperty("Diagnostics", getPropertyValue("Diagnostics"));
  alg->setProperty("Debug", isDebug);
  double wavMin = checkForMandatoryInstrumentDefault<double>(this, "WavelengthMin", instrument, "LambdaMin");
  alg->setProperty("WavelengthMin", wavMin);
  double wavMax = checkForMandatoryInstrumentDefault<double>(this, "WavelengthMax", instrument, "LambdaMax");
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

  alg->setPropertyValue("SubtractBackground", getPropertyValue("SubtractBackground"));
  alg->setPropertyValue("BackgroundProcessingInstructions", getPropertyValue("BackgroundProcessingInstructions"));
  alg->setPropertyValue("BackgroundCalculationMethod", getPropertyValue("BackgroundCalculationMethod"));
  alg->setPropertyValue("DegreeOfPolynomial", getPropertyValue("DegreeOfPolynomial"));
  alg->setPropertyValue("CostFunction", getPropertyValue("CostFunction"));

  alg->setProperty("InputWorkspace", inputWS);
  alg->execute();

  // Set the unbinned output workspace in Q, scaled and cropped if necessary
  MatrixWorkspace_sptr IvsQ = alg->getProperty("OutputWorkspace");
  IvsQ = scale(IvsQ);
  const auto params = getRebinParams(IvsQ, theta);
  auto IvsQC = cropQ(IvsQ, params);
  setProperty("OutputWorkspace", IvsQC);

  // Set the binned output workspace in Q
  if (params.hasQStep()) {
    MatrixWorkspace_sptr IvsQB = rebin(IvsQ, params);
    setProperty("OutputWorkspaceBinned", IvsQB);
  } else {
    g_log.error("NRCalculateSlitResolution failed. Workspace in Q will not be "
                "rebinned. Please provide dQ/Q.");
    setProperty("OutputWorkspaceBinned", IvsQC);
  }

  // Set the output workspace in wavelength, if debug outputs are enabled
  if (!isDefault("OutputWorkspaceWavelength") || isChild()) {
    MatrixWorkspace_sptr IvsLam = alg->getProperty("OutputWorkspaceWavelength");
    setProperty("OutputWorkspaceWavelength", IvsLam);
  }

  // Set the output transmission workspaces
  setWorkspacePropertyFromChild(alg, "OutputWorkspaceTransmission");
  setWorkspacePropertyFromChild(alg, "OutputWorkspaceFirstTransmission");
  setWorkspacePropertyFromChild(alg, "OutputWorkspaceSecondTransmission");

  // Set other properties so they can be updated in the Reflectometry interface
  setProperty("ThetaIn", theta);
  setProperty("MomentumTransferMin", params.qMin);
  setProperty("MomentumTransferMax", params.qMax);
  if (params.hasQStep())
    setProperty("MomentumTransferStep", -(*params.qStep));
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
std::vector<std::string> ReflectometryReductionOneAuto3::getDetectorNames(const MatrixWorkspace_sptr &inputWS) {
  std::vector<std::string> wsIndices;
  boost::split(wsIndices, m_processingInstructionsWorkspaceIndex, boost::is_any_of(":,-+"));
  // vector of comopnents
  std::vector<std::string> detectors;

  try {
    for (const auto &wsIndex : wsIndices) {

      size_t index = boost::lexical_cast<size_t>(wsIndex);

      auto detector = inputWS->getDetector(index);
      auto parent = detector->getParent();

      if (parent) {
        auto parentType = parent->type();
        auto detectorName = (parentType == "Instrument") ? detector->getName() : parent->getName();
        detectors.emplace_back(detectorName);
      }
    }
  } catch (const boost::bad_lexical_cast &) {
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
MatrixWorkspace_sptr ReflectometryReductionOneAuto3::correctDetectorPositions(MatrixWorkspace_sptr inputWS,
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
double ReflectometryReductionOneAuto3::calculateTheta(const MatrixWorkspace_sptr &inputWS) {
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
void ReflectometryReductionOneAuto3::populateAlgorithmicCorrectionProperties(const IAlgorithm_sptr &alg,
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

auto ReflectometryReductionOneAuto3::getRebinParams(const MatrixWorkspace_sptr &inputWS, const double theta)
    -> RebinParams {
  bool qMinIsDefault = true, qMaxIsDefault = true;
  auto const qMin = getPropertyOrDefault("MomentumTransferMin", inputWS->x(0).front(), qMinIsDefault);
  auto const qMax = getPropertyOrDefault("MomentumTransferMax", inputWS->x(0).back(), qMaxIsDefault);
  return RebinParams{qMin, qMinIsDefault, qMax, qMaxIsDefault, getQStep(inputWS, theta)};
}

/** Get the binning step the final output workspace in Q
 *
 * @param inputWS :: the workspace in Q
 * @param theta :: the angle of this run
 * @return :: the rebin step in Q, or none if it could not be found
 */
std::optional<double> ReflectometryReductionOneAuto3::getQStep(const MatrixWorkspace_sptr &inputWS,
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

/** Rebin a workspace in Q.
 *
 * @param inputWS :: the workspace in Q
 * @param params :: A vector containing the three rebin parameters (min, step
 * and max)
 * @return :: the output workspace
 */
MatrixWorkspace_sptr ReflectometryReductionOneAuto3::rebin(const MatrixWorkspace_sptr &inputWS,
                                                           const RebinParams &params) {
  auto algRebin = createChildAlgorithm("Rebin");
  algRebin->initialize();
  algRebin->setProperty("InputWorkspace", inputWS);
  algRebin->setProperty("OutputWorkspace", inputWS);
  algRebin->setProperty("Params", params.asVector());
  algRebin->execute();
  MatrixWorkspace_sptr binnedWS = algRebin->getProperty("OutputWorkspace");
  return binnedWS;
}

/** Optionally scale a workspace.
 *
 * @param inputWS :: the workspace to scale
 * @return :: the scaled workspace if the ScaleFactor was set or the
 * unchanged input workspace otherwise.
 */
MatrixWorkspace_sptr ReflectometryReductionOneAuto3::scale(MatrixWorkspace_sptr inputWS) {
  Property *scaleProp = getProperty("ScaleFactor");
  if (scaleProp->isDefault())
    return inputWS;

  double scaleFactor = getProperty("ScaleFactor");
  auto algScale = createChildAlgorithm("Scale");
  algScale->initialize();
  algScale->setProperty("InputWorkspace", inputWS);
  algScale->setProperty("OutputWorkspace", inputWS);
  algScale->setProperty("Factor", 1.0 / scaleFactor);
  algScale->execute();
  MatrixWorkspace_sptr scaledWS = algScale->getProperty("OutputWorkspace");
  return scaledWS;
}

/** Optionally crop a workspace in Q.
 *
 * @param inputWS :: the workspace to scale
 * @param params :: A vector containing the three rebin parameters (min, step
 * and max)
 * @return :: the rebinned workspace if a min/max was set, or the unchanged
 * input workspace otherwise.
 */
MatrixWorkspace_sptr ReflectometryReductionOneAuto3::cropQ(MatrixWorkspace_sptr inputWS, const RebinParams &params) {
  if (params.qMinIsDefault && params.qMaxIsDefault)
    return inputWS;

  auto algCrop = createChildAlgorithm("CropWorkspace");
  algCrop->initialize();
  algCrop->setProperty("InputWorkspace", inputWS);
  algCrop->setProperty("OutputWorkspace", inputWS);
  if (!(params.qMinIsDefault))
    algCrop->setProperty("XMin", params.qMin);
  if (!(params.qMaxIsDefault))
    algCrop->setProperty("XMax", params.qMax);
  algCrop->execute();
  MatrixWorkspace_sptr croppedWS = algCrop->getProperty("OutputWorkspace");
  return croppedWS;
}

/**
 * @brief Get the Property Or return a default given value
 *
 * @param propertyName : the name of the property to get
 * @param defaultValue : the default value to use if the property is not set
 * @param isDefault [out] : true if the default value was used
 */
double ReflectometryReductionOneAuto3::getPropertyOrDefault(const std::string &propertyName, const double defaultValue,
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
bool ReflectometryReductionOneAuto3::checkGroups() {
  const std::string wsName = getPropertyValue("InputWorkspace");

  return (AnalysisDataService::Instance().doesExist(wsName) &&
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName));
} // namespace Algorithms

/** Set up the transmission properties on the child algorithm when processing
 * workspace groups.
 *
 * If a transmission input is a matrix workspace, it is applied to all of the
 * workspaces in the input workspace group. If it is a workspace group, then
 * only the first workspace in the group is used, and again is applied to all
 * of the workspaces in the input workspace group.
 */
void ReflectometryReductionOneAuto3::setTransmissionProperties(const Algorithm_sptr &alg,
                                                               std::string const &propertyName) {

  // Get the input transmission workspace. Note that we have to get it by name
  // and retrieve it from the ADS because the property type is MatrixWorkspace
  // so we can't access it using getProperty if it is a WorkspaceGroup.
  const auto inputName = getPropertyValue(propertyName);
  if (inputName.empty())
    return;

  auto inputWS = AnalysisDataService::Instance().retrieveWS<Workspace>(inputName);
  if (!inputWS)
    return;

  MatrixWorkspace_sptr transWS;
  if (inputWS->isGroup()) {
    g_log.information(std::string("A group has been passed as ") + propertyName +
                      std::string("; only the first workspace in the group will be used"));
    auto groupWS = std::dynamic_pointer_cast<WorkspaceGroup>(inputWS);
    transWS = std::dynamic_pointer_cast<MatrixWorkspace>(groupWS->getItem(0));
  } else {
    transWS = std::dynamic_pointer_cast<MatrixWorkspace>(inputWS);
  }

  alg->setProperty(propertyName, transWS);
}

/** Used by processGroups to set up the algorithm to run on each group member.
 *
 * @param inputName : the input workspace name
 * @param outputNames : a struct holding the names to be set for the output
 * workspaces
 * @param recalculateIvsQ : set to true if recalculating IvsQ from existing
 * IvsLam outputs
 */
Algorithm_sptr ReflectometryReductionOneAuto3::createAlgorithmForGroupMember(std::string const &inputName,
                                                                             WorkspaceNames const &outputNames,
                                                                             bool recalculateIvsQ) {
  // Create a copy of ourselves
  Algorithm_sptr alg = createChildAlgorithm(name(), -1, -1, isLogging(), version());
  alg->setChild(false);
  alg->setRethrows(true);

  // Copy all the non-workspace properties over
  const std::vector<Property *> props = getProperties();
  for (const auto &prop : props) {
    if (prop) {
      IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty *>(prop);
      if (!wsProp)
        alg->setPropertyValue(prop->name(), prop->value());
    }
  }

  alg->setProperty("InputWorkspace", inputName);
  alg->setProperty("Debug", true);
  alg->setProperty("OutputWorkspace", outputNames.iVsQ);
  alg->setProperty("OutputWorkspaceBinned", outputNames.iVsQBinned);
  alg->setProperty("OutputWorkspaceWavelength", outputNames.iVsLam);

  if (!recalculateIvsQ) {
    setTransmissionProperties(alg, "FirstTransmissionRun");
    setTransmissionProperties(alg, "SecondTransmissionRun");

    if (!isDefault("FloodWorkspace")) {
      MatrixWorkspace_sptr flood = getProperty("FloodWorkspace");
      alg->setProperty("FloodWorkspace", flood);
    }
  } else {
    // A correction algorithm may be applied by default so if we don't want to
    // apply corrections explicitly set it to None
    alg->setProperty("CorrectionAlgorithm", "None");
    // Change the processing instructions because the input has already been
    // summed, so only has a single spectrum
    auto currentWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputNames.iVsLam));
    auto newProcInst = convertToSpectrumNumber("0", currentWorkspace);
    alg->setProperty("ProcessingInstructions", newProcInst);
    // We only want to recalculate IvsQ, so we should not perform the sum banks or background subtraction steps
    alg->setProperty("SubtractBackground", false);
    alg->setProperty("BackgroundProcessingInstructions", "");
    alg->setProperty("ROIDetectorIDs", "");
  }

  return alg;
}

void ReflectometryReductionOneAuto3::groupWorkspaces(const std::vector<std::string> &workspaceNames,
                                                     std::string const &outputName) {
  if (anyWorkspaceInListExists(workspaceNames)) {
    Algorithm_sptr groupAlg = createChildAlgorithm("GroupWorkspaces");
    groupAlg->setChild(false);
    groupAlg->setRethrows(true);
    groupAlg->setProperty("InputWorkspaces", workspaceNames);
    groupAlg->setProperty("OutputWorkspace", outputName);
    groupAlg->execute();
  }
}

/** Set the output workspaces for the main algorithm based on the grouped
 * outputs of the child algorihms from processGroups
 */
void ReflectometryReductionOneAuto3::setOutputGroupedWorkspaces(std::vector<WorkspaceNames> const &outputNames,
                                                                WorkspaceNames const &outputGroupNames) {
  // Extract each type of output workspaces as a string list for grouping
  std::vector<std::string> IvsQGroup, IvsQBinnedGroup, IvsLamGroup;
  std::for_each(outputNames.cbegin(), outputNames.cend(),
                [&IvsQGroup, &IvsQBinnedGroup, &IvsLamGroup](auto const &names) {
                  IvsQGroup.push_back(names.iVsQ);
                  IvsQBinnedGroup.push_back(names.iVsQBinned);
                  IvsLamGroup.push_back(names.iVsLam);
                });

  groupWorkspaces(IvsQGroup, outputGroupNames.iVsQ);
  groupWorkspaces(IvsQBinnedGroup, outputGroupNames.iVsQBinned);
  groupWorkspaces(IvsLamGroup, outputGroupNames.iVsLam);

  setPropertyValue("OutputWorkspace", outputGroupNames.iVsQ);
  setPropertyValue("OutputWorkspaceBinned", outputGroupNames.iVsQBinned);
  setPropertyValue("OutputWorkspaceWavelength", outputGroupNames.iVsLam);
}

/** Set an output property from a child algorithm
 */
void ReflectometryReductionOneAuto3::setOutputPropertyFromChild(const Algorithm_sptr &alg, std::string const &name) {
  setPropertyValue(name, alg->getPropertyValue(name));
}

/** Set our output properties from a child algorithm
 */
void ReflectometryReductionOneAuto3::setOutputPropertiesFromChild(const Algorithm_sptr &alg) {
  setOutputPropertyFromChild(alg, "ThetaIn");
  setOutputPropertyFromChild(alg, "MomentumTransferMin");
  setOutputPropertyFromChild(alg, "MomentumTransferMax");
  setOutputPropertyFromChild(alg, "MomentumTransferStep");
  setOutputPropertyFromChild(alg, "ScaleFactor");
}

/** This function is used by processGroups to execute the child algorithm over
 * each member in the group
 *
 * @param inputNames : the input workspaces for the child algorithm
 * @param originalNames : the original input group's member workspace names
 * @param runNumber : the run number of the group (our own value is passed in
 * because this is not a property a workspace group has)
 * @param recalculateIvsQ : if true, recalculate IvsQ based on previous IvsLam
 * outputs; IvsLam outputs must be passed as the new inputNames
 * @returns : the grouped output workspace names
 */
auto ReflectometryReductionOneAuto3::processGroupMembers(std::vector<std::string> const &inputNames,
                                                         std::vector<std::string> const &originalNames,
                                                         std::string const &runNumber, bool recalculateIvsQ) {
  // Compile a list of output workspace names for each group member
  std::vector<WorkspaceNames> allOutputNames;
  // Process each group member
  for (size_t i = 0; i < inputNames.size(); ++i) {
    // Get the default output workspace names
    allOutputNames.emplace_back(getOutputNamesForGroupMember(originalNames, runNumber, i));
    auto &outputNames = allOutputNames.back();
    // If recalculating IvsQ, the output IvsLam is the same as the input
    if (recalculateIvsQ)
      outputNames.iVsLam = inputNames[i];
    // Create and execute the child algorithm
    auto alg = createAlgorithmForGroupMember(inputNames[i], outputNames, recalculateIvsQ);
    alg->execute();
    // Update the parent algorithm outputs from the child - use the last run
    // through the loop, but don't overwrite them if recalculating IvsQ
    if (!recalculateIvsQ)
      setOutputPropertiesFromChild(alg);
  }
  // Set the grouped output workspace properties
  const auto groupedOutputNames = getOutputWorkspaceNames();
  setOutputGroupedWorkspaces(allOutputNames, groupedOutputNames);
  return groupedOutputNames;
}

/** Process groups. Groups are processed differently depending on transmission
 * runs and polarization analysis.
 */
bool ReflectometryReductionOneAuto3::processGroups() {
  // this algorithm effectively behaves as MultiPeriodGroupAlgorithm
  m_usingBaseProcessGroups = true;

  auto const groupName = getPropertyValue("InputWorkspace");
  auto const inputNames = getGroupMemberNames(groupName);
  std::string const runNumber = getRunNumberForWorkspaceGroup(groupName);

  auto outputNames = processGroupMembers(inputNames, inputNames, runNumber);

  // If not doing polarization correction, reduction stops here
  const bool polarizationAnalysisOn = getProperty("PolarizationAnalysis");
  if (!polarizationAnalysisOn)
    return true;

  // Correct the IvsLam workspaces
  applyPolarizationCorrection(outputNames.iVsLam);
  // Recalculate IvsQ based on the new IvsLam
  auto const recalculateIvsQ = true;
  auto const correctedIvsLamNames = getGroupMemberNames(outputNames.iVsLam);
  processGroupMembers(correctedIvsLamNames, inputNames, runNumber, recalculateIvsQ);
  return true;
}

/** Get the output workspace names for a workspace in a group.
 * If an input workspace has been passed with the format
 * TOF_<runNumber>_<otherInfo> then the output workspaces will be of the same
 * format otherwise they are numbered according to the wsGroupNumber
 */
auto ReflectometryReductionOneAuto3::getOutputNamesForGroupMember(const std::vector<std::string> &inputNames,
                                                                  const std::string &runNumber,
                                                                  const size_t wsGroupNumber) -> WorkspaceNames {
  auto const &inputName = inputNames[wsGroupNumber];
  const auto output = getOutputWorkspaceNames();
  std::string informativeName = "TOF" + runNumber + "_";

  WorkspaceNames outputNames;
  const auto inputNameSize = inputName.size();
  const auto informativeNameSize = informativeName.size();
  if (inputNameSize >= informativeNameSize && equal(informativeName.begin(), informativeName.end(), inputName.begin(),
                                                    inputName.begin() + informativeNameSize)) {
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

/** Find the polarization correction method to use for the given efficiencies workspace.
 * Checks the workspace axes labels to determine the appropriate correction method.
 * We don't check all the workspace labels here, we just look for the first label that matches
 * one of the efficiency factors for a supported correction method.
 */
std::string
ReflectometryReductionOneAuto3::findPolarizationCorrectionMethod(const API::MatrixWorkspace_sptr &efficiencies) {
  try {
    auto const &axis = dynamic_cast<TextAxis &>(*efficiencies->getAxis(1));

    for (size_t i = 0; i < axis.length(); ++i) {
      if (std::find(CorrectionMethod::WILDES_AXES.begin(), CorrectionMethod::WILDES_AXES.end(), axis.label(i)) !=
          CorrectionMethod::WILDES_AXES.end()) {
        return CorrectionMethod::WILDES;
      }
      if (std::find(CorrectionMethod::FREDRIKZE_AXES.begin(), CorrectionMethod::FREDRIKZE_AXES.end(), axis.label(i)) !=
          CorrectionMethod::FREDRIKZE_AXES.end()) {
        return CorrectionMethod::FREDRIKZE;
      }
    }
  } catch (std::bad_cast &) {
    throw std::runtime_error("Efficiencies workspace is not in a supported format");
  }
  throw std::runtime_error(
      "Axes labels for efficiencies workspace do not match any supported polarization correction method");
}

/** Find the polarization correction option to use for the given correction method, based on the number of workspaces
 * in the input workspace group.
 * For the Fredrikze algorithm the correction option represents the polarization analysis mode.
 * For Wildes the correction option is the flipper configuration. There are more than two possible flipper
 * configurations, but this is mainly intended for use by POLREF initially so we only need to support two for now.
 */
std::string ReflectometryReductionOneAuto3::findPolarizationCorrectionOption(const std::string &correctionMethod) {
  auto groupIvsLam =
      AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getPropertyValue("OutputWorkspaceWavelength"));
  auto numWorkspacesInGrp = groupIvsLam->size();
  if (numWorkspacesInGrp == 2) {
    return (correctionMethod == CorrectionMethod::FREDRIKZE) ? CorrectionOption::PNR
                                                             : CorrectionOption::FLIPPERS_NO_ANALYSER;
  }
  if (numWorkspacesInGrp == 4) {
    return (correctionMethod == CorrectionMethod::FREDRIKZE) ? CorrectionOption::PA : CorrectionOption::FLIPPERS_FULL;
  }
  throw std::runtime_error("Only input workspace groups with two or four periods are supported");
}

/** Construct a polarization efficiencies workspace based on values of input
 * properties.
 */
std::tuple<API::MatrixWorkspace_sptr, std::string, std::string>
ReflectometryReductionOneAuto3::getPolarizationEfficiencies() {
  MatrixWorkspace_sptr efficiencies;
  std::string correctionMethod;
  std::string correctionOption;

  if (!isDefault("PolarizationEfficiencies")) {
    // Get the efficiencies from the provided workspace
    efficiencies = getProperty("PolarizationEfficiencies");
    correctionMethod = findPolarizationCorrectionMethod(efficiencies);
    correctionOption = findPolarizationCorrectionOption(correctionMethod);
  } else {
    // Get the efficiencies from the parameter file
    auto groupIvsLam =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(getPropertyValue("OutputWorkspaceWavelength"));

    Workspace_sptr workspace = groupIvsLam->getItem(0);

    auto effAlg = createChildAlgorithm("ExtractPolarizationEfficiencies");
    effAlg->setProperty("InputWorkspace", workspace);
    effAlg->execute();
    efficiencies = effAlg->getProperty("OutputWorkspace");
    correctionMethod = effAlg->getPropertyValue("CorrectionMethod");
    correctionOption = effAlg->getPropertyValue("CorrectionOption");
  }

  return std::make_tuple(efficiencies, correctionMethod, correctionOption);
}

/**
 * Apply a polarization correction to workspaces in lambda.
 * @param outputIvsLam :: Name of a workspace group to apply the correction
 * to.
 */
void ReflectometryReductionOneAuto3::applyPolarizationCorrection(const std::string &outputIvsLam) {
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
    auto inputNames = getGroupMemberNames(outputIvsLam);
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
MatrixWorkspace_sptr ReflectometryReductionOneAuto3::getFloodWorkspace() {
  const std::string method = getProperty("FloodCorrection");
  if (method == "None") {
    return MatrixWorkspace_sptr();
  }
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
    const auto instrument = inputWS->getInstrument();
    const auto floodRunParam = instrument->getParameterAsString("Flood_Run");
    if (floodRunParam.empty()) {
      throw std::invalid_argument("Instrument parameter file doesn't have the Flood_Run parameter.");
    }
    boost::regex separator("\\s*,\\s*|\\s+");
    const auto parts = Strings::StrParts(floodRunParam, separator);
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
      const std::string prefix("Flood_");
      for (const auto prop : {"StartSpectrum", "EndSpectrum", "ExcludeSpectra", "Background", "CentralPixelSpectrum",
                              "RangeLower", "RangeUpper"}) {
        const auto param = instrument->getParameterAsString(prefix + prop);
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
void ReflectometryReductionOneAuto3::applyFloodCorrection(const MatrixWorkspace_sptr &flood,
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
void ReflectometryReductionOneAuto3::applyFloodCorrections() {
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

/**
 * Gets the name to use for the summed workspace.
 * @param wsPropertyName :: Name of the workspace to be summed.
 * @param isTransWs :: Whether or not this is a transmission workspace.
 */
std::string ReflectometryReductionOneAuto3::getSummedWorkspaceName(const std::string &wsPropertyName,
                                                                   const bool isTransWs) {
  MatrixWorkspace_const_sptr matrixWs = getProperty(wsPropertyName);

  std::string runNumber;
  if (matrixWs) {
    runNumber = getRunNumber(*matrixWs);
  } else {
    runNumber = getRunNumberForWorkspaceGroup(getPropertyValue(wsPropertyName));
  }

  const auto &ws_prefix = isTransWs ? TRANS_WORKSPACE_PREFIX : TOF_WORKSPACE_PREFIX;
  const std::string hide_prefix = getProperty("HideSummedWorkspaces") ? "__" : "";

  return hide_prefix + ws_prefix + runNumber + SUMMED_WORKSPACE_SUFFIX;
}

/**
 * Sum banks for a single data workspace.
 * @param roiDetectorIDs :: The detector IDs to be summed. All are included if an empty string is passed.
 * @param propertyName :: Name of an input property containing a workspace
 *   that should be summed. The summed workspace replaces the old
 *   value of this property.
 * @param isTransWs :: Whether or not this is a transmission workspace.
 */
void ReflectometryReductionOneAuto3::sumBanksForWorkspace(const std::string &roiDetectorIDs,
                                                          const std::string &wsPropertyName, const bool isTransWs) {
  MatrixWorkspace_sptr ws = getProperty(wsPropertyName);
  auto output_ws_name = getSummedWorkspaceName(wsPropertyName, isTransWs);
  auto alg = createChildAlgorithm("ReflectometryISISSumBanks");
  alg->initialize();
  alg->setAlwaysStoreInADS(true);
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("ROIDetectorIDs", roiDetectorIDs);
  alg->setProperty("OutputWorkspace", output_ws_name);
  alg->execute();
  MatrixWorkspace_sptr out =
      std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(output_ws_name));
  setProperty(wsPropertyName, out);
}

/**
 * Sum banks for all workspaces that need to be summed:
 * the input data and the transmission runs.
 */
void ReflectometryReductionOneAuto3::sumBanks() {
  if (!isDefault("ROIDetectorIDs")) {
    const auto roiDetectorIDs = getPropertyValue("ROIDetectorIDs");
    sumBanksForWorkspace(roiDetectorIDs, "InputWorkspace");
    if (!isDefault("FirstTransmissionRun")) {
      sumBanksForWorkspace(roiDetectorIDs, "FirstTransmissionRun", true);
    }
    if (!isDefault("SecondTransmissionRun")) {
      sumBanksForWorkspace(roiDetectorIDs, "SecondTransmissionRun", true);
    }
  }
}

} // namespace Mantid::Reflectometry
