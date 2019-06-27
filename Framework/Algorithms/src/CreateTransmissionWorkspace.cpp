// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CreateTransmissionWorkspace.h"
#include "MantidAlgorithms/BoostOptionalToAlgorithmProperty.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CreateTransmissionWorkspace)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string CreateTransmissionWorkspace::name() const {
  return "CreateTransmissionWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int CreateTransmissionWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CreateTransmissionWorkspace::category() const {
  return "Reflectometry";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CreateTransmissionWorkspace::init() {
  auto inputValidator = boost::make_shared<WorkspaceUnitValidator>("TOF");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "FirstTransmissionRun", "", Direction::Input,
                      PropertyMode::Mandatory, inputValidator->clone()),
                  "First transmission run, or the low wavelength transmision "
                  "run if SecondTransmissionRun is also provided.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SecondTransmissionRun", "", Direction::Input,
                      PropertyMode::Optional, inputValidator->clone()),
                  "Second, high wavelength transmission run. Optional. Causes "
                  "the InputWorkspace to be treated as the low wavelength "
                  "transmission run.");

  this->initStitchingInputs();
  this->initIndexInputs();
  this->initWavelengthInputs();

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output Workspace IvsQ.");

  setPropertySettings("Params", std::make_unique<Kernel::EnabledWhenProperty>(
                                    "SecondTransmissionRun", IS_NOT_DEFAULT));

  setPropertySettings("StartOverlap",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SecondTransmissionRun", IS_NOT_DEFAULT));

  setPropertySettings("EndOverlap",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SecondTransmissionRun", IS_NOT_DEFAULT));
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CreateTransmissionWorkspace::exec() {
  OptionalMatrixWorkspace_sptr firstTransmissionRun;
  OptionalMatrixWorkspace_sptr secondTransmissionRun;
  OptionalDouble stitchingStart;
  OptionalDouble stitchingDelta;
  OptionalDouble stitchingEnd;
  OptionalDouble stitchingStartOverlap;
  OptionalDouble stitchingEndOverlap;

  // Get the transmission run property information.
  getTransmissionRunInfo(firstTransmissionRun, secondTransmissionRun,
                         stitchingStart, stitchingDelta, stitchingEnd,
                         stitchingStartOverlap, stitchingEndOverlap);
  // Get the monitor i0 index
  auto transWS = firstTransmissionRun.get();
  auto instrument = transWS->getInstrument();
  const OptionalInteger i0MonitorIndex = checkForOptionalInstrumentDefault<int>(
      this, "I0MonitorIndex", instrument, "I0MonitorIndex");

  // Get wavelength intervals.
  const MinMax wavelengthInterval =
      this->getMinMax("WavelengthMin", "WavelengthMax");
  const OptionalMinMax monitorBackgroundWavelengthInterval = getOptionalMinMax(
      this, "MonitorBackgroundWavelengthMin", "MonitorBackgroundWavelengthMax",
      instrument, "MonitorBackgroundMin", "MonitorBackgroundMax");
  const OptionalMinMax monitorIntegrationWavelengthInterval =
      getOptionalMinMax(this, "MonitorIntegrationWavelengthMin",
                        "MonitorIntegrationWavelengthMax", instrument,
                        "MonitorIntegralMin", "MonitorIntegralMax");

  const std::string processingCommands = getWorkspaceIndexList();

  // Create the transmission workspace.
  MatrixWorkspace_sptr outWS = this->makeTransmissionCorrection(
      processingCommands, wavelengthInterval,
      monitorBackgroundWavelengthInterval, monitorIntegrationWavelengthInterval,
      i0MonitorIndex, firstTransmissionRun.get(), secondTransmissionRun,
      stitchingStart, stitchingDelta, stitchingEnd, stitchingStartOverlap,
      stitchingEndOverlap);

  setProperty("OutputWorkspace", outWS);
}

/**
 * Create a transmission corrections workspace utilising one or two workspaces.
 *
 * Input workspaces are in TOF. These are converted to lambda, normalized and
 *stitched together (if two given).
 *
 * @param processingCommands : Processing instructions. Usually a list of
 *detector indexes to keep.
 * @param wavelengthInterval : Wavelength interval for the run workspace.
 * @param wavelengthMonitorBackgroundInterval : Wavelength interval for the
 *monitor background
 * @param wavelengthMonitorIntegrationInterval : Wavelength interval for the
 *monitor integration
 * @param i0MonitorIndex : Monitor index for the I0 monitor
 * @param firstTransmissionRun : The first transmission run
 * @param secondTransmissionRun : The second transmission run (optional)
 * @param stitchingStart : Stitching start (optional but dependent on
 *secondTransmissionRun)
 * @param stitchingDelta : Stitching delta (optional but dependent on
 *secondTransmissionRun)
 * @param stitchingEnd : Stitching end (optional but dependent on
 *secondTransmissionRun)
 * @param stitchingStartOverlap : Stitching start overlap (optional but
 *dependent on secondTransmissionRun)
 * @param stitchingEndOverlap : Stitching end overlap (optional but dependent on
 *secondTransmissionRun)
 * @return A transmission workspace in Wavelength units.
 */
MatrixWorkspace_sptr CreateTransmissionWorkspace::makeTransmissionCorrection(
    const std::string &processingCommands, const MinMax &wavelengthInterval,
    const OptionalMinMax &wavelengthMonitorBackgroundInterval,
    const OptionalMinMax &wavelengthMonitorIntegrationInterval,
    const OptionalInteger &i0MonitorIndex,
    MatrixWorkspace_sptr firstTransmissionRun,
    OptionalMatrixWorkspace_sptr secondTransmissionRun,
    const OptionalDouble &stitchingStart, const OptionalDouble &stitchingDelta,
    const OptionalDouble &stitchingEnd,
    const OptionalDouble &stitchingStartOverlap,
    const OptionalDouble &stitchingEndOverlap) {
  /*make struct of optional inputs to refactor method arguments*/
  /*make a using statements defining OptionalInteger for MonitorIndex*/
  auto trans1InLam =
      toLam(firstTransmissionRun, processingCommands, i0MonitorIndex,
            wavelengthInterval, wavelengthMonitorBackgroundInterval);
  MatrixWorkspace_sptr trans1Detector = trans1InLam.get<0>();
  MatrixWorkspace_sptr trans1Monitor = trans1InLam.get<1>();

  // Monitor integration ... can this happen inside the toLam routine?
  if (wavelengthMonitorIntegrationInterval.is_initialized()) {
    auto integrationAlg = this->createChildAlgorithm("Integration");
    integrationAlg->initialize();
    integrationAlg->setProperty("InputWorkspace", trans1Monitor);
    integrationAlg->setProperty(
        "RangeLower", wavelengthMonitorIntegrationInterval.get().get<0>());
    integrationAlg->setProperty(
        "RangeUpper", wavelengthMonitorIntegrationInterval.get().get<1>());
    integrationAlg->execute();
    trans1Monitor = integrationAlg->getProperty("OutputWorkspace");
  }
  MatrixWorkspace_sptr transmissionWS = divide(trans1Detector, trans1Monitor);
  if (secondTransmissionRun.is_initialized()) {
    auto transRun2 = secondTransmissionRun.get();
    g_log.debug(
        "Extracting second transmission run workspace indexes from spectra");

    auto trans2InLam =
        toLam(transRun2, processingCommands, i0MonitorIndex, wavelengthInterval,
              wavelengthMonitorBackgroundInterval);

    // Unpack the conversion results.
    MatrixWorkspace_sptr trans2Detector = trans2InLam.get<0>();
    MatrixWorkspace_sptr trans2Monitor = trans2InLam.get<1>();

    // Monitor integration ... can this happen inside the toLam routine?
    if (wavelengthMonitorIntegrationInterval.is_initialized()) {
      auto integrationAlg = this->createChildAlgorithm("Integration");
      integrationAlg->initialize();
      integrationAlg->setProperty("InputWorkspace", trans2Monitor);
      integrationAlg->setProperty(
          "RangeLower", wavelengthMonitorIntegrationInterval.get().get<0>());
      integrationAlg->setProperty(
          "RangeUpper", wavelengthMonitorIntegrationInterval.get().get<1>());
      integrationAlg->execute();
      trans2Monitor = integrationAlg->getProperty("OutputWorkspace");
    }

    MatrixWorkspace_sptr normalizedTrans2 =
        divide(trans2Detector, trans2Monitor);

    // Stitch the results.
    auto stitch1DAlg = this->createChildAlgorithm("Stitch1D");
    stitch1DAlg->initialize();
    AnalysisDataService::Instance().addOrReplace("transmissionWS",
                                                 transmissionWS);
    AnalysisDataService::Instance().addOrReplace("normalizedTrans2",
                                                 normalizedTrans2);
    stitch1DAlg->setProperty("LHSWorkspace", transmissionWS);
    stitch1DAlg->setProperty("RHSWorkspace", normalizedTrans2);
    if (stitchingStartOverlap.is_initialized()) {
      stitch1DAlg->setProperty("StartOverlap", stitchingStartOverlap.get());
    }
    if (stitchingEndOverlap.is_initialized()) {
      stitch1DAlg->setProperty("EndOverlap", stitchingEndOverlap.get());
    }
    if (stitchingStart.is_initialized() && stitchingEnd.is_initialized() &&
        stitchingDelta.is_initialized()) {
      const std::vector<double> params = {
          stitchingStart.get(), stitchingDelta.get(), stitchingEnd.get()};
      stitch1DAlg->setProperty("Params", params);
    } else if (stitchingDelta.is_initialized()) {
      const double delta = stitchingDelta.get();
      stitch1DAlg->setProperty("Params", std::vector<double>(1, delta));
    }
    stitch1DAlg->execute();
    transmissionWS = stitch1DAlg->getProperty("OutputWorkspace");
    AnalysisDataService::Instance().remove("transmissionWS");
    AnalysisDataService::Instance().remove("normalizedTrans2");
  }

  return transmissionWS;
}
} // namespace Algorithms
} // namespace Mantid
