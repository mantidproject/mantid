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

/*Anonomous namespace */
namespace {

/**
* Translate all the workspace indexes in an origin workspace into workspace
* indexes of a host end-point workspace. This is done using spectrum numbers as
* the intermediate.
*
* @param originWS : Origin workspace, which provides the original workspace
* index to spectrum number mapping.
* @param hostWS : Workspace onto which the resulting workspace indexes will be
* hosted
* @throws :: If the specId are not found to exist on the host end-point
*workspace.
* @return :: Remapped workspace indexes applicable for the host workspace.
*results
*as comma separated string.
*/
std::string
createProcessingCommandsFromDetectorWS(MatrixWorkspace_const_sptr originWS,
                                       MatrixWorkspace_const_sptr hostWS) {
  auto spectrumMap = originWS->getSpectrumToWorkspaceIndexMap();
  auto it = spectrumMap.begin();
  std::stringstream result;
  specnum_t specId = (*it).first;
  result << static_cast<int>(hostWS->getIndexFromSpectrumNumber(specId));
  ++it;
  for (; it != spectrumMap.end(); ++it) {
    specId = (*it).first;
    result << ","
           << static_cast<int>(hostWS->getIndexFromSpectrumNumber(specId));
  }
  return result.str();
}

/**
@param ws1 : First workspace to compare
@param ws2 : Second workspace to compare against
@param severe: True to indicate that failure to verify should result in an
exception. Otherwise a warning is generated.
@return : true if spectrum maps match. False otherwise
*/
bool verifySpectrumMaps(MatrixWorkspace_const_sptr ws1,
                        MatrixWorkspace_const_sptr ws2) {
  auto map1 = ws1->getSpectrumToWorkspaceIndexMap();
  auto map2 = ws2->getSpectrumToWorkspaceIndexMap();
  if (map1 != map2) {
    return false;
  } else {
    return true;
  }
}
}

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

  // Init properties for transmission normalization
  initTransmissionProperties();

  // Init properties for algorithmic corrections
  initAlgorithmicProperties();

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output Workspace IvsQ.");

  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspaceWavelength",
                                                   "", Direction::Output,
                                                   PropertyMode::Optional),
                  "Output Workspace IvsLam. Intermediate workspace.");

  initMomentumTransferProperties();
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
  declareProperty("NormalizeByIntegratedMonitors", true,
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

/** Initialize properties related to transmission normalization
*/
void ReflectometryReductionOne2::initTransmissionProperties() {

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

  declareProperty(
      make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "These parameters are used for stitching together transmission runs. "
      "Values are in wavelength (angstroms). This input is only needed if a "
      "second transmission run is provided.");

  declareProperty(make_unique<PropertyWithValue<double>>(
                      "StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start wavelength for stitching transmission runs together. "
                  "This parameter is only used if a second transmission run "
                  "is provided.");

  declareProperty(
      make_unique<PropertyWithValue<double>>("EndOverlap", Mantid::EMPTY_DBL(),
                                             Direction::Input),
      "End wavelength (angstroms) for stitching transmission runs together. "
      "This parameter is only used if a second transmission run is provided.");

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

/** Initialize algorithmic correction properties
*/
void ReflectometryReductionOne2::initAlgorithmicProperties() {

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
void ReflectometryReductionOne2::initMomentumTransferProperties() {

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

  // Validate transmission runs if given
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
    Property *monProperty = getProperty("I0MonitorIndex");
    Property *backgroundMinProperty =
        getProperty("MonitorBackgroundWavelengthMin");
    Property *backgroundMaxProperty =
        getProperty("MonitorBackgroundWavelengthMin");
    if (!monProperty->isDefault() && !backgroundMinProperty->isDefault() &&
        !backgroundMaxProperty->isDefault()) {
      auto monitorWS = makeMonitorWS(IvsLam);
      IvsLam = divide(detectorWS, monitorWS);
    } else {
      IvsLam = detectorWS;
    }
  }

  // Transmission correction
  MatrixWorkspace_sptr transRun = getProperty("FirstTransmissionRun");
  if (transRun) {
    IvsLam = transmissionCorrection(IvsLam);
  } else if (getPropertyValue("CorrectionAlgorithm") != "None") {
    IvsLam = algorithmicCorrection(IvsLam);
  } else {
    g_log.warning("No transmission correction will be applied.");
  }

  // Convert to Q
  auto IvsQ = convertToQ(IvsLam);

  setProperty("OutputWorkspaceWavelength", IvsLam);
  setProperty("OutputWorkspace", IvsQ);
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
MatrixWorkspace_sptr
ReflectometryReductionOne2::makeDetectorWS(MatrixWorkspace_sptr inputWS) {

  std::string processingCommands = getPropertyValue("ProcessingInstructions");
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

  auto groupDirectBeamAlg = this->createChildAlgorithm("GroupDetectors");
  groupDirectBeamAlg->initialize();
  groupDirectBeamAlg->setProperty("GroupingPattern", processingCommands);
  groupDirectBeamAlg->setProperty("InputWorkspace", inputWS);
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

/** Perform transmission correction by running 'CreateTransmissionWorkspace' on
* the input workspace
* @param detectorWS :: the input workspace
* @return :: the input workspace normalized by transmission
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::transmissionCorrection(
    MatrixWorkspace_sptr detectorWS) {

  const bool strictSpectrumChecking = getProperty("StrictSpectrumChecking");

  MatrixWorkspace_sptr transmissionWS = getProperty("FirstTransmissionRun");
  Unit_const_sptr xUnit = transmissionWS->getAxis(0)->unit();

  if (xUnit->unitID() == "TOF") {

    // Processing instructions for transmission workspace
    std::string transmissionCommands = getProperty("ProcessingInstructions");
    if (strictSpectrumChecking) {
      // If we have strict spectrum checking, the processing commands need to be
      // made from the
      // numerator workspace AND the transmission workspace based on matching
      // spectrum numbers.
      transmissionCommands =
          createProcessingCommandsFromDetectorWS(detectorWS, transmissionWS);
    }

    MatrixWorkspace_sptr secondTransmissionWS =
        getProperty("SecondTransmissionRun");
    auto alg = this->createChildAlgorithm("CreateTransmissionWorkspace");
    alg->initialize();
    alg->setProperty("FirstTransmissionRun", transmissionWS);
    alg->setProperty("SecondTransmissionRun", secondTransmissionWS);
    alg->setPropertyValue("Params", getPropertyValue("Params"));
    alg->setPropertyValue("StartOverlap", getPropertyValue("StartOverlap"));
    alg->setPropertyValue("EndOverlap", getPropertyValue("EndOverlap"));
    alg->setPropertyValue("I0MonitorIndex", getPropertyValue("I0MonitorIndex"));
    alg->setPropertyValue("WavelengthMin", getPropertyValue("WavelengthMin"));
    alg->setPropertyValue("WavelengthMax", getPropertyValue("WavelengthMax"));
    alg->setPropertyValue("MonitorBackgroundWavelengthMin",
                          getPropertyValue("MonitorBackgroundWavelengthMin"));
    alg->setPropertyValue("MonitorBackgroundWavelengthMax",
                          getPropertyValue("MonitorBackgroundWavelengthMax"));
    alg->setPropertyValue("MonitorIntegrationWavelengthMin",
                          getPropertyValue("MonitorIntegrationWavelengthMin"));
    alg->setPropertyValue("MonitorIntegrationWavelengthMax",
                          getPropertyValue("MonitorIntegrationWavelengthMax"));
    alg->setProperty("ProcessingInstructions", transmissionCommands);
    alg->execute();
    transmissionWS = alg->getProperty("OutputWorkspace");
  }

  // Rebin the transmission run to be the same as the input.
  auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
  rebinToWorkspaceAlg->initialize();
  rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", detectorWS);
  rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", transmissionWS);
  rebinToWorkspaceAlg->execute();
  transmissionWS = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

  const bool match = verifySpectrumMaps(detectorWS, transmissionWS);
  if (!match) {
    std::string message = "Spectrum maps between workspaces do NOT match up.";
    if (strictSpectrumChecking) {
      throw std::invalid_argument(message);
    } else {
      g_log.warning(message);
    }
  }

  MatrixWorkspace_sptr normalized = divide(detectorWS, transmissionWS);
  return normalized;
}

/**
* Perform transmission correction using alternative correction algorithms
* @param detectorWS : workspace in wavelength which is to be normalized by the
* results of the transmission corrections.
* @return : corrected workspace
*/
MatrixWorkspace_sptr ReflectometryReductionOne2::algorithmicCorrection(
    MatrixWorkspace_sptr detectorWS) {

  const std::string corrAlgName = getProperty("CorrectionAlgorithm");

  IAlgorithm_sptr corrAlg = createChildAlgorithm(corrAlgName);
  corrAlg->initialize();
  if (corrAlgName == "PolynomialCorrection") {
    corrAlg->setPropertyValue("Coefficients", getPropertyValue("Polynomial"));
  } else if (corrAlgName == "ExponentialCorrection") {
    corrAlg->setPropertyValue("C0", getPropertyValue("C0"));
    corrAlg->setPropertyValue("C1", getPropertyValue("C1"));
  } else {
    throw std::runtime_error("Unknown correction algorithm: " + corrAlgName);
  }

  corrAlg->setProperty("InputWorkspace", detectorWS);
  corrAlg->setProperty("Operation", "Divide");
  corrAlg->execute();

  return corrAlg->getProperty("OutputWorkspace");
}

/**
* The input workspace (in wavelength) to convert to Q
* @param inputWS : the input workspace to convert
* @return : output workspace in Q
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::convertToQ(MatrixWorkspace_sptr inputWS) {

  // Convert to Q
  auto convertUnits = this->createChildAlgorithm("ConvertUnits");
  convertUnits->initialize();
  convertUnits->setProperty("InputWorkspace", inputWS);
  convertUnits->setProperty("Target", "MomentumTransfer");
  convertUnits->setProperty("AlignBins", true);
  convertUnits->execute();
  MatrixWorkspace_sptr IvsQ = convertUnits->getProperty("OutputWorkspace");

  // Rebin (optional)
  Property *qStepProp = getProperty("MomentumTransferStep");
  if (!qStepProp->isDefault()) {
    double qstep = getProperty("MomentumTransferStep");
    qstep = -qstep;

    std::vector<double> qparams;
    Property *qMin = getProperty("MomentumTransferMin");
    Property *qMax = getProperty("MomentumTransferMax");
    if (!qMin->isDefault() && !qMax->isDefault()) {
      double qmin = getProperty("MomentumTransferMin");
      double qmax = getProperty("MomentumTransferMax");
      qparams.push_back(qmin);
      qparams.push_back(qstep);
      qparams.push_back(qmax);
    } else {
      g_log.information("MomentumTransferMin and MomentumTransferMax will not "
                        "be used to regin IvsQ workspace");
      qparams.push_back(qstep);
    }
    IAlgorithm_sptr algRebin = createChildAlgorithm("Rebin");
    algRebin->initialize();
    algRebin->setProperty("InputWorkspace", IvsQ);
    algRebin->setProperty("OutputWorkspace", IvsQ);
    algRebin->setProperty("Params", qparams);
    algRebin->execute();
    IvsQ = algRebin->getProperty("OutputWorkspace");
  }

  // Scale (optional)
  Property *scaleProp = getProperty("ScaleFactor");
  if (!scaleProp->isDefault()) {
    double scaleFactor = getProperty("ScaleFactor");
    IAlgorithm_sptr algScale = createChildAlgorithm("Scale");
    algScale->initialize();
    algScale->setProperty("InputWorkspace", IvsQ);
    algScale->setProperty("OutputWorkspace", IvsQ);
    algScale->setProperty("Factor", 1.0 / scaleFactor);
    algScale->execute();
    IvsQ = algScale->getProperty("OutputWorkspace");
  }

  return IvsQ;
}

} // namespace Algorithms
} // namespace Mantid
