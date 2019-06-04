// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ReflectometryWorkflowBase.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {
namespace {

/**
 *  Helper method used with the stl to determine whether values are negative
 * @param value : Value to check
 * @return : True if negative.
 */
bool checkNotPositive(const int value) { return value < 0; }
} // namespace

//----------------------------------------------------------------------------------------------

/**
 * Init index properties.
 */
void ReflectometryWorkflowBase::initIndexInputs() {
  declareProperty(std::make_unique<PropertyWithValue<int>>(
                      "I0MonitorIndex", Mantid::EMPTY_INT(), Direction::Input),
                  "I0 monitor workspace index");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "",
                      boost::make_shared<MandatoryValidator<std::string>>(),
                      Direction::Input),
                  "Grouping pattern on workspace indexes to yield only "
                  "the detectors of interest. See GroupDetectors for details.");
}

/**
 * Init common wavelength inputs.
 */
void ReflectometryWorkflowBase::initWavelengthInputs() {
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "WavelengthMin", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength minimum in angstroms");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "WavelengthMax", Mantid::EMPTY_DBL(),
                      boost::make_shared<MandatoryValidator<double>>(),
                      Direction::Input),
                  "Wavelength maximum in angstroms");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorBackgroundWavelengthMin", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength minimum for monitor background in angstroms.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorBackgroundWavelengthMax", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength maximum for monitor background in angstroms.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMin", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength minimum for integration in angstroms.");
  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "MonitorIntegrationWavelengthMax", Mantid::EMPTY_DBL(),
                      Direction::Input),
                  "Wavelength maximum for integration in angstroms.");
}

/**
 * Init stitching inputs
 */
void ReflectometryWorkflowBase::initStitchingInputs() {
  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "Params", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "These parameters are used for stitching together transmission runs. "
      "Values are in wavelength (angstroms). This input is only needed if a "
      "SecondTransmission run is provided.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "StartOverlap", Mantid::EMPTY_DBL(), Direction::Input),
                  "Start wavelength for stitching transmission runs together");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("EndOverlap", Mantid::EMPTY_DBL(),
                                             Direction::Input),
      "End wavelength (angstroms) for stitching transmission runs together");
}

/**
 * Determine if the property value is the same as the default value.
 * This can be used to determine if the property has not been set.
 * @param propertyName : Name of property to query
 * @return: True only if the property has its default value.
 */
bool ReflectometryWorkflowBase::isPropertyDefault(
    const std::string &propertyName) const {
  Property *property = this->getProperty(propertyName);
  return property->isDefault();
}

/**
 * @return The processing instructions.
 */
const std::string ReflectometryWorkflowBase::getWorkspaceIndexList() const {
  const std::string instructions = getProperty("ProcessingInstructions");
  return instructions;
}

/**
 * Fetch min, max inputs as a vector (int) if they are non-default and set them
 * to the optionalUpperLower object.
 * Performs checks to verify that invalid indexes have not been passed in.
 * @param propertyName : Property name to fetch
 * @param isPointDetector : Flag indicates that the execution is in point
 * detector mode.
 * @param optionalUpperLower : Object to set min and max on.
 */
void ReflectometryWorkflowBase::fetchOptionalLowerUpperPropertyValue(
    const std::string &propertyName, bool isPointDetector,
    OptionalWorkspaceIndexes &optionalUpperLower) const {
  if (!isPropertyDefault(propertyName)) {
    // Validation of property inputs.
    if (isPointDetector) {
      throw std::invalid_argument(
          "Cannot have a region of interest property in point detector mode.");
    }
    std::vector<int> temp = this->getProperty(propertyName);
    if (temp.size() != 2) {
      const std::string message =
          propertyName + " requires a lower and upper boundary";
      throw std::invalid_argument(message);
    }
    if (temp[0] > temp[1]) {
      throw std::invalid_argument("Min must be <= Max index");
    }
    if (std::find_if(temp.begin(), temp.end(), checkNotPositive) !=
        temp.end()) {
      const std::string message = propertyName + " contains negative indexes";
      throw std::invalid_argument(message);
    }
    // Assignment
    optionalUpperLower = temp;
  }
}

/**
 * Get min max pairs as a tuple.
 * @param minProperty : Property name for the min property
 * @param maxProperty : Property name for the max property
 * @return A tuple consisting of min, max
 */
