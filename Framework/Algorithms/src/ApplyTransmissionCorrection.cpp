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

void ApplyTransmissionCorrection::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Workspace to apply the transmission correction to");
  declareProperty(std::make_unique<WorkspaceProperty<>>("TransmissionWorkspace",
                                                        "", Direction::Output,
                                                        PropertyMode::Optional),
                  "Workspace containing the transmission values [optional]");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Workspace to store the corrected data in");

  // Alternatively, the user can specify a transmission that will ba applied to
  // all wavelength bins
  declareProperty(
      "TransmissionValue", EMPTY_DBL(),
      "Transmission value to apply to all wavelengths. If specified, "
      "TransmissionWorkspace will not be used.");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("TransmissionError", 0.0, mustBePositive,
                  "The error on the transmission value (default 0.0)");

  declareProperty(
      "ThetaDependent", true,
      "If true, a theta-dependent transmission correction will be applied.");
}

void ApplyTransmissionCorrection::exec() {
  // Check whether we only need to divided the workspace by
  // the transmission.
  const bool thetaDependent = getProperty("ThetaDependent");

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const double trans_value = getProperty("TransmissionValue");
  const double trans_error = getProperty("TransmissionError");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  HistogramY TrIn(inputWS->y(0).size(), trans_value);
  HistogramE ETrIn(inputWS->y(0).size(), trans_error);

  if (isEmpty(trans_value)) {
    // Get the transmission workspace
    MatrixWorkspace_const_sptr transWS = getProperty("TransmissionWorkspace");

    // Check that the two input workspaces are consistent (same number of X
    // bins)
    if (transWS->y(0).size() != inputWS->y(0).size()) {
      g_log.error() << "Input and transmission workspaces have a different "
                       "number of wavelength bins\n";
      throw std::invalid_argument("Input and transmission workspaces have a "
                                  "different number of wavelength bins");
    }

    TrIn = transWS->y(0);
    ETrIn = transWS->e(0);
  }

  const int numHists = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numHists);

  // Create a Workspace2D to match the intput workspace
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
    auto &YOut = corrWS->mutableY(i);
    auto &EOut = corrWS->mutableE(i);

    const double exp_term = 0.5 / cos(spectrumInfo.twoTheta(i)) + 0.5;
    for (int j = 0; j < static_cast<int>(inputWS->y(0).size()); j++) {
      if (!thetaDependent) {
        YOut[j] = 1.0 / TrIn[j];
        EOut[j] = std::fabs(ETrIn[j] * TrIn[j] * TrIn[j]);
      } else {
        EOut[j] = std::fabs(ETrIn[j] * exp_term / pow(TrIn[j], exp_term + 1.0));
        YOut[j] = 1.0 / pow(TrIn[j], exp_term);
      }
    }

    progress.report("Applying Transmission Correction");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS = inputWS * corrWS;
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid