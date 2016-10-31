#include "MantidAlgorithms/CreateTransmissionWorkspace2.h"
#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"

#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateTransmissionWorkspace2)

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
                  "Grouping pattern on workspace indexes to yield only "
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

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output workspace in wavelength.");
}

/** Initialize properties related to monitors
*/
void CreateTransmissionWorkspace2::initMonitorProperties() {

  // Monitor workspace index
  declareProperty(make_unique<PropertyWithValue<int>>(
                      "I0MonitorIndex", Mantid::EMPTY_INT(), Direction::Input),
                  "I0 monitor workspace index");

  // Minimum wavelength for background subtraction
  declareProperty(
      make_unique<PropertyWithValue<double>>("MonitorBackgroundWavelengthMin",
                                             Mantid::EMPTY_DBL(),
                                             Direction::Input),
      "Wavelength minimum for monitor background subtraction in angstroms.");
  // Maximum wavelength for background subtraction
  declareProperty(
      make_unique<PropertyWithValue<double>>("MonitorBackgroundWavelengthMax",
                                             Mantid::EMPTY_DBL(),
                                             Direction::Input),
      "Wavelength maximum for monitor background subtraction in angstroms.");

  // Minimum wavelength for monitor integration
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength minimum for integration in angstroms.");
  // Maximum wavelength for monitor integration
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength maximum for integration in angstroms.");
}

/** Initialize properties used for stitching transmission runs
*/
void CreateTransmissionWorkspace2::initStitchProperties() {

  declareProperty(
      make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "These parameters are used for stitching together transmission runs. "
      "Values are in wavelength (angstroms). This input is only needed if a "
      "SecondTransmission run is provided.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start wavelength for stitching transmission runs together. "
                  "Only used if a second transmission run is provided.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "EndOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "End wavelength (angstroms) for stitching transmission runs "
                  "together. Only used if a second transmission run is "
                  "provided.");
}

/** Validate inputs
* @return :: error message to show
*/
std::map<std::string, std::string>
CreateTransmissionWorkspace2::validateInputs() {

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

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateTransmissionWorkspace2::exec() {

  MatrixWorkspace_sptr firstTransWS = getProperty("FirstTransmissionRun");
  firstTransWS = convertToWavelength(firstTransWS);
  firstTransWS = normalizeDetectorsByMonitors(firstTransWS);

  MatrixWorkspace_sptr secondTransWS = getProperty("SecondTransmissionRun");
  if (secondTransWS) {
    secondTransWS = convertToWavelength(secondTransWS);
    secondTransWS = normalizeDetectorsByMonitors(secondTransWS);

    // Stitch the results.
    auto stitch = createChildAlgorithm("Stitch1D");
    stitch->initialize();
    stitch->setProperty("LHSWorkspace", firstTransWS);
    stitch->setProperty("RHSWorkspace", secondTransWS);
    stitch->setPropertyValue("StartOverlap", getPropertyValue("StartOverlap"));
    stitch->setPropertyValue("EndOverlap", getPropertyValue("EndOverlap"));
    stitch->setPropertyValue("Params", getPropertyValue("Params"));
    stitch->execute();
    MatrixWorkspace_sptr outWS = stitch->getProperty("OutputWorkspace");
    setProperty("OutputWorkspace", outWS);
  } else {
    setProperty("OutputWorkspace", firstTransWS);
  }
}

/** Converts an input workspace in TOF to wavelength and crops to specified
* limits.
* @param inputWS :: the workspace to convert
* @return :: the workspace in wavelength
*/
MatrixWorkspace_sptr CreateTransmissionWorkspace2::convertToWavelength(
    MatrixWorkspace_sptr inputWS) {

  auto convertUnitsAlg = this->createChildAlgorithm("ConvertUnits");
  convertUnitsAlg->initialize();
  convertUnitsAlg->setProperty("InputWorkspace", inputWS);
  convertUnitsAlg->setProperty("Target", "Wavelength");
  convertUnitsAlg->setProperty("AlignBins", true);
  convertUnitsAlg->execute();
  MatrixWorkspace_sptr outputWS =
      convertUnitsAlg->getProperty("OutputWorkspace");

  // Crop out the lambda x-ranges now that the workspace is in wavelength.
  double wavelengthMin = getProperty("WavelengthMin");
  double wavelengthMax = getProperty("WavelengthMax");

  auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", outputWS);
  cropWorkspaceAlg->setProperty("XMin", wavelengthMin);
  cropWorkspaceAlg->setProperty("XMax", wavelengthMax);
  cropWorkspaceAlg->execute();
  outputWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

  return outputWS;
}

/** Normalize detectors by monitors
* @param IvsLam :: a workspace in wavelength that contains spectra for both
* monitors and detectors
* @return :: the normalized workspace
*/
MatrixWorkspace_sptr CreateTransmissionWorkspace2::normalizeDetectorsByMonitors(
    MatrixWorkspace_sptr IvsLam) {

  // Detector workspace

  std::string processingCommands = getPropertyValue("ProcessingInstructions");
  auto groupAlg = createChildAlgorithm("GroupDetectors");
  groupAlg->initialize();
  groupAlg->setProperty("GroupingPattern", processingCommands);
  groupAlg->setProperty("InputWorkspace", IvsLam);
  groupAlg->execute();
  MatrixWorkspace_sptr detectorWS = groupAlg->getProperty("OutputWorkspace");

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

  // Extract the monitor workspace
  int monitorIndex = getProperty("I0MonitorIndex");
  auto cropWorkspaceAlg = createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", IvsLam);
  cropWorkspaceAlg->setProperty("StartWorkspaceIndex", monitorIndex);
  cropWorkspaceAlg->setProperty("EndWorkspaceIndex", monitorIndex);
  cropWorkspaceAlg->execute();
  MatrixWorkspace_sptr monitorWS =
      cropWorkspaceAlg->getProperty("OutputWorkspace");

  // Flat background correction
  double backgroundMin = getProperty("MonitorBackgroundWavelengthMin");
  double backgroundMax = getProperty("MonitorBackgroundWavelengthMax");
  auto correctMonitorsAlg =
      this->createChildAlgorithm("CalculateFlatBackground");
  correctMonitorsAlg->initialize();
  correctMonitorsAlg->setProperty("InputWorkspace", monitorWS);
  correctMonitorsAlg->setProperty("StartX", backgroundMin);
  correctMonitorsAlg->setProperty("EndX", backgroundMax);
  correctMonitorsAlg->setProperty("SkipMonitors", false);
  correctMonitorsAlg->execute();
  monitorWS = correctMonitorsAlg->getProperty("OutputWorkspace");

  // Normalization by integrated monitors
  // Only if MonitorIntegrationWavelengthMin and MonitorIntegrationWavelengthMax
  // have been given

  Property *intMinProperty = getProperty("MonitorIntegrationWavelengthMin");
  Property *intMaxProperty = getProperty("MonitorIntegrationWavelengthMax");
  if (intMinProperty->isDefault() || intMaxProperty->isDefault())
    return divide(detectorWS, monitorWS);

  auto integrationAlg = createChildAlgorithm("Integration");
  integrationAlg->initialize();
  integrationAlg->setProperty("InputWorkspace", monitorWS);
  integrationAlg->setPropertyValue("RangeLower", intMinProperty->value());
  integrationAlg->setPropertyValue("RangeUpper", intMaxProperty->value());
  integrationAlg->execute();
  monitorWS = integrationAlg->getProperty("OutputWorkspace");

  return divide(detectorWS, monitorWS);
}

} // namespace Algorithms
} // namespace Mantid
