// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/CreateTransmissionWorkspace2.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Reflectometry {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateTransmissionWorkspace2)

namespace {
// Prefix for names of intermediate transmission workspaces in lambda
std::string const TRANS_LAM_PREFIX("TRANS_LAM_");
} // namespace

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateTransmissionWorkspace2::name() const {
  return "CreateTransmissionWorkspace";
}
/// Summary of algorithm's purpose
const std::string CreateTransmissionWorkspace2::summary() const {
  return "Creates a transmission run workspace in wavelength from one or two "
         "input workspaces in TOF.";
}
/// Algorithm's version for identification. @see Algorithm::version
int CreateTransmissionWorkspace2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateTransmissionWorkspace2::category() const {
  return "Reflectometry";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateTransmissionWorkspace2::init() {
  auto inputValidator = std::make_shared<WorkspaceUnitValidator>("TOF");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "FirstTransmissionRun", "", Direction::Input,
                      PropertyMode::Mandatory, inputValidator->clone()),
                  "First transmission run. Corresponds to the low wavelength "
                  "transmission run if a SecondTransmissionRun is also "
                  "provided.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional, inputValidator->clone()),
                  "High wavelength transmission run. Optional. Causes the "
                  "first transmission run to be treated as the low wavelength "
                  "transmission run.");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "",
                      std::make_shared<MandatoryValidator<std::string>>(),
                      Direction::Input),
                  "Grouping pattern on spectrum numbers to yield only "
                  "the detectors of interest. See GroupDetectors for details.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "WavelengthMin", Mantid::EMPTY_DBL(),
                      std::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength minimum in angstroms");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "WavelengthMax", Mantid::EMPTY_DBL(),
                      std::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength maximum in angstroms");

  initMonitorProperties();

  initStitchProperties();

  declareProperty("Debug", false,
                  "Whether to enable the output of extra workspaces.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output, PropertyMode::Optional),
      "Output workspace in wavelength.");

  // Declare Debug output workspaces

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceFirstTransmission", "", Direction::Output,
                      PropertyMode::Optional),
                  "Output workspace in wavelength for first transmission run");
  setPropertySettings(
      "OutputWorkspaceFirstTransmission",
      std::make_unique<Kernel::EnabledWhenProperty>("Debug", IS_EQUAL_TO, "1"));

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspaceSecondTransmission", "",
                      Direction::Output, PropertyMode::Optional),
                  "Output workspace in wavelength for second transmission run");
  setPropertySettings(
      "OutputWorkspaceSecondTransmission",
      std::make_unique<Kernel::EnabledWhenProperty>("Debug", IS_EQUAL_TO, "1"));
}

/** Validate inputs
 * @return :: error message to show
 */
