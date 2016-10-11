#include "MantidAlgorithms/ReflectometryReductionOne2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Unit.h"



#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {


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

  // Incident theta
  declareProperty(make_unique<PropertyWithValue<double>>(
                      "ThetaIn", Mantid::EMPTY_DBL(), Direction::Input),
                  "Final theta value in degrees. Optional, this value will be "
                  "calculated internally and provided as ThetaOut if not "
                  "provided.");

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

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceWavelength",
                                                   "", Direction::Output,
                                                   PropertyMode::Optional),
                  "Output Workspace IvsLam. Intermediate workspace.");
}

/** Initialize properties related to monitors
*/
void ReflectometryReductionOne2::initMonitorProperties() {

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

  // Normalization by integrated monitors
  declareProperty("NormalizeByIntegratedMonitors", false,
                  "Normalize by dividing by the integrated monitors.");

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

/** Initialize properties related to direct beam normalization
*/
void ReflectometryReductionOne2::initDirectBeamProperties() {

  declareProperty(make_unique<ArrayProperty<int>>("RegionOfDirectBeam"),
                  "Indices of the spectra a pair (lower, upper) that mark the "
                  "ranges that correspond to the direct beam in multi-detector "
                  "mode.");
}

/** Validate inputs
*/
std::map<std::string, std::string>
ReflectometryReductionOne2::validateInputs() {

  std::map<std::string, std::string> results;

  // Validate direct beam if given
  Property *directBeamProperty = getProperty("RegionOfDirectBeam");
  if (!directBeamProperty->isDefault()) {

    std::vector<int> directBeamRegion = getProperty("RegionOfDirectBeam");
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

/** Execute the algorithm.
*/
void ReflectometryReductionOne2::exec() {
  MatrixWorkspace_sptr runWS = getProperty("InputWorkspace");

  auto xUnitID = runWS->getAxis(0)->unit()->unitID();

  // Neither TOF or Lambda? Abort.
  if ((xUnitID != "Wavelength") && (xUnitID != "TOF"))
    throw std::invalid_argument(
        "InputWorkspace must have units of TOF or Wavelength");

  // Output workspace in wavelength
  MatrixWorkspace_sptr IvsLam;

  if (xUnitID == "Wavelength") {
    IvsLam = runWS;
  } else {
    // xUnitID == "TOF"

    IvsLam = convertToWavelength(runWS);

    // Detector workspace

    auto detectorWS = makeDetectorWS(IvsLam);

    // Monitor workspace (only if I0MonitorIndex, MonitorBackgroundWavelengthMin
    // and MonitorBackgroundWavelengthMax have been given)

    Property *monProperty = this->getProperty("I0MonitorIndex");
    Property *backgroundMinProperty =
        this->getProperty("MonitorBackgroundWavelengthMin");
    Property *backgroundMaxProperty =
        this->getProperty("MonitorBackgroundWavelengthMin");
    if (!monProperty->isDefault() && !backgroundMinProperty->isDefault() &&
        !backgroundMaxProperty->isDefault()) {
      auto monitorWS = makeMonitorWS(IvsLam);
      IvsLam = divide(detectorWS, monitorWS);
    } else {
      IvsLam = detectorWS;
    }
  }
  setProperty("OutputWorkspaceWavelength", IvsLam);
}

/** Converts an input workspace in TOF to wavelength and crops to specified
 * limits.
 * @param inputWS :: the workspace to convert
 * @return :: the workspace in wavelength
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::convertToWavelength(MatrixWorkspace_sptr inputWS) {

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

/** Process an input workspace according to specified processing commands to get
* a detector workspace and optionally performs direct beam normalization.
* @param inputWS :: the input workspace in wavelength
* @return :: the detector workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::makeDetectorWS(
    MatrixWorkspace_sptr inputWS) {

  std::string processingCommands = getProperty("ProcessingInstructions");
  auto groupAlg = this->createChildAlgorithm("GroupDetectors");
  groupAlg->initialize();
  groupAlg->setProperty("GroupingPattern", processingCommands);
  groupAlg->setProperty("InputWorkspace", inputWS);
  groupAlg->execute();
  MatrixWorkspace_sptr detectorWS = groupAlg->getProperty("OutputWorkspace");

  Property *directBeamProperty = getProperty("RegionOfDirectBeam");
  if (directBeamProperty->isDefault()) {
    return detectorWS;
  }

  std::vector<int> directBeamRegion = getProperty("RegionOfDirectBeam");
  // Sum over the direct beam.
  processingCommands = std::to_string(directBeamRegion[0]) + "-" +
                       std::to_string(directBeamRegion[1]);

  processingCommands = getProperty("ProcessingInstructions");
  auto groupDirectBeamAlg = this->createChildAlgorithm("GroupDetectors");
  groupDirectBeamAlg->initialize();
  groupDirectBeamAlg->setProperty("GroupingPattern", processingCommands);
  groupDirectBeamAlg->setProperty("InputWorkspace", detectorWS);
  groupDirectBeamAlg->execute();
  MatrixWorkspace_sptr directBeamWS =
      groupDirectBeamAlg->getProperty("OutputWorkspace");

  // Perform direct beam normalization
  return divide(detectorWS, directBeamWS);
}

/** Creates a monitor workspace. This method should only be called if
* IOMonitorIndex has been specified and MonitorBackgroundWavelengthMin and
* MonitorBackgroundWavelengthMax have been given.
* @param inputWS :: the input workspace in wavelength
* @return :: the monitor workspace
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::makeMonitorWS(MatrixWorkspace_sptr inputWS) {

  // Extract the monitor workspace
  int monitorIndex = getProperty("I0MonitorIndex");
  auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", inputWS);
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

  // Normalization by integrated monitors ?

  bool normalizeByIntMon = getProperty("NormalizeByIntegratedMonitors");
  if (!normalizeByIntMon) {
    return monitorWS;
  }

  auto integrationAlg = this->createChildAlgorithm("Integration");
  integrationAlg->initialize();
  integrationAlg->setProperty("InputWorkspace", monitorWS);

  Property *integrationMinProperty =
      this->getProperty("MonitorIntegrationWavelengthMin");
  if (!integrationMinProperty->isDefault()) {
    integrationAlg->setProperty("RangeLower", integrationMinProperty->value());
  }

  Property *integrationMaxProperty =
      this->getProperty("MonitorIntegrationWavelengthMax");
  if (!integrationMaxProperty->isDefault()) {
    integrationAlg->setProperty("RangeUpper", integrationMaxProperty->value());
  }
  integrationAlg->execute();
  MatrixWorkspace_sptr integratedMonitor =
      integrationAlg->getProperty("OutputWorkspace");

  return integratedMonitor;
}

} // namespace Algorithms
} // namespace Mantid