ReflectometryWorkflowBase::MinMax
ReflectometryWorkflowBase::getMinMax(const std::string &minProperty,
                                     const std::string &maxProperty) const {
  const double min = getProperty(minProperty);
  const double max = getProperty(maxProperty);
  if (min > max) {
    throw std::invalid_argument(
        "Cannot have any WavelengthMin > WavelengthMax");
  }
  return MinMax(min, max);
}
/**
 * Get min max pairs of Optional Properties
 * @param alg : A Pointer to the algorithm to which the properties belong
 * @param minProperty : Property name for the min property
 * @param maxProperty : Property name for the max property
 * @param inst : Pointer to the instrument associated with the workspace (for
 * optional defaults)
 * @param minIdfName : name of the min property component in the instrument
 * defintion (for optional defaults)
 * @param maxIdfName : name of the max property component in the instrument
 * defintion (for optional defaults)
 * @return An initliazed/uninitialized boost::optional of type MinMax.
 */
ReflectometryWorkflowBase::OptionalMinMax
ReflectometryWorkflowBase::getOptionalMinMax(
    Mantid::API::Algorithm *const alg, const std::string &minProperty,
    const std::string &maxProperty,
    Mantid::Geometry::Instrument_const_sptr inst, std::string minIdfName,
    std::string maxIdfName) const {
  const auto min = checkForOptionalInstrumentDefault<double>(alg, minProperty,
                                                             inst, minIdfName);
  const auto max = checkForOptionalInstrumentDefault<double>(alg, maxProperty,
                                                             inst, maxIdfName);
  if (min.is_initialized() && max.is_initialized()) {
    MinMax result = getMinMax(minProperty, maxProperty);
    return OptionalMinMax(result);
  } else {
    return OptionalMinMax();
  }
}

/**
 * Check the first transmission run units.
 *
 * @return True only if the units of the first transmission run are in
 *wavelength.
 *
 */
bool ReflectometryWorkflowBase::validateFirstTransmissionInputs() const {
  WorkspaceUnitValidator tofValidator("TOF");
  WorkspaceUnitValidator wavelengthValidator("Wavelength");
  MatrixWorkspace_sptr firstTransmissionRun =
      this->getProperty("FirstTransmissionRun");
  if (!tofValidator.isValid(firstTransmissionRun).empty() &&
      !wavelengthValidator.isValid(firstTransmissionRun).empty()) {
    throw std::invalid_argument(
        "FirstTransmissionRun must be either in TOF or Wavelength");
  }

  const auto message = wavelengthValidator.isValid(firstTransmissionRun);
  const bool bInWavelength = message.empty();
  return bInWavelength;
}

/**
 * Validate the transmission workspace inputs when a second transmission run is
 * provided.
 * Throws if any of the property values do not make sense.
 * @param firstTransmissionInWavelength: Indicates that the first transmission
 * run is in units of wavlength.
 */
void ReflectometryWorkflowBase::validateSecondTransmissionInputs(
    const bool firstTransmissionInWavelength) const {
  // Verify that all the required inputs for the second transmission run are now
  // given.

  // Check if the first transmission run has been set
  bool ftrDefault = isPropertyDefault("FirstTransmissionRun");
  MatrixWorkspace_sptr ws = this->getProperty("FirstTransmissionRun");
  if (ws)
    ftrDefault = false;

  if (ftrDefault) {
    if (firstTransmissionInWavelength) {
      this->g_log.warning(
          "The first transmission run is in wavelength so is assumed to be "
          "correctly stitched in wavelength. "
          "The second transmission run and associated inputs will be ignored."
          "Run CreateTransmissionWorkspace to create a transmission workspace "
          "from TOF runs.");
    } else {
      throw std::invalid_argument("A SecondTransmissionRun is only valid if a "
                                  "FirstTransmissionRun is provided.");
    }
  } else {

    if (!isPropertyDefault("StartOverlap") &&
        !isPropertyDefault("EndOverlap")) {
      const double startOverlap = this->getProperty("StartOverlap");
      const double endOverlap = this->getProperty("EndOverlap");
      if (startOverlap >= endOverlap) {
        throw std::invalid_argument("EndOverlap must be > StartOverlap");
      }
    }

    if (!isPropertyDefault("SecondTransmissionRun")) {
      MatrixWorkspace_sptr trans1 = this->getProperty("FirstTransmissionRun");
      MatrixWorkspace_sptr trans2 = this->getProperty("SecondTransmissionRun");

      auto firstMap = trans1->getSpectrumToWorkspaceIndexMap();
      auto secondMap = trans2->getSpectrumToWorkspaceIndexMap();
      if (firstMap != secondMap) {
        throw std::invalid_argument("Spectrum maps differ between the "
                                    "transmission runs. They must be the "
                                    "same.");
      }
    }
  }
}