std::map<std::string, std::string>
CreateTransmissionWorkspace2::validateInputs() {

  std::map<std::string, std::string> results;

  // Validate wavelength range
  // Validate monitor background range
  // Validate monitor integration range
  const auto wavelength = validateWavelengthRanges();
  results.insert(wavelength.begin(), wavelength.end());

  return results;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateTransmissionWorkspace2::exec() {
  getRunNumbers();

  // Process the first run
  MatrixWorkspace_sptr firstTransWS = getProperty("FirstTransmissionRun");
  convertProcessingInstructions(firstTransWS);
  firstTransWS = normalizeDetectorsByMonitors(firstTransWS);
  firstTransWS = cropWavelength(firstTransWS);

  // If we only have one run, set it as the output and finish
  if (isDefault("SecondTransmissionRun")) {
    setOutputWorkspace(firstTransWS);
    return;
  }

  // Process the second run
  MatrixWorkspace_sptr secondTransWS = getProperty("SecondTransmissionRun");
  convertProcessingInstructions(secondTransWS);
  secondTransWS = normalizeDetectorsByMonitors(secondTransWS);
  secondTransWS = cropWavelength(secondTransWS);

  // Stitch the processed runs
  auto stitch = createChildAlgorithm("Stitch1D");
  stitch->initialize();
  stitch->setProperty("LHSWorkspace", firstTransWS);
  stitch->setProperty("RHSWorkspace", secondTransWS);
  stitch->setPropertyValue("StartOverlap", getPropertyValue("StartOverlap"));
  stitch->setPropertyValue("EndOverlap", getPropertyValue("EndOverlap"));
  stitch->setPropertyValue("Params", getPropertyValue("Params"));
  stitch->setProperty("ScaleRHSWorkspace",
                      getPropertyValue("ScaleRHSWorkspace"));
  stitch->execute();
  MatrixWorkspace_sptr stitchedWS = stitch->getProperty("OutputWorkspace");

  // Set the outputs
  setOutputWorkspace(stitchedWS);
  setOutputTransmissionRun(1, firstTransWS);
  setOutputTransmissionRun(2, secondTransWS);
}

/** Normalize detectors by monitors
 * @param IvsTOF :: a workspace in TOF that contains spectra for both
 * monitors and detectors
 * @return :: the normalized workspace in Wavelength
 */
MatrixWorkspace_sptr CreateTransmissionWorkspace2::normalizeDetectorsByMonitors(
    const MatrixWorkspace_sptr &IvsTOF) {

  // Detector workspace
  MatrixWorkspace_sptr detectorWS = makeDetectorWS(IvsTOF);

  // Monitor workspace
  // Only if I0MonitorIndex, MonitorBackgroundWavelengthMin
  // and MonitorBackgroundWavelengthMax have been given

  Property *monProperty = getProperty("I0MonitorIndex");
  Property *backgroundMinProperty =
      getProperty("MonitorBackgroundWavelengthMin");
  Property *backgroundMaxProperty =
      getProperty("MonitorBackgroundWavelengthMin");
  if (monProperty->isDefault() || backgroundMinProperty->isDefault() ||
      backgroundMaxProperty->isDefault()) {
    return detectorWS;
  }

  // Normalization by integrated monitors
  // Only if defined by property
  const bool normalizeByIntegratedMonitors =
      getProperty("NormalizeByIntegratedMonitors");

  auto monitorWS = makeMonitorWS(IvsTOF, normalizeByIntegratedMonitors);
  if (!normalizeByIntegratedMonitors)
    detectorWS = rebinDetectorsToMonitors(detectorWS, monitorWS);

  return divide(detectorWS, monitorWS);
}

/** Get the run numbers of the input workspaces and store them
 * in class variables.
 */
void CreateTransmissionWorkspace2::getRunNumbers() {
  m_firstTransmissionRunNumber = getRunNumber("FirstTransmissionRun");
  m_secondTransmissionRunNumber = getRunNumber("SecondTransmissionRun");
}

/** Get the run number for a given workspace. Also sets the m_missingRunNumber
 * flag if a required run number could not be found.
 * @returns : the run number as a string, or an empty string if it was not
 * fouind
 */
std::string
CreateTransmissionWorkspace2::getRunNumber(std::string const &propertyName) {
  auto runNumber = std::string();
  MatrixWorkspace_sptr transWS = getProperty(propertyName);
  if (transWS) {
    auto const &run = transWS->run();
    if (run.hasProperty("run_number")) {
      runNumber = run.getPropertyValueAsType<std::string>("run_number");
    } else {
      m_missingRunNumber = true;
    }
  }
  return runNumber;
}

/** Output an interim transmission run if in debug mode or if running as a
 * child algorithm. Note that the workspace will only be output if a sensible
 * name can be constructed, which requires the workspace to have a run number.
 * @param which Which of the runs to store: 1 - first, 2 - second.
 * @param ws A workspace to store.
 */
void CreateTransmissionWorkspace2::setOutputTransmissionRun(
    int which, const MatrixWorkspace_sptr &ws) {
  bool const isDebug = getProperty("Debug");
  if (!isDebug)
    return;

  if (which < 1 || which > 2) {
    throw std::logic_error("There are only two runs: 1 and 2.");
  }

  // Set the output property
  auto const runDescription =
      which == 1 ? "FirstTransmission" : "SecondTransmission";
  auto const propertyName = std::string("OutputWorkspace") + runDescription;

  // If the user provided an output name, just set the value
  if (!isDefault(propertyName)) {
    setProperty(propertyName, ws);
    return;
  }

  // Otherwise try to set a default name based on the run number
  auto const &runNumber =
      which == 1 ? m_firstTransmissionRunNumber : m_secondTransmissionRunNumber;
  if (runNumber.empty()) {
    throw std::runtime_error(
        std::string("Input workspace has no run number; cannot set default "
                    "name for the "
                    "output workspace. Please specify a name using the ") +
        propertyName + std::string(" property."));
  }

  auto const defaultName = TRANS_LAM_PREFIX + runNumber;
  setPropertyValue(propertyName, defaultName);
  setProperty(propertyName, ws);
}

/** Output the final transmission workspace
 * @param ws A workspace to store.
 * @throws If the output workspace does not have a name and a default could not
 * be found
 */
void CreateTransmissionWorkspace2::setOutputWorkspace(
    const API::MatrixWorkspace_sptr &ws) {
  // If the user provided an output name, just set the value
  if (!isDefault("OutputWorkspace")) {
    setProperty("OutputWorkspace", ws);
    return;
  }

  // Otherwise, we want to set a default name based on the run number
  if (m_missingRunNumber) {
    if (isChild()) {
      setProperty("OutputWorkspace", ws);
      return;
    } else {
      throw std::runtime_error(
          "Input workspace has no run number; cannot set default name for the "
          "output workspace. Please specify a name using the OutputWorkspace "
          "property.");
    }
  }

  std::string outputName = TRANS_LAM_PREFIX;
  if (!m_firstTransmissionRunNumber.empty()) {
    outputName.append(m_firstTransmissionRunNumber);
  }
  if (!m_secondTransmissionRunNumber.empty()) {
    outputName.append("_");
    outputName.append(m_secondTransmissionRunNumber);
  }

  setPropertyValue("OutputWorkspace", outputName);
  setProperty("OutputWorkspace", ws);
}
} // namespace Reflectometry
} // namespace Mantid
