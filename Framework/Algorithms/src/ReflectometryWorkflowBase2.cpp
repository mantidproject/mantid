#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

/** Initialize properties related to direct beam normalization
*/
void ReflectometryWorkflowBase2::initDirectBeamProperties() {

  declareProperty(make_unique<ArrayProperty<int>>("RegionOfDirectBeam"),
                  "Indices of the spectra a pair (lower, upper) that mark the "
                  "ranges that correspond to the direct beam in multi-detector "
                  "mode.");
}

/** Initialize properties related to monitors
*/
void ReflectometryWorkflowBase2::initMonitorProperties() {

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

/** Initialize properties related to transmission normalization
*/
void ReflectometryWorkflowBase2::initTransmissionProperties() {

  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "FirstTransmissionRun", "", Direction::Input, PropertyMode::Optional),
      "First transmission run, or the low wavelength transmission run if "
      "SecondTransmissionRun is also provided.");

  auto inputValidator = boost::make_shared<WorkspaceUnitValidator>("TOF");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional, inputValidator),
                  "Second, high wavelength transmission run. Optional. Causes "
                  "the FirstTransmissionRun to be treated as the low "
                  "wavelength transmission run.");

  initStitchProperties();

  declareProperty(make_unique<PropertyWithValue<bool>>("StrictSpectrumChecking",
                                                       true, Direction::Input),
                  "Enforces spectrum number checking prior to normalization by "
                  "transmission workspace. Applies to input workspace and "
                  "transmission workspace.");

  setPropertyGroup("FirstTransmissionRun", "Transmission");
  setPropertyGroup("SecondTransmissionRun", "Transmission");
  setPropertyGroup("Params", "Transmission");
  setPropertyGroup("StartOverlap", "Transmission");
  setPropertyGroup("EndOverlap", "Transmission");
  setPropertyGroup("StrictSpectrumChecking", "Transmission");
}

/** Initialize properties used for stitching transmission runs
*/
void ReflectometryWorkflowBase2::initStitchProperties() {

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

/** Initialize algorithmic correction properties
*/
void ReflectometryWorkflowBase2::initAlgorithmicProperties() {

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

/** Validate direct beam if given
*
* @return :: A map with results of validation
*/
std::map<std::string, std::string>
ReflectometryWorkflowBase2::validateDirectBeamProperties() {

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

/** Validate transmission runs if given
*
* @return :: A map with results of validation
*/
std::map<std::string, std::string>
ReflectometryWorkflowBase2::validateTransmissionProperties() {

  std::map<std::string, std::string> results;

  MatrixWorkspace_sptr firstTransmissionRun =
      getProperty("FirstTransmissionRun");
  if (firstTransmissionRun) {
    auto xUnitFirst = firstTransmissionRun->getAxis(0)->unit()->unitID();
    if (xUnitFirst != "TOF" && xUnitFirst != "Wavelength") {
      results["FirstTransmissionRun"] =
          "First transmission run must be in TOF or wavelength";
    }
    MatrixWorkspace_sptr secondTransmissionRun =
        getProperty("SecondTransmissionRun");
    if (secondTransmissionRun) {
      auto xUnitSecond = secondTransmissionRun->getAxis(0)->unit()->unitID();
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

/** Converts an input workspace in TOF to wavelength and crops to specified
* limits.
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

  // Crop out the lambda x-ranges now that the workspace is in wavelength.
  double wavelengthMin = getProperty("WavelengthMin");
  double wavelengthMax = getProperty("WavelengthMax");

  auto cropWorkspaceAlg = createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", outputWS);
  cropWorkspaceAlg->setProperty("XMin", wavelengthMin);
  cropWorkspaceAlg->setProperty("XMax", wavelengthMax);
  cropWorkspaceAlg->execute();
  outputWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

  return outputWS;
}

/** Process an input workspace according to specified processing commands to get
* a detector workspace.
* @param inputWS :: the input workspace in wavelength
* @return :: the detector workspace
*/
MatrixWorkspace_sptr
ReflectometryWorkflowBase2::makeDetectorWS(MatrixWorkspace_sptr inputWS) {

  std::string processingCommands = getPropertyValue("ProcessingInstructions");
  auto groupAlg = createChildAlgorithm("GroupDetectors");
  groupAlg->initialize();
  groupAlg->setProperty("GroupingPattern", processingCommands);
  groupAlg->setProperty("InputWorkspace", inputWS);
  groupAlg->execute();
  MatrixWorkspace_sptr detectorWS = groupAlg->getProperty("OutputWorkspace");

  return detectorWS;
}

/** Creates a monitor workspace. This method should only be called if
* IOMonitorIndex has been specified and MonitorBackgroundWavelengthMin and
* MonitorBackgroundWavelengthMax have been given.
* @param inputWS :: the input workspace in wavelength
* @return :: the monitor workspace
*/
MatrixWorkspace_sptr
ReflectometryWorkflowBase2::makeMonitorWS(MatrixWorkspace_sptr inputWS,
                                          bool integratedMonitors) {

  // Extract the monitor workspace
  int monitorIndex = getProperty("I0MonitorIndex");
  auto cropWorkspaceAlg = createChildAlgorithm("CropWorkspace");
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

} // namespace Algorithms
} // namespace Mantid