/**
 * Get the transmission run information.
 *
 * Transmission runs are optional, but you cannot have the second without the
 *first. Also, stitching
 * parameters are required if the second is present. This getter fetches and
 *assigns to the optional reference arguments
 *
 * @param firstTransmissionRun
 * @param secondTransmissionRun
 * @param stitchingStart
 * @param stitchingDelta
 * @param stitchingEnd
 * @param stitchingStartOverlap
 * @param stitchingEndOverlap
 */
void ReflectometryWorkflowBase::getTransmissionRunInfo(
    OptionalMatrixWorkspace_sptr &firstTransmissionRun,
    OptionalMatrixWorkspace_sptr &secondTransmissionRun,
    OptionalDouble &stitchingStart, OptionalDouble &stitchingDelta,
    OptionalDouble &stitchingEnd, OptionalDouble &stitchingStartOverlap,
    OptionalDouble &stitchingEndOverlap) const {
  bool bFirstTransInWavelength = false;
  MatrixWorkspace_sptr trans1 = this->getProperty("FirstTransmissionRun");
  if (trans1) {
    bFirstTransInWavelength = validateFirstTransmissionInputs();

    firstTransmissionRun = trans1;
  }

  MatrixWorkspace_sptr trans2 = this->getProperty("SecondTransmissionRun");
  if (trans2) {
    // Check that the property values provided make sense together.
    validateSecondTransmissionInputs(bFirstTransInWavelength);

    // Set the values.
    { secondTransmissionRun = trans2; }
    {
      if (!this->isPropertyDefault("Params")) {
        std::vector<double> params = getProperty("Params");
        if (params.size() == 1) {
          stitchingDelta = params[0];
        } else {
          stitchingStart = params[0];
          stitchingDelta = params[1];
          stitchingEnd = params[2];
        }
      }
    }
    {
      if (!this->isPropertyDefault("StartOverlap")) {
        double temp = this->getProperty("StartOverlap");
        stitchingStartOverlap = temp;
      }
      if (!this->isPropertyDefault("EndOverlap")) {
        double temp = this->getProperty("EndOverlap");
        stitchingEndOverlap = temp;
      }
    }
  }
}

/**
 * Convert the TOF workspace into a monitor workspace. Crops to the monitorIndex
 * and applying flat background correction as part of the process.
 * @param toConvert : TOF wavlength to convert.
 * @param monitorIndex : Monitor index to crop to
 * @param backgroundMinMax : Min and Max Lambda range for Flat background
 * correction.
 * @return The cropped and corrected monitor workspace.
 */
MatrixWorkspace_sptr ReflectometryWorkflowBase::toLamMonitor(
    const MatrixWorkspace_sptr &toConvert, const OptionalInteger monitorIndex,
    const OptionalMinMax &backgroundMinMax) {
  // Convert Units.
  auto convertUnitsAlg = this->createChildAlgorithm("ConvertUnits");
  convertUnitsAlg->initialize();
  convertUnitsAlg->setProperty("InputWorkspace", toConvert);
  convertUnitsAlg->setProperty("Target", "Wavelength");
  convertUnitsAlg->execute();

  // Crop the to the monitor index.
  MatrixWorkspace_sptr monitorWS =
      convertUnitsAlg->getProperty("OutputWorkspace");
  auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", monitorWS);
  cropWorkspaceAlg->setProperty("StartWorkspaceIndex", monitorIndex.get());
  cropWorkspaceAlg->setProperty("EndWorkspaceIndex", monitorIndex.get());
  cropWorkspaceAlg->execute();
  monitorWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

  // If min&max are both 0, we won't do the flat background normalization.
  if (backgroundMinMax.get().get<0>() == 0 &&
      backgroundMinMax.get().get<1>() == 0)
    return monitorWS;

  // Flat background correction
  auto correctMonitorsAlg =
      this->createChildAlgorithm("CalculateFlatBackground");
  correctMonitorsAlg->initialize();
  correctMonitorsAlg->setProperty("InputWorkspace", monitorWS);
  correctMonitorsAlg->setProperty("StartX", backgroundMinMax.get().get<0>());
  correctMonitorsAlg->setProperty("EndX", backgroundMinMax.get().get<1>());
  correctMonitorsAlg->setProperty("SkipMonitors", false);
  correctMonitorsAlg->execute();
  monitorWS = correctMonitorsAlg->getProperty("OutputWorkspace");

  return monitorWS;
}

