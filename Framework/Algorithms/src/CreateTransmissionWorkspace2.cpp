// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateTransmissionWorkspace2.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {

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
  auto inputValidator = boost::make_shared<WorkspaceUnitValidator>("TOF");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "FirstTransmissionRun", "", Direction::Input,
                      PropertyMode::Mandatory, inputValidator->clone()),
                  "First transmission run. Corresponds to the low wavelength "
                  "transmision run if a SecondTransmissionRun is also "
                  "provided.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional, inputValidator->clone()),
                  "High wavelength transmission run. Optional. Causes the "
                  "first transmission run to be treated as the low wavelength "
                  "transmission run.");

  declareProperty(Kernel::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "",
                      boost::make_shared<MandatoryValidator<std::string>>(),
                      Direction::Input),
                  "Grouping pattern on spectrum numbers to yield only "
                  "the detectors of interest. See GroupDetectors for details.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "WavelengthMin", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength minimum in angstroms");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "WavelengthMax", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength maximum in angstroms");

  initMonitorProperties();

  initStitchProperties();

  declareProperty("Debug", false,
                  "Whether to enable the output of extra workspaces.");

  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output, PropertyMode::Optional),
      "Output workspace in wavelength.");
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

  MatrixWorkspace_sptr outWS;

  MatrixWorkspace_sptr firstTransWS = getProperty("FirstTransmissionRun");
  convertProcessingInstructions(firstTransWS);

  firstTransWS = normalizeDetectorsByMonitors(firstTransWS);
  firstTransWS = cropWavelength(firstTransWS);

  MatrixWorkspace_sptr secondTransWS = getProperty("SecondTransmissionRun");
  if (secondTransWS) {
    storeTransitionRun(1, firstTransWS);

    convertProcessingInstructions(secondTransWS);

    secondTransWS = normalizeDetectorsByMonitors(secondTransWS);
    secondTransWS = cropWavelength(secondTransWS);
    storeTransitionRun(2, secondTransWS);

    // Stitch the results.
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
    outWS = stitch->getProperty("OutputWorkspace");
  } else {
    outWS = firstTransWS;
  }
  storeOutputWorkspace(outWS);
}

/** Normalize detectors by monitors
 * @param IvsTOF :: a workspace in TOF that contains spectra for both
 * monitors and detectors
 * @return :: the normalized workspace in Wavelength
 */
MatrixWorkspace_sptr CreateTransmissionWorkspace2::normalizeDetectorsByMonitors(
    const MatrixWorkspace_sptr IvsTOF) {

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
  MatrixWorkspace_sptr firstTransWS = getProperty("FirstTransmissionRun");
  auto const &run = firstTransWS->run();
  if (run.hasProperty("run_number")) {
    m_firstTransmissionRunNumber =
        run.getPropertyValueAsType<std::string>("run_number");
  }

  MatrixWorkspace_sptr secondTransWS = getProperty("SecondTransmissionRun");
  if (secondTransWS && secondTransWS->run().hasProperty("run_number")) {
    m_secondTransmissionRunNumber =
        secondTransWS->run().getPropertyValueAsType<std::string>("run_number");
  }
}

/** Store a transition run in ADS
 * @param which Which of the runs to store: 1 - first, 2 - second.
 * @param ws A workspace to store.
 */
void CreateTransmissionWorkspace2::storeTransitionRun(int which,
                                                      MatrixWorkspace_sptr ws) {
  if (which < 1 || which > 2) {
    throw std::logic_error("There are only two runs: 1 and 2.");
  }
  auto const &runNumber =
      which == 1 ? m_firstTransmissionRunNumber : m_secondTransmissionRunNumber;

  if (!runNumber.empty()) {
    auto const name = TRANS_LAM_PREFIX + runNumber;
    AnalysisDataService::Instance().addOrReplace(name, ws);
  }
}

/** Store the stitched transition workspace run in ADS
 * @param ws A workspace to store.
 */
void CreateTransmissionWorkspace2::storeOutputWorkspace(
    API::MatrixWorkspace_sptr ws) {
  bool const isDebug = getProperty("Debug");
  if (isDefault("OutputWorkspace") && (!isChild() || isDebug)) {
    std::string name = TRANS_LAM_PREFIX;
    if (!m_firstTransmissionRunNumber.empty()) {
      name.append(m_firstTransmissionRunNumber);
    } else {
      return;
    }
    if (!m_secondTransmissionRunNumber.empty()) {
      name.append("_");
      name.append(m_secondTransmissionRunNumber);
    }
    if (!isChild()) {
      setPropertyValue("OutputWorkspace", name);
    } else {
      AnalysisDataService::Instance().addOrReplace(name, ws);
    }
  }
  setProperty("OutputWorkspace", ws);
}

} // namespace Algorithms
} // namespace Mantid
