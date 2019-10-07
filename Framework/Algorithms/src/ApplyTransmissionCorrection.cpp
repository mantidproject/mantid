// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ApplyTransmissionCorrection.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyTransmissionCorrection)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace HistogramData;
using namespace DataObjects;

namespace {
namespace PropertyNames {
const std::string INPUT_WKSP("InputWorkspace");
const std::string OUTPUT_WKSP("OutputWorkspace");
const std::string TRANSMISSION_WKSP("TransmissionWorkspace");
const std::string TRANSMISSION_VALUE("TransmissionValue");
const std::string TRANSMISSION_ERROR("TransmissionError");
} // namespace PropertyNames
} // namespace

void ApplyTransmissionCorrection::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(
      std::make_unique<WorkspaceProperty<>>(PropertyNames::INPUT_WKSP, "",
                                            Direction::Input, wsValidator),
      "Workspace to apply the transmission correction to");
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      PropertyNames::TRANSMISSION_WKSP, "", Direction::Output,
                      PropertyMode::Optional),
                  "Workspace containing the transmission values [optional]");
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      PropertyNames::OUTPUT_WKSP, "", Direction::Output),
                  "Workspace to store the corrected data in");

  // Alternatively, the user can specify a transmission that will ba applied to
  // all wavelength bins
  declareProperty(
      PropertyNames::TRANSMISSION_VALUE, EMPTY_DBL(),
      "Transmission value to apply to all wavelengths. If specified, "
      "TransmissionWorkspace will not be used.");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(PropertyNames::TRANSMISSION_ERROR, 0.0, mustBePositive,
                  "The error on the transmission value (default 0.0)");

  declareProperty(
      "ThetaDependent", true,
      "If true, a theta-dependent transmission correction will be applied.");
}

std::map<std::string, std::string>
ApplyTransmissionCorrection::validateInputs() {
  std::map<std::string, std::string> result;

  // needed to find out what mode we are working in
  const double trans_value = getProperty(PropertyNames::TRANSMISSION_VALUE);
  if (isEmpty(trans_value)) { // require a transmission workspace
    MatrixWorkspace_const_sptr transWS =
        getProperty(PropertyNames::TRANSMISSION_WKSP);
    if (!transWS) {
      const std::string msg(
          "Must specify \"TransmissionValue\" or \"TransmissionWorkspace\"");
      result[PropertyNames::TRANSMISSION_VALUE] = msg;
      result[PropertyNames::TRANSMISSION_WKSP] = msg;
    } else {
      MatrixWorkspace_const_sptr inputWS =
          getProperty(PropertyNames::INPUT_WKSP);
      if ((transWS->getNumberHistograms() > 1) &&
          (transWS->getNumberHistograms() != inputWS->getNumberHistograms())) {
        const std::string msg("Input and transmission workspaces have "
                              "incompatible number of spectra");
        result[PropertyNames::INPUT_WKSP] = msg;
        result[PropertyNames::TRANSMISSION_WKSP] = msg;
      } else if (transWS->y(0).size() != inputWS->y(0).size()) {
        const std::string msg("Input and transmission workspaces have a "
                              "different number of wavelength bins");
        result[PropertyNames::INPUT_WKSP] = msg;
        result[PropertyNames::TRANSMISSION_WKSP] = msg;
      }
    }
  }

  return result;
}

void ApplyTransmissionCorrection::exec() {
  const bool thetaDependent = getProperty("ThetaDependent");
  const double trans_value = getProperty(PropertyNames::TRANSMISSION_VALUE);
  const double trans_error = getProperty(PropertyNames::TRANSMISSION_ERROR);
  MatrixWorkspace_const_sptr transWS =
      getProperty(PropertyNames::TRANSMISSION_WKSP);

  // Check whether we only need to divided the workspace by the transmission.
  // theta-dependence modifies the input transmission information
  if (thetaDependent) {
    MatrixWorkspace_sptr inputWS = getProperty(PropertyNames::INPUT_WKSP);

    // initial values of tranmission come from the value
    HistogramY TrIn(inputWS->y(0).size(), trans_value);
    HistogramE ETrIn(inputWS->y(0).size(), trans_error);

    if (isEmpty(trans_value)) {
      // override the values specified above
      TrIn = transWS->y(0);
      ETrIn = transWS->e(0);
    }

    const auto numHists = static_cast<int>(inputWS->getNumberHistograms());
    Progress progress(this, 0.0, 1.0, numHists);

    // Create a Workspace2D to hold the correction (applied through
    // multiplication)
    MatrixWorkspace_sptr corrWS = create<HistoWorkspace>(*inputWS);

    const auto &spectrumInfo = inputWS->spectrumInfo();

    // Loop through the spectra and apply correction
    PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *corrWS))
    for (int i = 0; i < numHists; i++) {
      PARALLEL_START_INTERUPT_REGION

      if (!spectrumInfo.hasDetectors(i)) {
        g_log.warning() << "Workspace index " << i
                        << " has no detector assigned to it - discarding'\n";
        continue;
      }

      // Copy over the X data
      corrWS->setSharedX(i, inputWS->sharedX(i));

      // Skip if we have a monitor or if the detector is masked.
      if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
        continue;

      // Compute theta-dependent transmission term for each wavelength bin
      auto &correctionY = corrWS->mutableY(i);
      auto &correctionE = corrWS->mutableE(i);

      const double exp_term = 0.5 / cos(spectrumInfo.twoTheta(i)) + 0.5;
      for (int j = 0; j < static_cast<int>(inputWS->y(0).size()); j++) {
        correctionY[j] = pow(TrIn[j], -1. * exp_term); // 1 / TrIn^exp_term
        correctionE[j] =
            std::fabs(ETrIn[j] * exp_term / pow(TrIn[j], exp_term + 1.0));
      }

      progress.report("Calculating transmission correction");
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // apply the correction
    MatrixWorkspace_sptr outputWS = inputWS * corrWS;
    ;
    setProperty(PropertyNames::OUTPUT_WKSP, outputWS);
  } else { // !thetaDependent case - divide does the actual work
    // this case divide and set output workspace
    auto divideAlg = createChildAlgorithm("Divide", 0.0, 1.0);
    divideAlg->setProperty("LHSWorkspace",
                           getPropertyValue(PropertyNames::INPUT_WKSP));
    if (transWS) { // use the supplied transmission workspace
      divideAlg->setProperty(
          "RHSWorkspace", getPropertyValue(PropertyNames::TRANSMISSION_WKSP));
    } else {
      // set the RHS to be a single value workspace with the requested
      // uncertainty
      auto createSingleAlg =
          createChildAlgorithm("CreateSingleValuedWorkspace");
      createSingleAlg->setProperty("DataValue", trans_value);
      createSingleAlg->setProperty("ErrorValue", trans_error);
      createSingleAlg->executeAsChildAlg();
      MatrixWorkspace_sptr singleWS =
          createSingleAlg->getProperty("OutputWorkspace");

      divideAlg->setProperty("RHSWorkspace", singleWS);
    }
    divideAlg->setProperty("OutputWorkspace",
                           getPropertyValue(PropertyNames::OUTPUT_WKSP));
    divideAlg->executeAsChildAlg();

    // call divide and set output workspace
    MatrixWorkspace_sptr outputWS = divideAlg->getProperty("OutputWorkspace");
    setProperty(PropertyNames::OUTPUT_WKSP, outputWS);
  }
}

} // namespace Algorithms
} // namespace Mantid