/**
 * Convert to a detector workspace in lambda.
 * @param processingCommands : Commands to apply to crop and add spectra of the
 * toConvert workspace.
 * @param toConvert : TOF wavelength to convert.
 * @param wavelengthMinMax : Wavelength minmax to keep. Crop out the rest.
 * @return Detector workspace in wavelength
 */
MatrixWorkspace_sptr
ReflectometryWorkflowBase::toLamDetector(const std::string &processingCommands,
                                         const MatrixWorkspace_sptr &toConvert,
                                         const MinMax &wavelengthMinMax) {

  auto convertUnitsAlg = this->createChildAlgorithm("ConvertUnits");
  convertUnitsAlg->initialize();
  convertUnitsAlg->setProperty("InputWorkspace", toConvert);
  convertUnitsAlg->setProperty("Target", "Wavelength");
  convertUnitsAlg->setProperty("AlignBins", true);
  convertUnitsAlg->execute();
  MatrixWorkspace_sptr detectorWS =
      convertUnitsAlg->getProperty("OutputWorkspace");

  // Process the input workspace according to the processingCommands to get a
  // detector workspace

  auto performIndexAlg = this->createChildAlgorithm("GroupDetectors");
  performIndexAlg->initialize();
  performIndexAlg->setProperty("GroupingPattern", processingCommands);
  performIndexAlg->setProperty("InputWorkspace", detectorWS);
  performIndexAlg->execute();
  detectorWS = performIndexAlg->getProperty("OutputWorkspace");

  // Crop out the lambda x-ranges now that the workspace is in wavelength.
  auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
  cropWorkspaceAlg->initialize();
  cropWorkspaceAlg->setProperty("InputWorkspace", detectorWS);
  cropWorkspaceAlg->setProperty("XMin", wavelengthMinMax.get<0>());
  cropWorkspaceAlg->setProperty("XMax", wavelengthMinMax.get<1>());
  cropWorkspaceAlg->execute();
  detectorWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

  return detectorWS;
}

MatrixWorkspace_sptr
ReflectometryWorkflowBase::makeUnityWorkspace(const HistogramX &x) {
  auto createWorkspaceAlg = this->createChildAlgorithm("CreateWorkspace");
  createWorkspaceAlg->initialize();
  createWorkspaceAlg->setProperty("DataX", x.rawData());
  createWorkspaceAlg->setProperty("DataY",
                                  std::vector<double>(x.size() - 1, 1.0));
  createWorkspaceAlg->setProperty("NSpec", 1);
  createWorkspaceAlg->setProperty("DataE",
                                  std::vector<double>(x.size() - 1, 0.0));
  createWorkspaceAlg->setProperty("UnitX", "Wavelength");
  createWorkspaceAlg->execute();
  MatrixWorkspace_sptr unityWorkspace =
      createWorkspaceAlg->getProperty("OutputWorkspace");
  return unityWorkspace;
}

/**
 * Convert From a TOF workspace into a detector and monitor workspace both in
 * Lambda.
 * @param toConvert: TOF workspace to convert
 * @param processingCommands : Detector index ranges
 * @param monitorIndex : Monitor index
 * @param wavelengthMinMax : Wavelength min max for detector workspace
 * @param backgroundMinMax : Wavelength min max for flat background correction
 * of monitor workspace
 * @return Tuple of detector and monitor workspaces
 */
ReflectometryWorkflowBase::DetectorMonitorWorkspacePair
ReflectometryWorkflowBase::toLam(MatrixWorkspace_sptr toConvert,
                                 const std::string &processingCommands,
                                 const OptionalInteger monitorIndex,
                                 const MinMax &wavelengthMinMax,
                                 const OptionalMinMax &backgroundMinMax) {
  // Detector Workspace Processing
  MatrixWorkspace_sptr detectorWS =
      toLamDetector(processingCommands, toConvert, wavelengthMinMax);

  MatrixWorkspace_sptr monitorWS;
  if (monitorIndex.is_initialized() && backgroundMinMax.is_initialized()) {
    // Monitor Workspace Processing
    monitorWS = toLamMonitor(toConvert, monitorIndex, backgroundMinMax);
  } else {
    // We don't have a monitor index, so we divide through by unity.
    monitorWS = makeUnityWorkspace(detectorWS->x(0));
  }

  // Rebin the Monitor Workspace to match the Detector Workspace.
  auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
  rebinToWorkspaceAlg->initialize();
  rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", monitorWS);
  rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", detectorWS);
  rebinToWorkspaceAlg->execute();
  monitorWS = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

  return DetectorMonitorWorkspacePair(detectorWS, monitorWS);
}

} // namespace Algorithms
} // namespace Mantid
