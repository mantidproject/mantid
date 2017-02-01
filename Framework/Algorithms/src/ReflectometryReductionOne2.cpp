#include "MantidAlgorithms/ReflectometryReductionOne2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

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
  // Normalization by integrated monitors
  declareProperty("NormalizeByIntegratedMonitors", true,
                  "Normalize by dividing by the integrated monitors.");

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
}

/** Validate inputs
*/
std::map<std::string, std::string>
ReflectometryReductionOne2::validateInputs() {

  std::map<std::string, std::string> results;

  const auto wavelength = validateWavelengthRanges();
  results.insert(wavelength.begin(), wavelength.end());

  const auto directBeam = validateDirectBeamProperties();
  results.insert(directBeam.begin(), directBeam.end());

  const auto transmission = validateTransmissionProperties();
  results.insert(transmission.begin(), transmission.end());

  return results;
}

/** Execute the algorithm.
*/
void ReflectometryReductionOne2::exec() {
  MatrixWorkspace_sptr runWS = getProperty("InputWorkspace");

  const auto xUnitID = runWS->getAxis(0)->unit()->unitID();

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

    // Detector workspace
    auto detectorWS = makeDetectorWS(runWS);

    // Normalization by direct beam (optional)
    Property *directBeamProperty = getProperty("RegionOfDirectBeam");
    if (!directBeamProperty->isDefault()) {
      const auto directBeam = makeDirectBeamWS(runWS);
      detectorWS = divide(detectorWS, directBeam);
    }

    // Monitor workspace (only if I0MonitorIndex, MonitorBackgroundWavelengthMin
    // and MonitorBackgroundWavelengthMax have been given)
    Property *monProperty = getProperty("I0MonitorIndex");
    Property *backgroundMinProperty =
        getProperty("MonitorBackgroundWavelengthMin");
    Property *backgroundMaxProperty =
        getProperty("MonitorBackgroundWavelengthMin");
    if (!monProperty->isDefault() && !backgroundMinProperty->isDefault() &&
        !backgroundMaxProperty->isDefault()) {
      const bool integratedMonitors =
          getProperty("NormalizeByIntegratedMonitors");
      const auto monitorWS = makeMonitorWS(runWS, integratedMonitors);
      if (!integratedMonitors)
        detectorWS = rebinDetectorsToMonitors(detectorWS, monitorWS);
      IvsLam = divide(detectorWS, monitorWS);
    } else {
      IvsLam = detectorWS;
    }

    // Crop to wavelength limits
    IvsLam = cropWavelength(IvsLam);
  }

  // Transmission correction
  MatrixWorkspace_sptr transRun = getProperty("FirstTransmissionRun");
  if (transRun) {
    IvsLam = transmissionCorrection(IvsLam);
  } else if (getPropertyValue("CorrectionAlgorithm") != "None") {
    IvsLam = algorithmicCorrection(IvsLam);
  }

  // Convert to Q
  auto IvsQ = convertToQ(IvsLam);

  setProperty("OutputWorkspaceWavelength", IvsLam);
  setProperty("OutputWorkspace", IvsQ);
}

/** Creates a direct beam workspace in wavelength from an input workspace in
* TOF. This method should only be called if RegionOfDirectBeam is provided.
*
* @param inputWS :: the input workspace in TOF
* @return :: the direct beam workspace in wavelength
*/
MatrixWorkspace_sptr
ReflectometryReductionOne2::makeDirectBeamWS(MatrixWorkspace_sptr inputWS) {

  std::vector<int> directBeamRegion = getProperty("RegionOfDirectBeam");
  // Sum over the direct beam.
  const std::string processingCommands = std::to_string(directBeamRegion[0]) +
                                         "-" +
                                         std::to_string(directBeamRegion[1]);

  auto groupDirectBeamAlg = this->createChildAlgorithm("GroupDetectors");
  groupDirectBeamAlg->initialize();
  groupDirectBeamAlg->setProperty("GroupingPattern", processingCommands);
  groupDirectBeamAlg->setProperty("InputWorkspace", inputWS);
  groupDirectBeamAlg->execute();
  MatrixWorkspace_sptr directBeamWS =
      groupDirectBeamAlg->getProperty("OutputWorkspace");

  directBeamWS = convertToWavelength(directBeamWS);

  return directBeamWS;
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
    const std::string message =
        "Spectrum maps between workspaces do NOT match up.";
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
  convertUnits->setProperty("AlignBins", false);
  convertUnits->execute();
  MatrixWorkspace_sptr IvsQ = convertUnits->getProperty("OutputWorkspace");

  return IvsQ;
}

} // namespace Algorithms
} // namespace Mantid
