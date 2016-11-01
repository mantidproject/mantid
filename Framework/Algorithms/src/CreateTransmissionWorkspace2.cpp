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

/** Normalize detectors by monitors
* @param IvsLam :: a workspace in wavelength that contains spectra for both
* monitors and detectors
* @return :: the normalized workspace
*/
MatrixWorkspace_sptr CreateTransmissionWorkspace2::normalizeDetectorsByMonitors(
    MatrixWorkspace_sptr IvsLam) {

  // Detector workspace
  MatrixWorkspace_sptr detectorWS = makeDetectorWS(IvsLam);

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
  // Only if both MonitorIntegrationWavelengthMin and
  // MonitorIntegrationWavelengthMax are have been given

  Property *intMinProperty = getProperty("MonitorIntegrationWavelengthMin");
  Property *intMaxProperty = getProperty("MonitorIntegrationWavelengthMax");
  bool integratedMonitors =
      (intMinProperty->isDefault() || intMaxProperty->isDefault()) ? false
                                                                   : true;
  auto monitorWS = makeMonitorWS(IvsLam, integratedMonitors);

  return divide(detectorWS, monitorWS);
}

} // namespace Algorithms
} // namespace Mantid
